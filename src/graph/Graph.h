#pragma once
#include <chrono>
#include "mutex.h"
#include "thirdparty/json.hpp"
#include "IPlugConstants.h"
#include "src/misc/MessageBus.h"
#include "src/nodes/io/InputNode.h"
#include "src/nodes/io/OutputNode.h"
#include "src/ui/GraphBackground.h"
#include "Serializer.h"
#include "SortGraph.h" 
#include "src/parameter/ParameterManager.h"
#include "src/ui/CableLayer.h"
#include "src/ui/gallery/NodeGallery.h"
#include "src/misc/HistoryStack.h"
#include "FormatGraph.h"
#include "thirdparty/soundwoofer.h"

/**
 * This is the "god object" which will handle all the nodes
 * and interactions with the graph
 * It's not a IControl itself but owns a few which make up the GUI
 */
class Graph {
  MessageBus::Bus* mBus = nullptr;
  MessageBus::Subscription<Node*> mNodeDelSub;
  MessageBus::Subscription<Node*> mNodeBypassEvent;
  MessageBus::Subscription<Node*> mNodeCloneEvent;
  MessageBus::Subscription<Node*> mNodeSpliceCombineEvent;
  MessageBus::Subscription<NodeList::NodeInfo> mNodeAddEvent;
  MessageBus::Subscription<bool> mAwaitAudioMutexEvent;
  MessageBus::Subscription<bool> mPushUndoState;
  MessageBus::Subscription<bool> mPopUndoState;
  MessageBus::Subscription<GraphStats**> mReturnStats;
  MessageBus::Subscription<AutomationAttachRequest> mAutomationRequest;

  IGraphics* mGraphics = nullptr;

  /**
   * Holds all the nodes in the processing graph
   */
  WDL_PtrList<Node> mNodes;

  /**
   * Mutex to keep changes to the graph like adding/removing or rerouting from crashing
   */
  WDL_Mutex mAudioMutex;
  int mPauseAudio = 0;

  /**
   * Dummy nodes to get the audio blocks in and out of the graph
   */
  InputNode* mInputNode;
  OutputNode* mOutputNode;

  /**
   * This is the channel count to be used internally
   * All nodes will allocate buffers to according to this
   * Using anything besides stereo will cause problems with the faust DSP code
   */
  int mChannelCount = 0;

  /**
   * This is the actual input channel count provided by the DAW
   * The input channel count can be 1 if the plugin is on a mono track
   * But internal processing will still happen in stereo
   */
  int mInPutChannelCount = 0;

  int mSampleRate = 0;

  int mMaxBlockSize = MAX_BUFFER;

  /**
   * Control elements
   */
  GraphBackground* mBackground = nullptr; // Always at the bottom
  CableLayer* mCableLayer = nullptr; // Always below the Gallery
  NodeGallery* mNodeGallery = nullptr; // Always top most

  HistoryStack mHistoryStack;

  GraphStats mStats;

  /**
   * Editor window properties
   * Kept around for the serialization
   */
  int mWindowWidth = 0;
  int mWindowHeight = 0;
  float mWindowScale = 0;

  /**
   * Used to slice the dsp block in smaller slices
   */
  sample** mSliceBuffer[2] = { nullptr };

public:
  ParameterManager mParamManager;


