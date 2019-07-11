#include "steam_internal.h"
#include "steam_client.h"

#include "steam_friends.h"
#include "steam_user.h"
#include "steam_ugc.h"
#include "steam_utils.h"

#include <cassert>

#define FUNC(name, ...) SteamAPI_ISteamClient_##name

namespace steam {

bool initAPI() {
  return SteamAPI_Init();
}

Client::Client() {
  // TODO: handle errors, use Expected<>
  m_ptr = (intptr_t)::SteamClient();
  m_pipe = FUNC(CreateSteamPipe)(m_ptr);
  m_user = FUNC(ConnectToGlobalUser)(m_ptr, m_pipe);
}
Client::~Client() {
  FUNC(ReleaseUser)(m_ptr, m_pipe, m_user);
  FUNC(BReleaseSteamPipe)(m_ptr, m_pipe);
}

Friends Client::friends() const {
  auto ptr = FUNC(GetISteamFriends)(m_ptr, m_user, m_pipe, STEAMFRIENDS_INTERFACE_VERSION);
  CHECK(ptr);
  return (intptr_t)ptr;
}

User Client::user() const {
  auto ptr = FUNC(GetISteamUser)(m_ptr, m_user, m_pipe, STEAMUSER_INTERFACE_VERSION);
  CHECK(ptr);
  return (intptr_t)ptr;
}

Utils Client::utils() const {
  auto ptr = FUNC(GetISteamUtils)(m_ptr, m_pipe, STEAMUTILS_INTERFACE_VERSION);
  CHECK(ptr);
  return (intptr_t)ptr;
}
UGC Client::ugc() const {
  auto ptr = FUNC(GetISteamUGC)(m_ptr, m_user, m_pipe, STEAMUGC_INTERFACE_VERSION);
  CHECK(ptr);
  return (intptr_t)ptr;
}
}
