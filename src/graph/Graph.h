#pragma once
#include <chrono>
#include "../../thirdparty/soundwoofer/soundwoofer.h"
#include "../misc/constants.h"
#include "../types/gmutex.h"
#include "../types/types.h"
#include "../types/pointerList.h"
#include "../nodes/io/InputNode.h"
#include "../nodes/io/OutputNode.h"
#include "../parameter/ParameterManager.h"

namespace guitard {
  /**
   * A object of this class will hold a bunch of Nodes and can process them
   */
  class Graph {
    ParameterManager* mParamManager = nullptr;

    /**
     * Holds all the nodes in the processing graph
     */
    PointerList<Node> mNodes;

    /**
     * List used to process the nodes in the right order
     */
    PointerList<Node> mProcessList;

    /**
     * Mutex to keep changes to the graph like adding/removing or rerouting from crashing
     */
    Mutex mAudioMutex;

    /**
     * Acts as a semaphore since the mAudioMutex only needs to be locked once to stop the audio thread
     */
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

    /**
     * The max blockSize which can be used to minimize round trip delay when having cycles in the graph
     */
    int mMaxBlockSize = GUITARD_MAX_BUFFER;


    /**
     * Used to slice the dsp block in smaller slices
     * Only does stereo for now
     */
    sample** mSliceBuffer[2] = { nullptr };

    /**
     * This is the zoom level of the graph, it's here since the gui
     * may be able to handle several graphs at some point
     */
    float mScale = 1.0;

    /**
     * True if a thread is in the ProcessBlock right now
     */
    bool mIsProcessing = false;

  public:
    GraphStats mStats;

    explicit Graph(ParameterManager* pParamManager) {
      mParamManager = pParamManager; // we'll keep this around to let nodes register parameters

      mInputNode = new InputNode();
      mOutputNode = new OutputNode();
      mOutputNode->connectInput(mInputNode->mSocketsOut[0]);
    }

    ~Graph() {
      removeAllNodes();
      // TODOG get rid of all the things
    }

    void lockAudioThread() {
      mPauseAudio++;
      while (mIsProcessing) { } // wait till the audio thread is done
      //if (mPauseAudio == 0) {
      //  mAudioMutex.lock();
      //}
    }

    void unlockAudioThread() {
      //if (mPauseAudio == 1) {
      //  mAudioMutex.unlock();
      //}
      mPauseAudio--;
    }

    /**
     * Called from outside to update samplerate, an channel configs
     */
    void OnReset(const int pSampleRate, const int pOutputChannels = 2, const int pInputChannels = 2) {
      if (pSampleRate != mSampleRate || pOutputChannels != mChannelCount || pInputChannels != mInPutChannelCount) {
        lockAudioThread();
        mSampleRate = pSampleRate;
        {
          if (mSliceBuffer[0] != nullptr) {
            for (int c = 0; c < pOutputChannels; c++) {
              delete mSliceBuffer[0];
              mSliceBuffer[0] = nullptr;
              delete mSliceBuffer[1];
              mSliceBuffer[1] = nullptr;
            }
          }
          mSliceBuffer[0] = new sample * [pOutputChannels];
          mSliceBuffer[1] = new sample * [pOutputChannels];
          /**
           * There's no need to create a buffer inside since it will use the pointer from the
           * scratch buffer offset by the sub-block position
           */
        }
        mChannelCount = pOutputChannels;
        mInPutChannelCount = pInputChannels;
        mInputNode->setInputChannels(pInputChannels);
        mInputNode->OnReset(pSampleRate, pOutputChannels);
        mOutputNode->OnReset(pSampleRate, pOutputChannels);
        for (int i = 0; i < mNodes.size(); i++) {
          mNodes[i]->OnReset(pSampleRate, pOutputChannels);
        }
        unlockAudioThread();
      }
      else {
        /**
         * If nothing has changed we'll assume a transport
         */
        OnTransport();
      }
    }

    /**
     * Will clear all the DSP buffers to kill reverbs and so on
     */
    void OnTransport() {
      lockAudioThread();
      mInputNode->OnTransport();
      mOutputNode->OnTransport();
      for (size_t i = 0; i < mNodes.size(); i++) {
        mNodes[i]->OnTransport();
      }
      unlockAudioThread();
    }

    /**
     * Will set the max blocksize to change roundtrip delay inside the graph
     */
    void setBlockSize(const int size) {
      if (size == mMaxBlockSize || size > GUITARD_MAX_BUFFER) { return; }
      lockAudioThread();
      mMaxBlockSize = size;
      for (int i = 0; i < mNodes.size(); i++) {
        Node* n = mNodes[i];
        n->mMaxBlockSize = size;
        n->OnReset(mSampleRate, mChannelCount, true);
      }
      unlockAudioThread();
    }

