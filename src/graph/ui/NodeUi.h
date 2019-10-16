#pragma once
#include "IControl.h"
#include "src/graph/misc/ParameterCoupling.h"
#include "src/graph/misc/NodeSocket.h"

typedef std::function<void(float x, float y)> NodeUiCallback;
struct NodeUiParam {
  iplug::igraphics::IGraphics* pGraphics;
  const char* pBg;
  float* X;
  float* Y;
  WDL_PtrList<ParameterCoupling>* pParameters;
  WDL_PtrList<NodeSocket>* inSockets;
  WDL_PtrList<NodeSocket>* outSockets;
  // NodeUiCallback pCallback;
};

using namespace iplug;
using namespace igraphics;


/**
 * This class represents a Node on the UI, it's seperate to the node itself
 * since it will only exists as long as the UI window is open but is owned by the node
 */
class NodeUi : public IControl {
  WDL_PtrList<ParameterCoupling>* mParameters;
  WDL_PtrList<NodeSocket>* mInSockets;
  WDL_PtrList<NodeSocket>* mOutSockets;
public:
  NodeUi(NodeUiParam pParam) :
    IControl(IRECT(0, 0, 0, 0), kNoParameter)
  {
    X = pParam.X;
    Y = pParam.Y;
    mGraphics = pParam.pGraphics;
    mParameters = pParam.pParameters;
    mInSockets = pParam.inSockets;
    mOutSockets = pParam.outSockets;

    mBitmap = mGraphics->LoadBitmap(pParam.pBg, 1, false);
    float w = mBitmap.W();
    float h = mBitmap.H();
    IRECT rect;
    rect.L = *X - w / 2;
    rect.R = *X + w / 2;
    rect.T = *Y - h / 2;
    rect.B = *Y + h / 2;
    SetTargetAndDrawRECTs(rect);
    mBlend = EBlend::Clobber;

    for (int i = 0; i < mParameters->GetSize(); i++) {
      ParameterCoupling* couple = mParameters->Get(i);
      float px = *X + couple->x - (couple->w * 0.5);
      float py = *Y + couple->y - (couple->h * 0.5);
      iplug::igraphics::IRECT controlPos(px, py, px + couple->w, py + couple->h);
      // use the daw parameter to sync the values if possible
      if (couple->parameterIdx != iplug::kNoParameter) {
        couple->control = new iplug::igraphics::IVKnobControl(
          controlPos, couple->parameterIdx
        );
      }
      else {
        // use the callback to get tha value to the dsp, won't allow automation though
        couple->control = new iplug::igraphics::IVKnobControl(
          controlPos, [couple](iplug::igraphics::IControl* pCaller) {
          *(couple->value) = pCaller->GetValue();
        }
        );
      }
      couple->control->SetValue(couple->defaultVal);
      mGraphics->AttachControl(couple->control);
      couple->control->SetValue(couple->defaultVal);
      couple->control->SetValueToDefault();

      // optinally hide the lables etc
      //IVectorBase* vcontrol = dynamic_cast<IVectorBase*>(couple->control);
      //if (vcontrol != nullptr) {
      //  vcontrol->SetShowLabel(false);
      //  vcontrol->SetShowValue(false);
      //}
    }
  }

  ~NodeUi() {
    for (int i = 0; i < mParameters->GetSize(); i++) {
      ParameterCoupling* param = mParameters->Get(i);
      if (param->control != nullptr) {
        // this also destroys the object
        mGraphics->RemoveControl(param->control, true);
        param->control = nullptr;
      }
    }
  }


  void Draw(IGraphics& g) override {
    g.DrawBitmap(mBitmap, mRECT, 1, &mBlend);
  }

  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override {
    translate(dX, dY);
    //mCallback(dX, dY);
  }

  void translate(float dX, float dY) {
    for (int i = 0; i < mParameters->GetSize(); i++) {
      moveControl(mParameters->Get(i)->control, dX, dY);
    }
    moveControl(this, dX, dY);
    *X += dX;
    *Y += dY;
    //for (int i = 0; i < inputCount; i++) {
    //  moveControl(inSockets[i], x, y);
    //}
    //for (int i = 0; i < outputCount; i++) {
    //  moveControl(outSockets[i], x, y);
    //}

    mGraphics->SetAllControlsDirty();
  }

private:
  void moveControl(IControl* control, float x, float y) {
    if (control == nullptr) { return; }
    IRECT rect = control->GetRECT();
    rect.T += y;
    rect.L += x;
    rect.B += y;
    rect.R += x;
    control->SetTargetAndDrawRECTs(rect);
  }

protected:
  float* X;
  float* Y;
  IBitmap mBitmap;
  IBlend mBlend;
  IGraphics* mGraphics;
  NodeUiCallback mCallback;
};
