ifndef GCC
GCC = g++
endif
LD = $(GCC)

ifndef MACHINE
MACHINE=$(shell $(GCC) -dumpmachine)
endif

# Detecting system
ifeq ($(MACHINE), x86_64-linux-gnu)
	LIB_DIR=steamworks/redistributable_bin/linux64/
	LIB_NAME=steam_api
	NAME=steam_tools
else ifeq ($(MACHINE), x86_64-w64-mingw32)
	LIB_DIR=steamworks/redistributable_bin/win64/
	LIB_NAME=steam_api64
	NAME=steam_tools.exe
else ifeq ($(MACHINE), i686-w64-mingw32)
	LIB_DIR=steamworks/redistributable_bin/
	LIB_NAME=steam_api
	NAME=steam_tools.exe
#TODO: else handle error
endif

all: $(NAME)

INCLUDES=-I ./ -I keeperrl/ -I keeperrl/extern/ -I steamworks/public/ 
CFLAGS+=$(INCLUDES) -fmax-errors=20 -g -std=c++1y -pthread
LDFLAGS+=-L $(LIB_DIR) -l $(LIB_NAME)

OBJDIR = obj
_dummy := $(shell [ -d $(OBJDIR) ] || mkdir -p $(OBJDIR))
_dummy := $(shell [ -d $(OBJDIR)/keeperrl ] || mkdir -p $(OBJDIR)/keeperrl)

KEEPER_SRCS=debug util directory_path file_path
SRCS = $(shell ls -t *.cpp) $(KEEPER_SRCS:%=keeperrl/%.cpp)

OBJS = $(addprefix $(OBJDIR)/,$(SRCS:.cpp=.o))
DEPS = $(addprefix $(OBJDIR)/,$(SRCS:.cpp=.d))

$(OBJDIR)/%.o: %.cpp
	$(GCC) -MMD $(CFLAGS) -c $< -o $@

$(NAME): $(OBJS)
	$(LD) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) $(OBJDIR)/*.o
	$(RM) $(OBJDIR)/*.d
	$(RM) $(NAME)
	-rmdir $(OBJDIR)

print-vars:
	@echo SRCS: $(SRCS)
	@echo OBJS: $(OBJS)
	@echo CFLAGS: $(CFLAGS)
	@echo LDFLAGS: $(LDFLAGS)

-include $(DEPS)