    /**
     * Main entry point for the DSP
     */
    void ProcessBlock(sample** in, sample** out, const int nFrames) {
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

      const int nodeCount = mProcessList.size();

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

      // LockGuard lock(mAudioMutex); // TODOG might not need a lock if the gui has to wait
      mIsProcessing = true;

      mInputNode->CopyIn(in, nFrames);

      for (int n = 0; n < nodeCount; n++) {
        mProcessList[n]->BlockStart();
      }
      mOutputNode->BlockStart();

      for (int n = 0; n < nodeCount; n++) {
        mNodes[n]->ProcessBlock(nFrames);
      }
      mOutputNode->ProcessBlock(nFrames);

      mStats.valid = mOutputNode->mIsProcessed;

      mOutputNode->CopyOut(out, nFrames);

      mIsProcessing = false;
    }

    /**
     * Used to add nodes and pause the audio thread
     */
    void addNode(
      Node* node, Node* pInput = nullptr, const Coord2D pos = {0, 0},
        const int outputIndex = 0, const int inputIndex = 0, Node* clone = nullptr
    ) {
      if (mNodes.find(node) != -1) {
        assert(false); // In case node is already in the list
        return;
      }
      node->mPos = pos;
      node->setup(mSampleRate, mMaxBlockSize);

      if (clone != nullptr) {
        node->copyState(clone);
      }

      if (mParamManager != nullptr) {
        mParamManager->claimNode(node);
      }

      if (pInput != nullptr) {
        node->connectInput(pInput->mSocketsOut[outputIndex], inputIndex);
      }

      mNodes.add(node);

      sortGraph();
    }

    /**
     * Will re route the connections the node provided
     * Only takes care of the first input and first output
     */
    void byPassConnection(Node* node) const {
      if (node->mInputCount > 0 && node->mOutputCount > 0) {
        NodeSocket* inSock = node->mSocketsIn[0];
        NodeSocket* outSock = node->mSocketsOut[0];
        NodeSocket* prevSock = inSock->mConnectedTo[0];
        if (prevSock != nullptr) { // make sure there's a previous node
          int nextSocketCount = 0;
          NodeSocket* nextSockets[MAX_SOCKET_CONNECTIONS] = { nullptr };
          for (int i = 0; i < MAX_SOCKET_CONNECTIONS; i++) { // Gather all the sockets connected to the output of this node
            NodeSocket* nextSock = outSock->mConnectedTo[i];
            if (nextSock != nullptr) {
              bool duplicate = false;
              for (int j = 0; j < nextSocketCount; j++) {
                if (nextSockets[j] == nextSock) {
                  duplicate = true;
                }
              }
              if (!duplicate) {
              nextSockets[nextSocketCount] = nextSock;
              nextSocketCount++;
            }
          }
          }

          outSock->disconnectAll();
          inSock->disconnectAll();

          for (int i = 0; i < nextSocketCount; i++) {
            prevSock->connect(nextSockets[i]);
          }
        }
      }
    }

    /**
     * Adds a combine node after provided node to act as a dry wet mix
     * and returns a pointer to the new combine
     */
    Node* spliceInCombine(Node* node) {
      if (node->mInputCount > 0 && node->mOutputCount > 0) {
        NodeSocket* inSock = node->mSocketsIn[0];
        NodeSocket* outSock = node->mSocketsOut[0];
        NodeSocket* source = inSock->mConnectedTo[0];
        NodeSocket* target = outSock->mConnectedTo[0];
        if (target == nullptr || source == nullptr) {
          return nullptr;
        }
        Node* combine = NodeList::createNode("CombineNode");
        addNode(combine, nullptr, node->mPos);

        for (int i = 0; i < MAX_SOCKET_CONNECTIONS; i++) {
          if (outSock->mConnectedTo[i] != nullptr) {
            combine->mSocketsOut[0]->connect(outSock->mConnectedTo[i]);
          }
        }
        combine->mSocketsIn[0]->connect(outSock);
        combine->mSocketsIn[1]->connect(source);
        return combine;
      }
      return nullptr;
    }

    void removeAllNodes() {
      while (mNodes.size()) {
        removeNode(0);
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
        byPassConnection(node);
      }
      sortGraph();

      // unlockAudioThread(); TODOG we should be able to unlock here

      mNodes.remove(node);
      if (mNodes.find(node) != -1) {
        assert(false);
      }

      if (mParamManager != nullptr) {
        mParamManager->releaseNode(node);
      }
      
      node->cleanUp();

      delete node;
      unlockAudioThread();
    }

