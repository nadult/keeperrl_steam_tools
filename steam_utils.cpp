#include "steam_internal.h"
#include "steam_utils.h"

#define FUNC(name, ...) SteamAPI_ISteamUtils_##name

namespace steam {

void CallResultBase::update(Utils& utils, void* data, int data_size, int ident) {
  using Status = QueryStatus;
  bool failed = false;
  if (status == Status::pending) {
    auto is_completed = FUNC(IsAPICallCompleted)(utils.m_ptr, handle, &failed);
    if (!failed && is_completed) {
      auto result = FUNC(GetAPICallResult)(utils.m_ptr, handle, data, data_size, ident, &failed);
      if (result && !failed)
        status = Status::completed;
    }

    if (failed) {
      status = Status::failed;
      failure = FUNC(GetAPICallFailureReason)(utils.m_ptr, handle);
    }
  }
}

static const pair<ESteamAPICallFailure, const char*> texts[] = {
    {k_ESteamAPICallFailureNone, ""},
    {k_ESteamAPICallFailureSteamGone, "API call failure: SteamGone"},
    {k_ESteamAPICallFailureNetworkFailure, "API call failure: network failure"},
    {k_ESteamAPICallFailureInvalidHandle, "API call failure: invalid handle"},
    {k_ESteamAPICallFailureMismatchedCallback, "API call failure: mismatched callback"}};

const char* CallResultBase::failText() const {
  for (auto& pair : texts)
    if (failure == pair.first)
      return pair.second;
  return "API call failure: unknown";
}

Utils::Utils(intptr_t ptr) : m_ptr(ptr) {
}

pair<int, int> Utils::imageSize(int image_id) const {
  uint32_t w = 0, h = 0;
  if (!FUNC(GetImageSize)(m_ptr, image_id, &w, &h))
    CHECK(false);
  return {w, h};
}

vector<uint8> Utils::imageData(int image_id) const {
  auto size = imageSize(image_id);
  vector<uint8> out(size.first * size.second * 4);
  if (!FUNC(GetImageRGBA)(m_ptr, image_id, out.data(), out.size()))
    CHECK(false);
  return out;
}
unsigned Utils::appId() const {
  return FUNC(GetAppID)(m_ptr);
}
}