  explicit Graph(MessageBus::Bus* pBus) : mParamManager(pBus) {
    //SoundWoofer::instance().fetchIRs([](bool res) {
    //  int i = 0;
    //});

    auto &sw = SoundWoofer::instance();
    sw.setPluginName("testplugin");
    sw.sendPreset("presetname", "somedata", 8);
    //sw.fetchIRs();
    //sw.downloadIR(sw.getIRs().at(0));

    mBus = pBus;
    
    mInputNode = new InputNode(mBus);
    mOutputNode = new OutputNode(mBus);
    mOutputNode->connectInput(mInputNode->shared.socketsOut[0]);

    /**
     * All the events the Graph is subscribed to
     */

    mNodeDelSub.subscribe(mBus, MessageBus::NodeDeleted, [&](Node* param) {
      MessageBus::fireEvent(mBus, MessageBus::PushUndoState, false);
      this->removeNode(param, true);
    });

    mNodeBypassEvent.subscribe(mBus, MessageBus::BypassNodeConnection, [&](Node* param) {
      MessageBus::fireEvent(mBus, MessageBus::PushUndoState, false);
      this->byPassConnection(param);
    });

    mNodeAddEvent.subscribe(mBus, MessageBus::NodeAdd, [&](const NodeList::NodeInfo &info) {
      MessageBus::fireEvent(mBus, MessageBus::PushUndoState, false);
      this->addNode(info.constructor(), nullptr,300, 300);
    });

    mNodeCloneEvent.subscribe(mBus, MessageBus::CloneNode, [&](Node* node) {
      Node* clone = NodeList::createNode(node->shared.type);
      if (clone != nullptr) {
        this->addNode(clone, nullptr, node->shared.X, node->shared.Y, 0, 0, node);
        clone->mUi->mDragging = true;
        mGraphics->SetCapturedControl(clone->mUi);
      }
    });

    mAwaitAudioMutexEvent.subscribe(mBus, MessageBus::AwaitAudioMutex, [&](const bool doPause) {
      if (doPause) {
        this->lockAudioThread();
      }
      else {
        this->unlockAudioThread();
      }
      
    });

    mPushUndoState.subscribe(mBus, MessageBus::PushUndoState, [&](bool) {
      WDBGMSG("PushState");
      this->serialize(*(mHistoryStack.pushState()));
    });

    mPopUndoState.subscribe(mBus, MessageBus::PopUndoState, [&](const bool redo) {
      nlohmann::json* state = mHistoryStack.popState(redo);
      if (state != nullptr) {
        WDBGMSG("PopState");
        this->deserialize(*state);
      }
    });

    mReturnStats.subscribe(mBus, MessageBus::GetGraphStats, [&](GraphStats** stats) {
      *stats = &mStats;
    });

    mNodeSpliceCombineEvent.subscribe(mBus, MessageBus::NodeSpliceInCombine, [&](Node* node) {
      this->spliceInCombine(node);
    });

    mAutomationRequest.subscribe(mBus, MessageBus::AttachAutomation, [&](AutomationAttachRequest r) {
      MessageBus::fireEvent(mBus, MessageBus::PushUndoState, false);
      WDL_PtrList<Node>& n = this->mNodes;
      for (int i = 0; i < n.GetSize(); i++) {
        Node* node = n.Get(i);
        if (node == nullptr) { continue; }
        for (int p = 0; p < node->shared.parameterCount; p++) {
          if (node->shared.parameters[p].control == r.targetControl) {
            if (node != r.automationNode) {
              // Don't allow automation on self
              node->attachAutomation(r.automationNode, p);
            }
          }
        }
      }
    });
  }

  void testadd() {
    return;
    // formatTest();
    Node* test = NodeList::createNode("CabLibNode");
    addNode(test, nullptr, 0, 500);
    // mOutputNode->connectInput(test->shared.socketsOut[0]);
  }

  ~Graph() {
    removeAllNodes();
    // TODOG get rid of all the things
  }

  void OnReset(const int pSampleRate, const int pOutputChannels = 2, const int pInputChannels = 2) {
    if (pSampleRate != mSampleRate || pOutputChannels != mChannelCount || pInputChannels != mInPutChannelCount) {
      lockAudioThread();
      mSampleRate = pSampleRate;
      resizeSliceBuffer(pOutputChannels);
      mChannelCount = pOutputChannels;
      mInPutChannelCount = pInputChannels;
      mInputNode->setInputChannels(pInputChannels);
      mInputNode->OnReset(pSampleRate, pOutputChannels);
      mOutputNode->OnReset(pSampleRate, pOutputChannels);
      for (int i = 0; i < mNodes.GetSize(); i++) {
        mNodes.Get(i)->OnReset(pSampleRate, pOutputChannels);
      }
      unlockAudioThread();
    }
    else {
      /**
       * If nothing has changed we'll assume a transport
       */
      mInputNode->OnTransport();
      mOutputNode->OnTransport();
      for (int i = 0; i < mNodes.GetSize(); i++) {
        mNodes.Get(i)->OnTransport();
      }
    }
  }

