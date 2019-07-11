#pragma once

#include "steam/steamclientpublic.h"

#include <vector>
#include <string>

#ifndef CHECK
#include <cassert>
#define CHECK assert
#endif

namespace steam {

using std::string;
using std::vector;

class Client;
class Friends;
class UGC;
class Utils;
class User;

bool initAPI();
void runCallbacks();
}
