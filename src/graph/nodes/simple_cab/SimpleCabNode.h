#pragma once
#include "thirdparty/fftconvolver/TwoStageFFTConvolver.h"
#include "config.h"
#include "src/graph/Node.h"
#include "src/graph/nodes/simple_cab/c.h"
#include "src/graph/nodes/simple_cab/cident.h"
#include "src/graph/nodes/simple_cab/clean.h"

class SimpleCabNode : public Node {
  fftconvolver::FFTConvolver convolver;
  float* convertBufferIn;
  float* convertBufferOut;
  double selectedIr;
public:
  SimpleCabNode() : Node() {
    type = "SimpleCabNode";
    convolver.init(64, cleanIR, 3000);
    convertBufferIn = new float[1024];
    convertBufferOut = new float[1024];
    selectedIr = 0;
  }

  void setup(int p_samplerate = 48000, int p_maxBuffer = 512, int p_channles = 2, int p_inputs = 1, int p_outputs = 1) {
    Node::setup(p_samplerate, p_maxBuffer, 2, 1, 1);
    ParameterCoupling* p = new ParameterCoupling("IR", &selectedIr, 0.0, 0.0, 2.0, 1.0);
    p->x = 100;
    p->y = 100;
    parameters.Add(p);
  }

  ~SimpleCabNode() {
    delete convertBufferIn;
    delete convertBufferOut;
  }

  void ProcessBlock(int nFrames) {
    if (isProcessed) { return; }
    for (int i = 0; i < inputCount; i++) {
      if (!inSockets.Get(0)->connectedNode->isProcessed) {
        return;
      }
    }
    //int prev = (int)*(parameters[0]->value);
    //parameters[0]->update();
    //int cur = (int)*(parameters[0]->value);
    //if (prev != cur) {
    //  if (cur == 0) {
    //    convolver.init(64, cleanIR, 3000);
    //  }
    //  if (cur == 1) {
    //    convolver.init(64, stackIR, 4500);
    //  }
    //  if (cur == 2) {
    //    convolver.init(64, driveIR, 4200);
    //  }
    //}

    float inverseChannelCount = 1.0 / channelCount;
    for (int i = 0; i < nFrames; i++) {
      convertBufferIn[i] = 0;
      for (int c = 0; c < channelCount; c++) {
        convertBufferIn[i] += inSockets.Get(0)->buffer[c][i];
      }
      convertBufferIn[i] *= inverseChannelCount;
    }
    
    convolver.process(convertBufferIn, convertBufferOut, nFrames);
    
    for (int i = 0; i < nFrames; i++) {
      for (int c = 0; c < channelCount; c++) {
         outputs[0][c][i] = convertBufferOut[i];
      }
    }

    isProcessed = true;
  }

  void setupUi(iplug::igraphics::IGraphics* pGrahics) override {
    //background = new NodeBackground(pGrahics, PNGSIMPLECABBG_FN, L, T,
    //  [&](float x, float y) {
    //  this->translate(x, y);
    //});
    //pGrahics->AttachControl(background);
    Node::setupUi(pGrahics);
    // uiReady = true;
  }
};
