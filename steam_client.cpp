#include "steam_internal.h"
#include "steam_client.h"

#include "steam_friends.h"
#include "steam_user.h"
#include "steam_ugc.h"
#include "steam_utils.h"

#include <cassert>

#define FUNC(name) SteamAPI_ISteamClient_##name
#define US_FUNC(name) SteamAPI_ISteamUserStats_##name

namespace steam {

bool initAPI() {
  return SteamAPI_Init();
}

void runCallbacks() {
  SteamAPI_RunCallbacks();
}

Client::Client() {
  // TODO: handle errors, use Expected<>
  m_ptr = (intptr_t)::SteamClient();
  m_pipe = FUNC(CreateSteamPipe)(m_ptr);
  m_user = FUNC(ConnectToGlobalUser)(m_ptr, m_pipe);

  m_user_stats = (intptr_t)FUNC(GetISteamUserStats)(m_ptr, m_user, m_pipe, STEAMUSERSTATS_INTERFACE_VERSION);
  m_utils = (intptr_t)FUNC(GetISteamUtils)(m_ptr, m_pipe, STEAMUTILS_INTERFACE_VERSION);
}
Client::~Client() {
  m_ugc.reset();
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

UGC& Client::ugc() {
  if (!m_ugc) {
    auto ptr = FUNC(GetISteamUGC)(m_ptr, m_user, m_pipe, STEAMUGC_INTERFACE_VERSION);
    CHECK(ptr);
    m_ugc.reset(new UGC((intptr_t)ptr));
  }
  return *m_ugc.get();
}

optional<int> Client::numberOfCurrentPlayers() {
  optional<int> out;
  if (!m_nocp)
    m_nocp.emplace(US_FUNC(GetNumberOfCurrentPlayers)(m_user_stats));
  else {
    auto ut = utils(); // TODO: fix it
    m_nocp->update(ut);

    if (!m_nocp->isPending()) {
      if (m_nocp->isCompleted()) {
        out = m_nocp->result().m_cPlayers;
      }
      // TODO: handle errors
      m_nocp = none;
    }
  }

  return out;
}
}
