# steam_tools

## Building

- extract steamworks sdk into steamworks/ directory in the root folder
- run fix\_headers.sh (it comments/removes declarations which cause compilation problems)
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
- run run\_linux.sh


TODO: more details
