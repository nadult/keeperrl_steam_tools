#include <stdio.h>

#include <steam/steam_api_common.h>
#include <steam/steam_api.h>
#include <steam/steam_api_flat.h>

#include <string>
#include <vector>
#include <cassert>

#include <unistd.h>

using namespace std;

namespace steam {

class Friends {
  public:
#define FUNC(name, ...) SteamAPI_ISteamFriends_##name
  Friends(ISteamFriends* ptr) : m_ptr(intptr_t(ptr)) {
    assert(m_ptr);
  }

  vector<CSteamID> allIds() const {
    int count = this->count();
    vector<CSteamID> out;
    out.reserve(count);
    for (int n = 0; n < count; n++)
      out.emplace_back(getIDByIndex(n));
    return out;
  }

  int count(unsigned flags = k_EFriendFlagAll) const {
    return FUNC(GetFriendCount)(m_ptr, flags);
  }

  CSteamID getIDByIndex(int idx, unsigned flags = k_EFriendFlagAll) const {
    return CSteamID(FUNC(GetFriendByIndex)(m_ptr, idx, flags));
  }

  string getName(CSteamID friend_id) const {
    return FUNC(GetFriendPersonaName)(m_ptr, friend_id);
  }

  int getAvatar(CSteamID friend_id, int size) const {
    assert(size >= 0 && size <= 2);
    if (size == 0)
      return FUNC(GetSmallFriendAvatar)(m_ptr, friend_id);
    else if (size == 1)
      return FUNC(GetMediumFriendAvatar)(m_ptr, friend_id);
    else // size == 2
      return FUNC(GetLargeFriendAvatar)(m_ptr, friend_id);
  }

  private:
#undef FUNC
  intptr_t m_ptr;
};

class User {
  public:
#define FUNC(name, ...) SteamAPI_ISteamUser_##name
  User(ISteamUser* ptr) : m_ptr(intptr_t(ptr)) {
    assert(m_ptr);
  }

  CSteamID getID() const {
    return FUNC(GetSteamID)(m_ptr);
  }

  private:
#undef FUNC
  intptr_t m_ptr;
};

class Utils {
#define FUNC(name, ...) SteamAPI_ISteamUtils_##name
  public:
  Utils(ISteamUtils* ptr) : m_ptr(intptr_t(ptr)) {
    assert(ptr);
  }

  // TODO: return expected ?
  pair<int, int> getImageSize(int image_id) const {
    uint32_t w = 0, h = 0;
    if (!FUNC(GetImageSize)(m_ptr, image_id, &w, &h))
      assert(false);
    return {w, h};
  }

  vector<uint8> getImageData(int image_id) const {
    auto size = getImageSize(image_id);
    vector<uint8> out(size.first * size.second * 4);
    if (!FUNC(GetImageRGBA)(m_ptr, image_id, out.data(), out.size()))
      assert(false);
    return out;
  }

  private:
#undef FUNC
  intptr_t m_ptr;
};

class UGC {
#define FUNC(name, ...) SteamAPI_ISteamUGC_##name
  public:
  UGC(ISteamUGC* ptr) : m_ptr(intptr_t(ptr)) {
    assert(ptr);
  }

  int numSubscribedItems() const {
    return (int)FUNC(GetNumSubscribedItems)(m_ptr);
  }

  using FileID = PublishedFileId_t;

  // TODO: consitency with get suffix
  vector<FileID> subscribedItems() const {
    vector<FileID> out(numSubscribedItems());
    int result = FUNC(GetSubscribedItems)(m_ptr, out.data(), out.size());
    out.resize(result);
    return out;
  }

