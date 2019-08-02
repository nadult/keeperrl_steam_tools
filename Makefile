ifndef GCC
GCC = g++
endif
LD = $(GCC)

include keeperrl/Makefile-steam

NAME=steam_tools
ifeq ($(MACHINE), x86_64-w64-mingw32)
	NAME=steam_tools.exe
	CFLAGS+=-DWINDOWS
else ifeq ($(MACHINE), i686-w64-mingw32)
	NAME=steam_tools.exe
	CFLAGS+=-DWINDOWS
else
	LDFLAGS+=-fuse-ld=gold -Wl,-rpath=.
endif

all: $(NAME)

INCLUDES=-I ./ -I keeperrl/ -I keeperrl/extern/
CFLAGS+=$(INCLUDES) -g -ggdb -std=c++1y -pthread -O0
LDFLAGS+=-L keeperrl/$(STEAM_LIB_DIR) -l $(STEAM_LIB_NAME)

OBJDIR = obj
_dummy := $(shell [ -d $(OBJDIR) ] || mkdir -p $(OBJDIR))
_dummy := $(shell [ -d $(OBJDIR)/keeperrl ] || mkdir -p $(OBJDIR)/keeperrl)

KEEPER_SRCS=debug util directory_path file_path 
STEAM_SRCS=$(shell ls -t keeperrl/steam_*.cpp)
SRCS = main.cpp $(STEAM_SRCS) $(KEEPER_SRCS:%=keeperrl/%.cpp)

OBJS = $(addprefix $(OBJDIR)/,$(SRCS:.cpp=.o))
DEPS = $(addprefix $(OBJDIR)/,$(SRCS:.cpp=.d))

$(OBJDIR)/%.o: %.cpp
	$(GCC) -MMD $(CFLAGS) -c $< -o $@

$(NAME): $(OBJS)
	$(LD) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) $(OBJDIR)/*.o $(OBJDIR)/*.d
	$(RM) $(OBJDIR)/keeperrl/*.o $(OBJDIR)/keeperrl/*.d
	$(RM) $(NAME)
	find $(OBJDIR) -type d -empty -delete

print-vars:
	@echo SRCS: $(SRCS)
	@echo OBJS: $(OBJS)
	@echo CFLAGS: $(CFLAGS)
	@echo LDFLAGS: $(LDFLAGS)

-include $(DEPS)