    void removeNode(const int index) {
      removeNode(mNodes[index]);
    }

#ifndef GUITARD_HEADLESS
    // TODO this should be handled differently
    void serialize(WDL_String& serialized) {
      nlohmann::json json;
      serialize(json);
      try {
        serialized.Set(json.dump(4).c_str());
      }
      catch (...) {
        assert(false); // Failed to dump json
      }
    }
#endif

    void serialize(nlohmann::json& json) {
      const int NoNode = -2;
      const int InNode = -1;
      try {
        json = { { "version", "PLUG_VERSION_HEX" } };
        json["maxBlockSize"] = mMaxBlockSize;

        json["input"]["gain"] = 1.0;
        json["input"]["position"] = {
          mInputNode->mPos.x, mInputNode->mPos.y
        };

        json["nodes"] = nlohmann::json::array();
        for (int i = 0; i < mNodes.size(); i++) {
          Node* node = mNodes[i];
          json["nodes"][i]["position"] = { node->mPos.x, node->mPos.y };
          // The index shouldn't really matter since they're all in order
          json["nodes"][i]["idx"] = i;
          json["nodes"][i]["type"] = node->mInfo->name;
          json["nodes"][i]["inputs"] = nlohmann::json::array();
          for (int prev = 0; prev < node->mInputCount; prev++) {
            Node* cNode = node->mSocketsIn[prev]->getConnectedNode();
            if (cNode == nullptr) {
              json["nodes"][i]["inputs"][prev] = { NoNode, 0 };
            }
            else if (cNode == mInputNode) {
              json["nodes"][i]["inputs"][prev] = { InNode, 0 };
            }
            else {
              json["nodes"][i]["inputs"][prev] = {
                mNodes.find(cNode),
                node->mSocketsIn[prev]->getConnectedSocketIndex()
              };
            }
          }
          json["nodes"][i]["parameters"] = nlohmann::json::array();
          for (int p = 0; p < node->mParameterCount; p++) {
            ParameterCoupling* para = &node->mParameters[p];
            const char* name = para->name;
            double val = para->getValue();
            int idx = para->parameterIdx;
            int automation = mNodes.find(para->automationDependency);
            json["nodes"][i]["parameters"][p] = {
              { "name", name },
              { "value", val }
            };
            if (automation >= 0) {
              // Only write the automation value in here if there's one
              json["nodes"][i]["parameters"][p]["automation"] = automation;
          }
            if (idx >= 0) {
              // only write an parameter index if there was one assigned
              json["nodes"][i]["parameters"][p]["idx"] = idx;
          }
          }
          node->serializeAdditional(json["nodes"][i]);
        }

        // Handle the output node
        json["output"]["gain"] = 1.0;
        json["output"]["position"] = { mOutputNode->mPos.x, mOutputNode->mPos.y };
        Node* lastNode = mOutputNode->mSocketsIn[0]->getConnectedNode();
        int lastNodeIndex = NoNode;
        if (lastNode == mInputNode) {
          lastNodeIndex = InNode;
        }
        else if (lastNode != nullptr) {
          lastNodeIndex = mNodes.find(lastNode);
        }

        json["output"]["inputs"][0] = {
          lastNodeIndex,
          mOutputNode->mSocketsIn[0]->getConnectedSocketIndex()
        };

      }
      catch (...) {
        assert(false); // Failed to serialize json
      }
    }

    void deserialize(const char* data) {
      try {
        nlohmann::json json = nlohmann::json::parse(data);
        deserialize(json);
      }
      catch (...) {
        return;
        // assert(false); // Failed to parse JSON
      }
    }

