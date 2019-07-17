#pragma once

#include "steam_base.h"
#include <steam/isteamutils.h>

namespace steam {

// TODO: cleanup
struct CallResultBase {
  using Status = QueryStatus;

  void update(void*, int, int);
  const char* failText() const;

  unsigned long long handle = 0;
  Status status = Status::invalid;
  ESteamAPICallFailure failure = k_ESteamAPICallFailureNone;
};

template <class T> class CallResult : public CallResultBase {
  public:
  CallResult(unsigned long long handle) {
    this->handle = handle;
    status = Status::pending;
    if (handle == k_uAPICallInvalid) {
      status = Status::failed;
      failure = k_ESteamAPICallFailureInvalidHandle;
    }
  }
  CallResult() {
  }

  void update() {
    CHECK(status != Status::invalid);
    CallResultBase::update(&result_, sizeof(result_), T::k_iCallback);
  }

  explicit operator bool() const {
    return status != Status::invalid;
  }
  void clear() {
    status = Status::invalid;
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
  STEAM_IFACE_DECL(Utils);
  friend struct CallResultBase;

  // TODO: return expected ?
  pair<int, int> imageSize(int image_id) const;
  vector<uint8> imageData(int image_id) const;

  unsigned appId() const;

  void updateCallResult(CallResultBase&, void* data, int data_size, int ident);
};
}
