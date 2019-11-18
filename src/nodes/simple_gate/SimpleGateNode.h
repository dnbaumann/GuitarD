#pragma once
#include "SimpleGate.h"

class SimpleGateNode : public SimpleGate {
public:
  SimpleGateNode(std::string pType) {
    type = pType;
  }

  void setupUi(iplug::igraphics::IGraphics* pGrahics) override {
    Node::setupUi(pGrahics);
    mUi->setColor(CATEGORYCOLORDYNAMICS);
  }
};