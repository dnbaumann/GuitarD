#pragma once
#include "Fuzz.h"

namespace guitard {
  class FuzzNode final : public FaustGenerated::Fuzz {
  public:
    FuzzNode(const std::string pType) {
      shared.type = pType;
    }

    void setupUi(iplug::igraphics::IGraphics* pGrahics) override {
      Node::setupUi(pGrahics);
      mUi->setColor(Theme::Categories::DISTORTION);
    }

    std::string getLicense() override {
      std::string l = "\nFaust code from Guitarix, probably needs to be replaced/removed";
      l += Fuzz::getLicense();
      return l;
    }
  };
}