  /**
   * Main entry point for the DSP
   */
  void ProcessBlock(iplug::sample** in, iplug::sample** out, const int nFrames) {
    /**
     * Process the block in smaller bits since it's too large
     * Also abused to lower the delay a feedback node creates
     */
    if (nFrames > mMaxBlockSize) {
      const int overhang = nFrames % mMaxBlockSize;
      int s = 0;
      while (true) {
        for (int c = 0; c < mChannelCount; c++) {
          mSliceBuffer[0][c] = &in[c][s];
          mSliceBuffer[1][c] = &out[c][s];
        }
        s += mMaxBlockSize;
        if (s <= nFrames) {
          ProcessBlock(mSliceBuffer[0], mSliceBuffer[1], mMaxBlockSize);
        }
        else {
          if (overhang > 0) {
            ProcessBlock(mSliceBuffer[0], mSliceBuffer[1], overhang);
          }
          return;
        }
      }
    }


    const int nodeCount = mNodes.GetSize();
    const int maxAttempts = 10;

    /**
     * Do a version without mutex and stats if there's no gui
     * since there is no need for locking if the graph can't be altered
     */
    if (mGraphics == nullptr) {

      mInputNode->CopyIn(in, nFrames);
      for (int n = 0; n < nodeCount; n++) {
        mNodes.Get(n)->BlockStart();
      }
      mOutputNode->BlockStart();

      // The List is pre sorted so the attempts are only needed to catch circular dependencies and other edge cases
      
      int attempts = 0;
      while (!mOutputNode->mIsProcessed && attempts < maxAttempts) {
        for (int n = 0; n < nodeCount; n++) {
          mNodes.Get(n)->ProcessBlock(nFrames);
        }
        mOutputNode->ProcessBlock(nFrames);
        attempts++;
      }

      if (attempts < maxAttempts) {
        for (int n = 0; n < nodeCount; n++) {
          mNodes.Get(n)->ProcessBlock(nFrames);
        }
      }
      mOutputNode->CopyOut(out, nFrames);
    }
    else {
      /**
       * The version with mutex locking
       */
      if (mPauseAudio > 0) {
        /**
         * Skip the block if the mutex is locked, waiting will most likely result in an under-run anyways
         */
        for (int c = 0; c < mChannelCount; c++) {
          for (int i = 0; i < nFrames; i++) {
            out[c][i] = 0;
          }
        }
        return;
      }

      const auto start = std::chrono::high_resolution_clock::now();
      WDL_MutexLock lock(&mAudioMutex);
      mInputNode->CopyIn(in, nFrames);
      for (int n = 0; n < nodeCount; n++) {
        mNodes.Get(n)->BlockStart();
      }
      mOutputNode->BlockStart();
      // The List is pre sorted so the attempts are only needed to catch circular dependencies and other edge cases
      int attempts = 0;
      while (!mOutputNode->mIsProcessed && attempts < maxAttempts) {
        for (int n = 0; n < nodeCount; n++) {
          mNodes.Get(n)->ProcessBlock(nFrames);
        }
        mOutputNode->ProcessBlock(nFrames);
        attempts++;
      }

      // This extra iteration makes sure the feedback loops get data from their previous nodes
      if (attempts < maxAttempts) {
        for (int n = 0; n < nodeCount; n++) {
          mNodes.Get(n)->ProcessBlock(nFrames);
        }
        if (!mStats.valid) {
          mStats.valid = true;
          MessageBus::fireEvent(mBus, MessageBus::GraphStatsChanged, &mStats);
        }
      }
      else {
        // failed processing
        if (mStats.valid) {
          mStats.valid = false;
          MessageBus::fireEvent(mBus, MessageBus::GraphStatsChanged, &mStats);
        }
      }

      mOutputNode->CopyOut(out, nFrames);
      const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now() - start
        );
      mStats.executionTime = duration.count();
    }
  }

  /**
   * The graph needs to know about the graphics context to add and remove the controls for the nodes
   * It also handles keystrokes globally
   */
  void setupUi(iplug::igraphics::IGraphics* pGraphics = nullptr) {
    if (pGraphics != nullptr && pGraphics != mGraphics) {
      WDBGMSG("Graphics context changed");
      mGraphics = pGraphics;
    }
    pGraphics->AttachCornerResizer(EUIResizerMode::Size, true);
    pGraphics->HandleMouseOver(true);
    pGraphics->AttachTextEntryControl();
    pGraphics->AttachPopupMenuControl(DEFAULT_LABEL_TEXT);
    
    mGraphics->SetKeyHandlerFunc([&](const IKeyPress & key, const bool isUp) {
      // Gets the keystrokes in the standalone app
      if (key.S && (key.VK == kVK_Z) && !isUp) {
        MessageBus::fireEvent<bool>(this->mBus, MessageBus::PopUndoState, false);
        return true;
      }
      if ((key.VK == kVK_F) && !isUp) {
        arrangeNodes();
        return true;
      }
      if ((key.VK == kVK_C) && !isUp) {
        centerGraph();
        return true;
      }
      if ((key.VK == kVK_Q) && !isUp) {
        centerNode(mInputNode);
        return true;
      }
      if ((key.VK == kVK_E) && !isUp) {
        centerNode(mOutputNode);
        return true;
      }
      if ((key.VK == kVK_S) && !isUp) {
        this->lockAudioThread();
        SortGraph::sortGraph(mNodes, mInputNode, mOutputNode);
        this->unlockAudioThread();
        return true;
      }
      return false;
    });

    mBackground = new GraphBackground(mBus, mGraphics, [&](float x, float y, float scale) {
      this->onViewPortChange(x, y, scale);
    });
    mGraphics->AttachControl(mBackground);

    for (int n = 0; n < mNodes.GetSize(); n++) {
        mNodes.Get(n)->setupUi(mGraphics);
    }
    mInputNode->setupUi(mGraphics);
    mOutputNode->setupUi(mGraphics);

    mCableLayer = new CableLayer(mBus, mGraphics, &mNodes, mOutputNode, mInputNode);
    mCableLayer->setRenderPriority(10);
    mGraphics->AttachControl(mCableLayer);

    mNodeGallery = new NodeGallery(mBus, mGraphics);
    mGraphics->AttachControl(mNodeGallery);

    scaleUi();
#ifndef NDEBUG
    testadd();
#endif
  }

  /**
   * Updates the scale in the background layer and scales the UI according to
   * mWindowWidth, mWindowHeight, mWindowScale
   */
  void scaleUi() const {
    if (mWindowWidth != 0 && mWindowHeight != 0 && mWindowScale != 0 && mGraphics != nullptr) {
      mBackground->mScale = mWindowScale;
      mGraphics->Resize(mWindowWidth, mWindowHeight, mWindowScale);
    }
  }

  void cleanupUi() {
    mWindowWidth = mGraphics->Width();
    mWindowHeight = mGraphics->Height();
    mWindowScale = mGraphics->GetDrawScale();
    for (int n = 0; n < mNodes.GetSize(); n++) {
      mNodes.Get(n)->cleanupUi(mGraphics);
    }

    mGraphics->RemoveControl(mNodeGallery, true);
    mNodeGallery = nullptr;

    mGraphics->RemoveControl(mBackground, true);
    mBackground = nullptr;

    mGraphics->RemoveControl(mCableLayer, true);
    mCableLayer = nullptr;

    mInputNode->cleanupUi(mGraphics);
    mOutputNode->cleanupUi(mGraphics);
    mGraphics->RemoveAllControls();
    mGraphics = nullptr;
  }

  /**
   * Called via a callback from the background to move around all the nodes
   * creating the illusion of a viewport
   */
  void onViewPortChange(const float dX = 0, const float dY = 0, float scale = 1) const {
    for (int i = 0; i < mNodes.GetSize(); i++) {
      mNodes.Get(i)->mUi->translate(dX, dY);
    }
    mOutputNode->mUi->translate(dX, dY);
    mInputNode->mUi->translate(dX, dY);
    // WDBGMSG("x %f y %f s %f\n", x, y, scale);
  }

  /**
   * Centers the viewport around a specific node
   */
  void centerNode(Node* node) const {
    IRECT center = mGraphics->GetBounds().GetScaledAboutCentre(0);
    center.L -= node->shared.X;
    center.T -= node->shared.Y;
    onViewPortChange(center.L, center.T);
  }

  /**
   * Averages all node positions and moves the viewport to that point
   * Bound to the C key
   */
  void centerGraph() const {
    Coord2D avg{ 0, 0 };
    const int count = mNodes.GetSize();
    for (int i = 0; i < count; i++) {
      const Node* n = mNodes.Get(i);
      avg.x += n->shared.X;
      avg.y += n->shared.Y;
    }
    float countf = count + 2;
    avg.x += mInputNode->shared.X + mOutputNode->shared.X;
    avg.y += mInputNode->shared.Y + mOutputNode->shared.Y;
    // We want that point to be in the center of the screen
    const IRECT center = mGraphics->GetBounds().GetScaledAboutCentre(0);
    avg.x = center.L - avg.x / countf;
    avg.y = center.T - avg.y / countf;
    onViewPortChange(avg.x, avg.y);
  }

  void layoutUi(iplug::igraphics::IGraphics* pGraphics = nullptr) {
    if (pGraphics != nullptr && pGraphics != mGraphics) {
      WDBGMSG("Graphics context changed");
      mGraphics = pGraphics;
      // TODOG find out whether the context ever changes
    }
  }

  /**
   * Used to add nodes and pause the audio thread
   */
  void addNode(Node* node, Node* pInput = nullptr, const float x = 0, const float y = 0, const int outputIndex = 0, const int inputIndex = 0, Node* clone = nullptr) {
    node->shared.X = x;
    node->shared.Y = y;
    node->setup(mBus, mSampleRate);
    if (clone != nullptr) {
      node->copyState(clone);
    }
    mParamManager.claimNode(node);
    node->setupUi(mGraphics);
    if (pInput != nullptr) {
      node->connectInput(pInput->shared.socketsOut[outputIndex], inputIndex);
    }
    // Allocating the node is thread safe, but not the node list itself
    lockAudioThread();
    mNodes.Add(node);
    SortGraph::sortGraph(mNodes, mInputNode, mOutputNode);
    // mMaxBlockSize = hasFeedBackNode() ? MIN_BLOCK_SIZE : MAX_BUFFER;
    unlockAudioThread();
  }

  void removeAllNodes() {
    while (mNodes.GetSize()) {
      removeNode(0);
    }
  }

  /**
   * Will re route the connections the node provided
   * Only takes care of the first input and first output
   */
  void byPassConnection(Node* node) const {
    if (node->shared.inputCount > 0 && node->shared.outputCount > 0) {
      NodeSocket* prevSock = node->shared.socketsIn[0];
      NodeSocket* nextSock = node->shared.socketsOut[0];
      if (prevSock != nullptr && prevSock->mConnectedTo[0] != nullptr && nextSock != nullptr) {
        for (int i = 0; i < MAX_SOCKET_CONNECTIONS; i++) {
          if (nextSock->mConnectedTo[i] != nullptr) {
            MessageBus::fireEvent<SocketConnectRequest>(mBus,
              MessageBus::SocketRedirectConnection,
              SocketConnectRequest{
                prevSock->mConnectedTo[0],
                nextSock->mConnectedTo[i]
              }
            );
          }
        }
        MessageBus::fireEvent<Node*>(mBus, MessageBus::NodeDisconnectAll, node);
      }
    }
  }

  void spliceInCombine(Node* node) {
    if (node->shared.inputCount > 0 && node->shared.outputCount > 0) {
      NodeSocket* inSock = node->shared.socketsIn[0];
      NodeSocket* outSock = node->shared.socketsOut[0];
      NodeSocket* source = inSock->mConnectedTo[0];
      NodeSocket* target = outSock->mConnectedTo[0];
      if (target == nullptr || source == nullptr) {
        return;
      }
      Node* combine = NodeList::createNode("CombineNode");
      addNode(combine, nullptr, node->shared.X, node->shared.Y);

      for (int i = 0; i < MAX_SOCKET_CONNECTIONS; i++) {
        if (outSock->mConnectedTo[i] != nullptr) {
          combine->shared.socketsOut[0]->connect(outSock->mConnectedTo[i]);
        }
      }
      combine->shared.socketsIn[0]->connect(outSock);
      combine->shared.socketsIn[1]->connect(source);
      combine->mUi->mDragging = true;
      mGraphics->SetCapturedControl(combine->mUi);
    }
  }

  /**
   * Removes the node and pauses the audio thread
   * Can also bridge the connection if possible
   */
  void removeNode(Node* node, const bool reconnect = false) {
    if (node == mInputNode || node == mOutputNode) { return; }
    lockAudioThread();
    if (reconnect) {
      /**
       * Since the cleanup will sever all connections to a node, it will have to be done before the
       * connection is bridged, or else the bridged connection will be severed again
       */
      byPassConnection(node);
    }
    node->cleanupUi(mGraphics);
    mParamManager.releaseNode(node);
    node->cleanUp();
    mNodes.DeletePtr(node, true);
    SortGraph::sortGraph(mNodes, mInputNode, mOutputNode);
    // mMaxBlockSize = hasFeedBackNode() ? MIN_BLOCK_SIZE : MAX_BUFFER;
    unlockAudioThread();
  }

  void removeNode(const int index) {
    removeNode(mNodes.Get(index));
  }

  void serialize(nlohmann::json& json) {
    if (mGraphics != nullptr) {
      mWindowWidth = mGraphics->Width();
      mWindowHeight = mGraphics->Height();
      mWindowScale = mGraphics->GetDrawScale();
    }
    json["scale"] = mWindowScale;
    json["width"] = mWindowWidth;
    json["height"] = mWindowHeight;
    Serializer::serialize(json, mNodes, mInputNode, mOutputNode);
  }

  void deserialize(nlohmann::json& json) {
    removeAllNodes();
    if (mGraphics != nullptr) {
      IGraphics* g = mGraphics;
      cleanupUi();
      setupUi(g);
    }
    if (json.contains("width")) {
      mWindowScale = json["scale"];
      mWindowWidth = json["width"];
      mWindowHeight = json["height"];
    }
    lockAudioThread();
    Serializer::deserialize(json, mNodes, mOutputNode, mInputNode, mSampleRate, &mParamManager, mBus);
    if (mGraphics != nullptr && mGraphics->WindowIsOpen()) {
      for (int i = 0; i < mNodes.GetSize(); i++) {
        mNodes.Get(i)->setupUi(mGraphics);
      }
      scaleUi();
    }
    unlockAudioThread();
  }

