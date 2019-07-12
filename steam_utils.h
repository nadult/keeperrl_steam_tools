#pragma once

#include "steam_base.h"
#include <steam/isteamutils.h>

// TODO: should be inside namespace
RICH_ENUM(CallResultStatus, pending, completed, failed);

namespace steam {

// TODO: cleanup
struct CallResultBase {
  using Status = CallResultStatus;
  void update(Utils&, void*, int, int);

  unsigned long long handle = 0;
  Status status = Status::pending;
  ESteamAPICallFailure failure = k_ESteamAPICallFailureNone;
};

template <class T> class CallResult : public CallResultBase {
  public:
  CallResult(unsigned long long handle) {
    this->handle = handle;
  }

  void update(Utils& utils) {
    CallResultBase::update(utils, &result_, sizeof(result_), T::k_iCallback);
  }

  bool isCompleted() const {
    return status == Status::completed;
  }
  bool isPending() const {
    return status == Status::pending;
  }

  const T& result() const {
    CHECK(status == Status::completed);
    return result_;
  }

  private:
  T result_;
};

class Utils {
  public:
  // TODO: return expected ?
  pair<int, int> imageSize(int image_id) const;
  vector<uint8> imageData(int image_id) const;

  unsigned appId() const;

  void updateCallResult(CallResultBase&, void* data, int data_size, int ident);

  private:
  Utils(intptr_t);
  friend class Client;
  friend struct CallResultBase; //TODO: clenaup

  intptr_t m_ptr;
};
}
