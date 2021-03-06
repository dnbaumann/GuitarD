#include "./guitard.h"
#include "./src/nodes/RegisterNodes.h"

using namespace iplug;
#include "IPlug_include_in_plug_src.h"



GuitarD::GuitarD(const InstanceInfo& info) : iplug::Plugin(info, MakeConfig(GUITARD_MAX_DAW_PARAMS, kNumPrograms)) {
  /**
   * Setup the soundwoofer lib
   */
  soundwoofer::setup::setPluginName(PLUG_NAME);
  WDL_String path;
#ifndef OS_LINUX
  UserHomePath(path);
#else // UserHomePath causes a linking error on linux for some reason
  char* homeDir = getenv("HOME"); // maybe call free on it
  path.Set(homeDir);
#endif
  soundwoofer::setup::setHomeDirectory(path.Get());

  mParamManager = new guitard::ParameterManager();
  mParamManager->setParamChangeCallback([&]() {
    /**
     * The DAW needs to be informed if the assignments for parameters have changed
     * TODOG This doesn't work for VST3 right now for some reason
     */
    this->InformHostOfParameterDetailsChange();
  });

  for (int i = 0; i < GUITARD_MAX_DAW_PARAMS; i++) {
    // Gather a good amount of parameters to expose to the daw so they can be assigned internally
    mParamManager->addParameter(GetParam(i));
  }

  mGraph = new guitard::Graph();
  mGraph->setParameterManager(mParamManager);

  /**
   * Distributed UI won't work since the editor doesn't only affect IParams
   * Syncing them up and separating the gui from the main classes would require some work
   */
#if IPLUG_EDITOR
  mMakeGraphicsFunc = [&]() {
    return igraphics::MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, 1.);
  };
  
  mLayoutFunc = [&](igraphics::IGraphics* pGraphics) {
    if (pGraphics->NControls()) {
      return;
    }
    pGraphics->SetSizeConstraints(PLUG_MIN_WIDTH, PLUG_MAX_WIDTH, PLUG_MIN_HEIGHT, PLUG_MAX_HEIGHT);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    pGraphics->LoadFont("ForkAwesome", ICON_FN);
    mGraphUi = new guitard::GraphUi(&mBus, pGraphics);
    mGraphUi->setGraph(mGraph);
  };
#endif
}

void GuitarD::OnReset() {
  if (mGraph != nullptr) {
    const int sr = static_cast<int>(GetSampleRate());
    const int outputChannels = NOutChansConnected();
    const int inputChannels = NInChansConnected();
    if (sr > 0 && outputChannels > 0 && inputChannels > 0) {
      /**
       * Depending on the DAW we might not be getting valid values just yet
       * So we check them and then inform the Graph
       */
      mGraph->OnReset(sr, outputChannels, inputChannels);
      mReady = true;
    }
  }
}

void GuitarD::OnActivate(bool active) {
  if (!active && mReady) {
    OnReset();
  }
}

void GuitarD::OnUIClose() {
  delete mGraphUi;
  mGraphUi = nullptr;
}

bool GuitarD::SerializeState(iplug::IByteChunk& chunk) const {
  WDL_String serialized;
  mGraph->serialize(serialized);
  if (serialized.GetLength() < 1) {
    return false;
  }
  chunk.PutStr(serialized.Get());
  return true;
}

int GuitarD::UnserializeState(const iplug::IByteChunk& chunk, int startPos) {
  WDL_String json_string;
  const int pos = chunk.GetStr(json_string, startPos);
  if (mGraphUi == nullptr) {
    mGraph->deserialize(json_string.Get());
  }
  else {
    mGraphUi->deserialize(json_string.Get());
  }
  return pos;
}

#if IPLUG_DSP
void GuitarD::ProcessBlock(iplug::sample** inputs, iplug::sample** outputs, int nFrames) {
  // Some DAWs already call the process function without having provided a valid samplerate/channel configuration
  if (mReady) {
    mGraph->ProcessBlock(inputs, outputs, nFrames);
  }
  else {
    OnReset();
    // Fill the output buffer just in case it wasn't initialized with silence
    const int nChans = NOutChansConnected();
    for (int c = 0; c < nChans; c++) {
      for (int i = 0; i < nFrames; i++) {
        outputs[c][i] = inputs[c][i];
      }
    }
  }
}
#endif