private:
  void resizeSliceBuffer(const int channelCount) {
    if (mSliceBuffer[0] != nullptr) {
      for (int c = 0; c < channelCount; c++) {
        delete mSliceBuffer[0];
        mSliceBuffer[0] = nullptr;
        delete mSliceBuffer[1];
        mSliceBuffer[1] = nullptr;
      }
    }
    mSliceBuffer[0] = new sample * [channelCount];
    mSliceBuffer[1] = new sample * [channelCount];
  }

  void lockAudioThread() {
    if (mPauseAudio == 0) {
      mAudioMutex.Enter();
    }
    mPauseAudio++;
  }

  void unlockAudioThread() {
    if (mPauseAudio == 1) {
      mAudioMutex.Leave();
    }
    mPauseAudio--;
  }

  /**
   * Will try to tidy up the node graph, bound to the F key
   */
  void arrangeNodes() const {
    FormatGraph::resetBranchPos(mInputNode);
    FormatGraph::arrangeBranch(mInputNode, Coord2D { mInputNode->shared.Y, mInputNode->shared.X });
    centerGraph();
  }

  bool hasFeedBackNode() const {
    for (int i = 0; i < mNodes.GetSize(); i++) {
      if (mNodes.Get(i)->shared.type == "FeedbackNode") {
        return true;
      }
    }
    return false;
  }



  /**
   * Test Setups
   */

  void formatTest() {
    /**
     *                            ------------
     *                            |          |
     *                --> test2 --|          |--> test5 --
     *                |           |          |           |
     *  in -> test1 --|           --> test4 --           |--> test7 --> out
     *                |                                  |
     *                --> test3 ----> test6 --------------
     */
    Node* test1 = NodeList::createNode("StereoToolNode");
    addNode(test1, mInputNode, 200, 0);
    Node* test2 = NodeList::createNode("StereoToolNode");
    addNode(test2, test1, 400, -100);
    Node* test3 = NodeList::createNode("StereoToolNode");
    addNode(test3, test1, 400, +100);
    Node* test4 = NodeList::createNode("StereoToolNode");
    addNode(test4, test2, 600, 0);
    Node* test5 = NodeList::createNode("CombineNode");
    addNode(test5, test2, 800, +100);
    test5->connectInput(test4->shared.socketsOut[0], 1);
    Node* test6 = NodeList::createNode("StereoToolNode");
    addNode(test6, test3, 400, +100);
    Node* test7 = NodeList::createNode("CombineNode");
    addNode(test7, test5, 1000, 0);
    test7->connectInput(test6->shared.socketsOut[0], 1);
    mOutputNode->shared.socketsIn[0]->disconnectAll();
    mOutputNode->connectInput(test7->shared.socketsOut[0]);
  }

};