    void deserialize(nlohmann::json& json) {
      try {
        const int NoNode = -2;
        const int InNode = -1;

        lockAudioThread();

        removeAllNodes();

        mInputNode->mPos.x = json["input"]["position"][0];
        mInputNode->mPos.y = json["input"]["position"][1];

        if (json.contains("maxBlockSize")) {
          mMaxBlockSize = json["maxBlockSize"];
        }
        mOutputNode->connectInput(nullptr, 0);

        int expectedIndex = 0;
        int paramBack = MAX_DAW_PARAMS - 1;

        // create all the nodes and setup the parameters in the first pass
        for (auto sNode : json["nodes"]) {
          const std::string className = sNode["type"];
          Node* node = NodeList::createNode(className);
          if (node == nullptr) { continue; } // we might not actually be able to provide a node with the name
          if (expectedIndex != sNode["idx"]) {
            WDBGMSG("Deserialization mismatched indexes, this will not load right\n");
          }

          addNode(node, nullptr, { sNode["position"][0], sNode["position"][1] });

          // Set the parameter values
          for (auto param : sNode["parameters"]) {
            std::string name = param["name"];
            int found = 0;
            for (int i = 0; i < node->mParameterCount; i++) {
              ParameterCoupling* para = &node->mParameters[i];
              if (para->name == name) {
                found++;
                if (param.contains("idk")) {
                para->parameterIdx = param["idx"];
                  para->wantsDawParameter = true;
                }
                else {
                  para->wantsDawParameter = false;
                }
                sample val = param["value"];
                para->setValue(val);
              }
            }

            for (int i = 0; i < node->mParameterCount; i++) {
              ParameterCoupling* para = &node->mParameters[i];
              if (para->parameterIdx == -1) {
                /**
                 * Rare case that happens when a node has more parameters in the current version of the plugin
                 * In order to not steal a parameter from the following nodes we need to assign it from the back
                 */
                para->parameterIdx = paramBack;
                paramBack--;
              }
            }
          }
          if (mParamManager != nullptr) {
            mParamManager->claimNode(node);
          }
          expectedIndex++;
        }

        // link the nodes all up accordingly in the second pass
        int currentNodeIdx = 0;
        for (auto sNode : json["nodes"]) {
          Node* node = mNodes[currentNodeIdx];
          if (node == nullptr) { continue; }
          int currentInputIdx = 0;
          for (auto connection : sNode["inputs"]) {
            const int inNodeIdx = connection[0];
            const int inBufferIdx = connection[1];
            if (inNodeIdx >= 0 && mNodes[inNodeIdx] != nullptr) {
              node->connectInput(
                mNodes[inNodeIdx]->mSocketsOut[inBufferIdx],
                currentInputIdx
              );
            }
            else if (inNodeIdx == InNode) {
              // if it's NoNode it's not connected at all and we'll just leave it at a nullptr
              node->connectInput(
                mInputNode->mSocketsOut[0],
                currentInputIdx
              );
            }
            currentInputIdx++;
          }

          // Link up the automation
          for (auto param : sNode["parameters"]) {
            std::string name = param["name"];
            for (int i = 0; i < node->mParameterCount; i++) {
              ParameterCoupling* para = &node->mParameters[i];
              if (para->name == name) {
                if (param.contains("automation")) {
                  const int automationIndex = param.at("automation");
                  if (automationIndex >= 0) {
                    node->attachAutomation(mNodes[automationIndex], i);
                  }
                }
              }
            }
          }

          // pass the additional info to the node
          node->deserializeAdditional(sNode);
          currentNodeIdx++;
        }

        // connect the output nodes to the global output
        mOutputNode->mPos.x = json["output"]["position"][0];
        mOutputNode->mPos.y = json["output"]["position"][1];
        const int outNodeIndex = json["output"]["inputs"][0][0];
        const int outConnectionIndex = json["output"]["inputs"][0][1];
        if (mNodes[outNodeIndex] != nullptr) {
          mOutputNode->connectInput(
            mNodes[outNodeIndex]->mSocketsOut[outConnectionIndex]
          );
        }
        else if (outNodeIndex == InNode) {
          // In case the output goes straight to the input
          mOutputNode->connectInput(mInputNode->mSocketsOut[0]);
        }

        sortGraph();
        unlockAudioThread();
      }
      catch (...) {
        WDBGMSG("Failed loading preset!");
        // assert(false); // To load graph with json
      }
    }

    float getScale() const {
      return mScale;
    }

    void setScale(float scale) {
      mScale = scale;
    }

    PointerList<Node> getNodes() const {
      return mNodes;
    }

    Node* getInputNode() const {
      return mInputNode;
    }

    Node* getOutputNode() const {
      return mOutputNode;
    }

    void sortGraph() {
      lockAudioThread();
      mProcessList.clear();
      for (int i = 0; i < MAX_SOCKET_CONNECTIONS; i++) {
        // Put in the nodes which directly follow the input node
        NodeSocket* con = mInputNode->mSocketsOut[0]->mConnectedTo[i];
        if (con != nullptr) {
          mProcessList.add(con->mParentNode);
        }
      }

      for (int tries = 0; tries < mNodes.size(); tries++) {
        for (int i = 0; i < mProcessList.size(); i++) {
          Node* node = mProcessList[i];
          for (int out = 0; out < node->mOutputCount; out++) {
            NodeSocket* outSocket = node->mSocketsOut[out];
            if (outSocket == nullptr) { continue; }
            for (int next = 0; next < MAX_SOCKET_CONNECTIONS; next++) {
              NodeSocket* nextSocket = outSocket->mConnectedTo[next];
              if (nextSocket == nullptr) { continue; }
              Node* nextNode = nextSocket->mParentNode;
              // Don't want to add duplicates or the output node
              if (mProcessList.find(nextNode) != -1) { continue; }
              mProcessList.add(nextNode);
            }
          }
        }
      }

      if (mProcessList.find(mOutputNode) == -1) {
        assert(false);
      }
      mProcessList.remove(mOutputNode);
      unlockAudioThread();
    }
  };
}
