#pragma once
#include "IControl.h"
#include "src/misc/MessageBus.h"
#include "NodeGalleryElement.h"

namespace guitard {
  class GalleryCategory : public IControl {
    WDL_PtrList<GalleryElement> mElements;
    bool mOpen = false;
    WDL_String mName;
    MessageBus::Bus* mBus = nullptr;
    IRECT mTitleRect;
    float mColumns = 0;
  public:

    GalleryCategory(MessageBus::Bus* bus) : IControl({}) {
      mBus = bus;
    }

    ~GalleryCategory() {
      mElements.Empty(true);
    }

    void addNode(const NodeList::NodeInfo node) {
      if (mName.GetLength() == 0) {
        // Take the name of the first node, they'll all be the same
        mName.Set(node.categoryName.c_str());
      }
      mElements.Compact();
      mElements.Add(new GalleryElement(node));
    }

    void OnResize() override {
      // Title is always visible
      mRECT.B = mRECT.T + Theme::Gallery::ELEMENT_TITLE_HEIGHT;
      if (mOpen) {
        // Calculate own height
        mTitleRect = mRECT;
        mColumns = std::max(static_cast<int>(
          floor(mRECT.W() / (Theme::Gallery::ELEMENT_WIDTH
            + Theme::Gallery::ELEMENT_PADDING * 1.5))), 1
        );
        int rows = ceilf(mElements.GetSize() / mColumns);
        mRECT.B +=
          rows * Theme::Gallery::ELEMENT_HEIGHT + rows
          * Theme::Gallery::ELEMENT_PADDING + Theme::Gallery::ELEMENT_PADDING;
      }
      else {
        mTitleRect = mRECT;
      }
      mTargetRECT = mRECT;
    }

    void Draw(IGraphics& g) override {
      if (mOpen) {
        if (mMouseIsOver) {
          g.FillRect(Theme::Gallery::CATEGORY_BG_HOVER, mRECT);
        }
        else {
          g.FillRect(Theme::Gallery::CATEGORY_BG, mRECT);
        }
        for (int i = 0; i < mElements.GetSize(); i++) {
          mElements.Get(i)->Draw(g, &mRECT, i, mColumns);
        }
      }

      if (mOpen) {
        g.FillRect(Theme::Gallery::CATEGORY_TITLE_BG_OPEN, mTitleRect);
      }
      else if (mMouseIsOver) {
        g.FillRect(Theme::Gallery::CATEGORY_TITLE_BG_HOVER, mTitleRect);
      }
      else {
        g.FillRect(Theme::Gallery::CATEGORY_TITLE_BG, mTitleRect);
      }
      g.DrawText(Theme::Gallery::CATEGORY_TITLE, mName.Get(), mTitleRect);
    }

    void OnMouseOver(float x, float y, const IMouseMod& mod) override {
      IControl::OnMouseOver(x, y, mod);
      const IRECT click = { x, y, x, y };
      for (int i = 0; i < mElements.GetSize(); i++) {
        GalleryElement* g = mElements.Get(i);
        g->mMouseIsOver = g->mRECT.Contains(click);
      }
    }

    void OnMouseOut() override {
      IControl::OnMouseOut();
      for (int i = 0; i < mElements.GetSize(); i++) {
        mElements.Get(i)->mMouseIsOver = false;
      }
    }

    void OnMouseUp(const float x, const float y, const IMouseMod& mod) override {
      IRECT p(x, y, x, y);
      if (mTitleRect.Contains(p)) {
        mOpen = !mOpen;
        OnResize();
        return;
      }
      for (int i = 0; i < mElements.GetSize(); i++) {
        GalleryElement* elem = mElements.Get(i);
        if (elem->mRECT.Contains(p)) {
          MessageBus::fireEvent<NodeList::NodeInfo>(mBus, MessageBus::NodeAdd, elem->mInfo);
        }
      }
    }
  };
}