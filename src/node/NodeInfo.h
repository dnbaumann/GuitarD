#pragma once
#include <functional>
#include <string>

namespace guitard {
  class Node;
  namespace NodeList {
    typedef std::function<Node * ()> NodeConstructor;

    struct NodeInfo {
      NodeConstructor constructor;
      std::string name;
      std::string displayName;
      std::string image;
      std::string categoryName;
    };
  }
}
