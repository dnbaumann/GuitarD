#pragma once
#include "src/graph/misc/NodeList.h"
#include "src/graph/nodes/stereo_tool/StereoToolNode.h"
#include "src/graph/nodes/simple_drive/SimpleDriveNode.h"
#include "src/graph/nodes/simple_delay/SimpleDelayNode.h"
#include "src/graph/nodes/simple_cab/SimpleCabNode.h"
#include "src/graph/nodes/crybaby/CryBabyNode.h"
#include "src/graph/nodes/combine/CombineNode.h"
#include "src/graph/nodes/feedback/FeedbackNode.h"
#include "src/graph/nodes/simple_reverb/SimpleReverbNode.h"
#include "src/graph/nodes/simple_gate/SimpleGateNode.h"
#include "src/graph/nodes/simple_compressor/SimpleComressorNode.h"
#include "src/graph/nodes/fuzz/FuzzNode.h"
#include "src/graph/nodes/bitcrusher/BitCrusherNode.h"
#include "src/graph/nodes/overdrive/OverDriveNode.h"


namespace NodeList {
  void registerNodes() {
    NodeList::registerNode(NodeList::NodeInfo {
      []() { return new StereoToolNode("StereoToolNode"); },
      "StereoToolNode",
      "Stereo Tool",
      "asd",
      "Tools"
    });

    NodeList::registerNode(NodeList::NodeInfo{
      []() { return new CombineNode("CombineNode"); },
      "CombineNode",
      "Combine",
      "asd",
      "Tools"
    });

    NodeList::registerNode(NodeList::NodeInfo{
      []() { return new FeedbackNode("FeedbackNode"); },
      "FeedbackNode",
      "Feedback",
      "asd",
      "Tools"
    });

    NodeList::registerNode(NodeList::NodeInfo{
      []() { return new SimpleDriveNode("SimpleDriveNode"); },
      "SimpleDriveNode",
      "Simple Drive",
      "asd",
      "Distortion"
    });

    NodeList::registerNode(NodeList::NodeInfo{
      []() { return new OverDriveNode("OverDriveNode"); },
        "OverDriveNode",
        "Overdrive",
        "asd",
        "Distortion"
      });

    NodeList::registerNode(NodeList::NodeInfo{
      []() { return new FuzzNode("FuzzNode"); },
      "FuzzNode",
      "Fuzz",
      "asd",
      "Distortion"
    });

    NodeList::registerNode(NodeList::NodeInfo{
      []() { return new BitCrusherNode("BitCrusherNode"); },
      "BitCrusherNode",
      "Bitcrusher",
      "asd",
      "Distortion"
    });

    NodeList::registerNode(NodeList::NodeInfo{
      []() { return new SimpleDelayNode("SimpleDelayNode"); },
      "SimpleDelayNode",
      "Mono Delay",
      "asd",
      "Delays/Reverbs"
    });

    NodeList::registerNode(NodeList::NodeInfo{
      []() { return new SimpleReverbNode("SimpleReverbNode"); },
      "SimpleReverbNode",
      "Stereo Reverb",
      "asd",
      "Delays/Reverbs"
    });

    NodeList::registerNode(NodeList::NodeInfo{
      []() { return new SimpleCabNode("SimpleCabNode"); },
      "SimpleCabNode",
      "Simple Cabinet",
      "asd",
      "Cabinets"
    });

    NodeList::registerNode(NodeList::NodeInfo{
      []() { return new CryBabyNode("CryBabyNode"); },
      "CryBabyNode",
      "Crybaby",
      "asd",
      "Filters"
    });

    NodeList::registerNode(NodeList::NodeInfo{
      []() { return new SimpleComressorNode("SimpleComressorNode"); },
      "SimpleComressorNode",
      "Simple Compressor",
      "asd",
      "Dynamics"
    });

    NodeList::registerNode(NodeList::NodeInfo{
      []() { return new SimpleGateNode("SimpleGateNode"); },
      "SimpleGateNode",
      "Simple Gate",
      "asd",
      "Dynamics"
    });
  }
}