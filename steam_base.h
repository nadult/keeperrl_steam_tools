#pragma once

#include "steam/steamclientpublic.h"
#include "keeperrl/util.h"

#ifndef CHECK
#include <cassert>
#define CHECK assert
#endif

namespace steam {

class Client;
class Friends;
class UGC;
class Utils;
class User;

bool initAPI();
void runCallbacks();
}
