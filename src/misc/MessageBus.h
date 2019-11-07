#pragma once
#include <map>
#include <functional>
#include <string>
#include <vector>

using namespace std;

namespace MessageBus {

  class BaseSubscription {
  public:
    virtual ~BaseSubscription() { };
  protected:
    bool subscribed;
    string mEventName;
  };

  typedef WDL_PtrList<BaseSubscription> SubsVector;

  map<string, SubsVector> subscriptions;

  template <class T>
  class Subscription : public BaseSubscription {
  public:
    function<void(T param)> mCallback;

    Subscription(string eventName, function<void(T param)> callback) {
      subscribed = false;
      subscribe(eventName, callback);
    }

    Subscription() {
      subscribed = false;
    }

    ~Subscription() {
      if (subscriptions.find(mEventName) != subscriptions.end()) {
        subscriptions.find(mEventName)->second.DeletePtr(this);
      }
    }

    void subscribe(string eventName, function<void(T param)> callback) {
      if (subscribed) {
        WDBGMSG("Trying to subscribe twice on the same Subscription!\n");
        return;
      }
      subscribed = true;
      mEventName = eventName;
      mCallback = callback;
      if (subscriptions.find(eventName) == subscriptions.end()) {
        SubsVector temp;
        temp.Add(this);
        subscriptions.insert(pair<string, SubsVector>(eventName, temp));
      }
      else {
        subscriptions.find(eventName)->second.Add(this);
      }
    }
  };

  template <class T>
  void fireEvent(string eventName, T param) {
    if (subscriptions.find(eventName) == subscriptions.end()) {
      WDBGMSG("Fired a event with not subscribers!\n");
      return;
    }
    SubsVector &subs = subscriptions.find(eventName)->second;
    for (int i = 0; i < subs.GetSize(); i++) {
      Subscription<T>* sub = dynamic_cast<Subscription<T>*>(subs.Get(i));
      if (sub != nullptr) {
        sub->mCallback(param);
      }
    }
  }
}