  private:
#undef FUNC
  intptr_t m_ptr;
};

class Client {
#define FUNC(name, ...) SteamAPI_ISteamClient_##name
  public:
  Client() {
    // TODO: handle errors, use Expected<>
    m_ptr = (intptr_t)::SteamClient();
    m_pipe = FUNC(CreateSteamPipe)(m_ptr);
    m_user = FUNC(ConnectToGlobalUser)(m_ptr, m_pipe);
  }
  ~Client() {
    FUNC(ReleaseUser)(m_ptr, m_pipe, m_user);
    FUNC(BReleaseSteamPipe)(m_ptr, m_pipe);
  }

  Client(const Client&) = delete;
  void operator=(const Client&) = delete;

  Friends getFriends() const {
    return FUNC(GetISteamFriends)(m_ptr, m_user, m_pipe, STEAMFRIENDS_INTERFACE_VERSION);
  }

  User getUser() const {
    return FUNC(GetISteamUser)(m_ptr, m_user, m_pipe, STEAMUSER_INTERFACE_VERSION);
  }

  Utils getUtils() const {
    return FUNC(GetISteamUtils)(m_ptr, m_pipe, STEAMUTILS_INTERFACE_VERSION);
  }
  UGC getUGC() const {
    return FUNC(GetISteamUGC)(m_ptr, m_user, m_pipe, STEAMUGC_INTERFACE_VERSION);
  }

  private:
#undef FUNC
  static constexpr const char* version = "144";
  intptr_t m_ptr;
  HSteamPipe m_pipe;
  HSteamUser m_user;
};
}

void printFriends(const steam::Friends& friends) {
  int count = friends.count();
  for (int n = 0; n < count; n++) {
    auto id = friends.getIDByIndex(n);
    printf("Friend #%d: %s [%llu]\n", n, friends.getName(id).c_str(), id.ConvertToUint64());
  }
  fflush(stdout);
}

void displayImage(vector<uint8_t> data, pair<int, int> size) {
  int max_size = 16, sub_size = 2;
  assert(size.first == max_size * sub_size && size.second == size.first);

  for (int y = 0; y < max_size; y++) {
    for (int x = 0; x < max_size; x++) {
      int value = 0;
      for (int ix = 0; ix < sub_size; ix++)
        for (int iy = 0; iy < sub_size; iy++)
          value += data[(x * sub_size + ix + (y * sub_size + iy) * size.first) * 4 + 1];
      value /= sub_size * sub_size;
      printf("%c", value > 80 ? 'X' : ' ');
    }
    printf("\n");
  }
  fflush(stdout);
}

void getFriendImages(const steam::Friends& friends, const steam::Utils& utils) {
  auto ids = friends.allIds();
  vector<int> results(ids.size(), -1);
  vector<bool> completed(ids.size(), false);

  for (size_t n = 0; n < ids.size(); n++)
    results[n] = friends.getAvatar(ids[n], 0);

  unsigned num_completed = 0;
  for (unsigned r = 0; r < 100 && num_completed < ids.size(); r++) {
    for (unsigned n = 0; n < ids.size(); n++) {
      if (completed[n])
        continue;

      results[n] = friends.getAvatar(ids[n], 0);
      if (results[n] == 0) {
        printf("%d: no avatar\n", n);
        num_completed++;
        completed[n] = true;
      } else if (results[n] == -1) {
        continue;
      } else {
        auto size = utils.getImageSize(results[n]);
        printf("%d: Getting avatar data (%dx%d)\n", n, size.first, size.second);
        auto data = utils.getImageData(results[n]);
        displayImage(data, size);

        completed[n] = true;
        num_completed++;
      }
    }
    usleep(100 * 1000);
  }
  printf("Completed: %d\n", num_completed);
}

int main() {
  if (!SteamAPI_Init()) {
    printf("Steam is not running\n");
    return 0;
  }

  steam::Client client;
  auto friends = client.getFriends();
  auto utils = client.getUtils();

  printFriends(friends);
  getFriendImages(friends, utils);

  auto ugc = client.getUGC();
  auto items = ugc.subscribedItems();
  printf("Items: %d\n", (int)items.size());

  return 0;
}
