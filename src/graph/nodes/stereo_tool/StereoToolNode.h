#pragma once
#include "StereoTool.h"

class StereoToolNode : public StereoTool {
 public:
  StereoToolNode() {
    type = "StereoToolNode";
  }

  void setupUi(iplug::igraphics::IGraphics* pGrahics) override {
    //background = new NodeBackground(pGrahics, PNGSTEREOSHAPERBG_FN, L, T,
    //  [&](float x, float y) {
    //    this->translate(x, y);
    //  });
    //pGrahics->AttachControl(background);
    //parameters[0]->x = 80;
    //parameters[0]->y = 80;
    //parameters[1]->x = 125;
    //parameters[1]->y = 140;
    //parameters[1]->w = 40;
    //parameters[1]->h = 40;
    //parameters[2]->x = 175;
    //parameters[2]->y = 80;
    Node::setupUi(pGrahics);
  }
};