#include <stdio.h>
#include <string>
#include <vector>
#include <unistd.h>

#include "steam_friends.h"
#include "steam_client.h"
#include "steam_ugc.h"
#include "steam_utils.h"

using namespace std;

void printFriends(const steam::Friends& friends) {
  auto ids = friends.ids();
  for (unsigned n = 0; n < ids.size(); n++)
    printf("Friend #%d: %s [%llu]\n", n, friends.name(ids[n]).c_str(), ids[n].ConvertToUint64());
  fflush(stdout);
}

void displayImage(vector<uint8_t> data, pair<int, int> size) {
  int max_size = 16, sub_size = 2;
  CHECK(size.first == max_size * sub_size && size.second == size.first);

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
  auto ids = friends.ids();
  vector<int> results(ids.size(), -1);
  vector<bool> completed(ids.size(), false);

  for (size_t n = 0; n < ids.size(); n++)
    results[n] = friends.avatar(ids[n], 0);

  unsigned num_completed = 0;
  for (unsigned r = 0; r < 100 && num_completed < ids.size(); r++) {
    for (unsigned n = 0; n < ids.size(); n++) {
      if (completed[n])
        continue;

      results[n] = friends.avatar(ids[n], 0);
      if (results[n] == 0) {
        printf("%d: no avatar\n", n);
        num_completed++;
        completed[n] = true;
      } else if (results[n] == -1) {
        continue;
      } else {
        auto size = utils.imageSize(results[n]);
        printf("%d: Getting avatar data (%dx%d)\n", n, size.first, size.second);
        auto data = utils.imageData(results[n]);
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
  if (!steam::initAPI()) {
    printf("Steam is not running\n");
    return 0;
  }

  steam::Client client;
  auto friends = client.friends();
  auto utils = client.utils();

  printFriends(friends);
  getFriendImages(friends, utils);

  auto ugc = client.ugc();
  auto items = ugc.subscribedItems();
  printf("Items: %d\n", (int)items.size());

  return 0;
}
