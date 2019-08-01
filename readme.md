# steam_tools for KeeperRL

## Building

- extract steamworks sdk into keeperrl/extern/steamworks/ directory in the root
  folder ../steamworks/ should contain public/ and redistributable/ subfolders
- go to keeperrl/ and run fix\_steamworks.sh (it comments/removes declarations
  which cause compilation problems)
- run make

For now following platforms are supported:
- linux x86\_64
- mingw x86\_64 (with MSYS2)
- mingw i686 (with MSYS2)

## Running:

on Windows:
- copy steam\_api.dll or steam\_api64.dll to root folder
- run steam\_works.exe

on Linux:
- copy appropriate (32 or 64 bit) libsteam\_api.so to root folder
- run ./steam\_works


TODO: more details
