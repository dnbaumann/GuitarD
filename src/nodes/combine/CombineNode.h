#pragma once
#include "src/node/Node.h"


class CombineNode final : public Node {
  const double smoothing = 0.999;
  double smoothed[8] = { 0 };
  double pan1 = 0;
  double pan2 = 0;
  double mix = 0.5;
  sample** emptyBuffer;
public:
  CombineNode(std::string pType) {
    pan1 = pan2 = 0;
    mix = 0.5;
    mType = pType;
    shared.width = 200;
    shared.height = 150;
  }

  void ProcessBlock(const int nFrames) override {
    if (mIsProcessed) { return; }
    NodeSocket* s1 = shared.socketsIn[0];
    NodeSocket* s2 = shared.socketsIn[1];

    // see which inputs are connected
    const bool has1 = s1->mConnectedNode != nullptr;
    const bool has2 = s2->mConnectedNode != nullptr;
    if (has1 == has2 && has1 == false) {
      outputSilence();
      return;
    }

    if ((has1 && !s1->mConnectedNode->mIsProcessed) || has2 && !s2->mConnectedNode->mIsProcessed) {
      // skip until inputs are ready
      return;
    }

    // Choose the buffer from the input or use silence
    sample** buffer1 = has1 ? s1->mConnectedTo->mParentBuffer : emptyBuffer;
    sample** buffer2 = has2 ? s2->mConnectedTo->mParentBuffer : emptyBuffer;

    // Update the params
    shared.parameters[0]->update();
    shared.parameters[1]->update();
    shared.parameters[2]->update();

    // prepare the values
    const double mix = *(shared.parameters[2]->value);
    const double invMix = 1 - mix;
    const double pan1 = *(shared.parameters[0]->value);
    const double pan2 = *(shared.parameters[1]->value);
    const double pan1l = min(1.0, max(-pan1 + 1.0, 0.0)) * invMix * (1.0 - smoothing);
    const double pan1r = min(1.0, max(+pan1 + 1.0, 0.0)) * invMix * (1.0 - smoothing);
    const double pan2l = min(1.0, max(-pan2 + 1.0, 0.0)) * mix * (1.0 - smoothing);
    const double pan2r = min(1.0, max(+pan2 + 1.0, 0.0)) * mix * (1.0 - smoothing);

    // do the math
    for (int i = 0; i < nFrames; i++) {
      smoothed[0] = pan1l + smoothed[1] * smoothing;
      smoothed[2] = pan2l + smoothed[3] * smoothing;
      smoothed[4] = pan1r + smoothed[5] * smoothing;
      smoothed[6] = pan2r + smoothed[7] * smoothing;
      mBuffersOut[0][0][i] = buffer1[0][i] * smoothed[0] + buffer2[0][i] * smoothed[2];
      mBuffersOut[0][1][i] = buffer1[1][i] * smoothed[4] + buffer2[1][i] * smoothed[6];
      smoothed[1] = smoothed[0];
      smoothed[3] = smoothed[2];
      smoothed[5] = smoothed[4];
      smoothed[7] = smoothed[6];
    }

    mIsProcessed = true;
  }

  void setup(MessageBus::Bus* pBus, const int pSamplerate = 48000, const int pMaxBuffer = 512, int pChannles = 2, int pInputs = 1, int pOutputs = 1) {
    Node::setup(pBus, pSamplerate, pMaxBuffer, 2, 2, 1);
    ParameterCoupling* p = new ParameterCoupling(
      "PAN 1", &pan1, 0.0, -1.0, 1.0, 0.01
    );
    p->x = -40;
    p->y = -20;
    shared.parameters[shared.parameterCount] = p;
    shared.parameterCount++;

    p = new ParameterCoupling(
      "PAN 2", &pan2, 0.0, -1.0, 1.0, 0.01
    );
    p->x = -40;
    p->y = 40;
    shared.parameters[shared.parameterCount] = p;
    shared.parameterCount++;

    p = new ParameterCoupling(
      "MIX", &mix, 0.5, 0.0, 1.0, 0.01
    );
    p->x = 40;
    p->y = 5;
    shared.parameters[shared.parameterCount] = p;
    shared.parameterCount++;

    shared.socketsIn[0]->mY = +40;
    shared.socketsIn[1]->mY = +100;
    shared.socketsOut[0]->mY = +70;
    shared.socketsOut[0]->mY = +70;
  }

  void deleteBuffers() override {
    Node::deleteBuffers();
    if (emptyBuffer != nullptr) {
      for (int c = 0; c < mChannelCount; c++) {
        delete emptyBuffer[c];
      }
      emptyBuffer = nullptr;
    }
  }

  void createBuffers() override {
    Node::createBuffers();
    // this will be used to do processing on a disconnected node
    emptyBuffer = new sample * [mChannelCount];
    for (int c = 0; c < mChannelCount; c++) {
      emptyBuffer[c] = new sample[mMaxBuffer];
      for (int i = 0; i < mMaxBuffer; i++) {
        emptyBuffer[c][i] = 0;
      }
    }
  }

  void setupUi(iplug::igraphics::IGraphics* pGrahics) override {
    Node::setupUi(pGrahics);
    mUi->setColor(Theme::Categories::TOOLS);
  }
};