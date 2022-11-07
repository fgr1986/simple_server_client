##########################################
###  Spectre Handler Library makefile  ###
##########################################

# # of cores used during compiling
export MAKEFLAGS="-j $(grep -c ^processor /proc/cpuinfo)"
# export MAKEFLAGS="-j 2"
# export MAKEFLAGS="-j 1"
############
# Version  #
############
VERSION=v0.2

#################
# Compiler def  #
#################
CC=gcc
CPP=g++
LD=g++

#################
# Gcc flags     #
#################
#CPPFLAGS+= -Wall -g // Debugg it turns on (almost) all the warnings that g++ can tell you about
#CPPFLAGS+= -Wall -O3 -fdump-tree-cfg-graph -fdump-rtl-expand-graph  // rtl graphs, O3
#CPPFLAGS+= -Wall -O3 -g -fdump-tree-original-raw -fdump-tree-all-all-finstrument-functions // rtl graphs, O3
#CPPFLAGS+= -std=c++11 -Wall -O3 -I $(MATLAB_ROOT)extern/include // Matlab,c++11
# CPPFLAGS+= -g -std=c++11 -Wall -O3 -v // verbose
# CPPFLAGS+= -std=c++11 -Wall -O3 -fPIC    // needed for dynamic shared library

# fPIC: Position Independent Code means that the generated machine code
# is not dependent on being located at a specific address in order to work.
# CPPFLAGS+= -std=c++11 -Wall -O3 -fPIC -g
# -Wall -Wextra -Wshadow -Wnon-virtual-dtor -pedantic
# -Wall -Wextra   reasonable and standard
# -Wshadow   warn the user if a variable declaration shadows one from a parent context
# -Wnon-virtual-dtor   warn the user if a class with virtual functions has a non-virtual
# destructor. This helps catch hard to track down memory errors
# -Wold-style-cast   warn for c-style casts
# -Wcast-align   warn for potential performance problem casts
# -Wunused   warn on anything being unused
# -Woverloaded-virtual   warn if you overload (not override) a virtual function
# -pedantic
# -Wconversion   warn on type conversions that may lose data
# -Wsign-conversion   warn on sign conversions
# -Wmisleading-indentation   warn if identation implies blocks where blocks do not exist

# fgarcia no fPIC
CPPFLAGS+= -std=c++14 -O3 -g -Wall -Wextra -Wshadow -Wnon-virtual-dtor -pedantic

#################
# Library flags #
#################
# // Matlab, c++11, boost
# # fgarcia no shared
# LDFLAGS= -L/usr/lib -lboost_thread -lboost_regex -lboost_filesystem -lboost_system -shared

# LDFLAGS= -L/usr/lib -lboost_thread -lboost_regex -lboost_filesystem -lboost_system -lpthread
# only boost:asio
LDFLAGS  = -L/usr/local/lib
LDFLAGS += -L/usr/lib
LDFLAGS +=-lboost_system -lpthread
# math and dynamic linking library
LDFLAGS += -lm -ldl


CPP_FILES:= $(wildcard *.cpp)
# if client is integrated
# SRC+= $(wildcard server/*.cpp)
# Strip dirs to create object names
C_FILE_NAMES = $(notdir $(C_FILES))
C_PATHS = $(dir $(C_FILES) )
C_OBJECTS = $(addprefix $(BUILD_DIR)/, $(C_FILE_NAMES:.c=.o) )

CPP_FILE_NAMES = $(notdir $(CPP_FILES))
CPP_PATHS = $(dir $(CPP_FILES) )
CPP_OBJECTS = $(addprefix $(BUILD_DIR)/, $(CPP_FILE_NAMES:.cpp=.o) )

ASM_FILE_NAMES = $(notdir $(ASM_FILES))
ASM_PATHS = $(dir $(ASM_FILES) )
ASM_OBJECTS = $(addprefix $(BUILD_DIR)/, $(ASM_FILE_NAMES:.S=.o) )

vpath %.c $(C_PATHS)
vpath %.cpp $(CPP_PATHS)
vpath %.S $(ASM_PATHS)

PROJECT_OBJECTS = $(C_OBJECTS) $(CPP_OBJECTS) $(ASM_OBJECTS)
EXECUTABLE=server
EXECDIR=./bin/
BUILD_DIR=build

INCLUDE_PATHS  = -I../include
# INCLUDE_PATHS  += -I/opt/homebrew/Cellar/boost/1.80.0/include
# INCLUDE_PATHS  += -I/opt/homebrew/Cellar/boost/1.80.0/include/boost/thread

all: build ${EXECUTABLE}

build:
	mkdir -p "$(EXECDIR)"
	mkdir -p "$(BUILD_DIR)"

# $(EXECUTABLE): $(OBJECTS)
# 	    $(CC) $(OBJECTS) -o $(EXECDIR)$(EXECUTABLE) $(LDFLAGS)
#     g++ -c -o AnnealControl.o -lboost_thread AnnealControl.cpp

$(BUILD_DIR)/%.o: %.c
	$(V)echo Compiling file: $(notdir $<)
	$(V)$(CC) $(CCFLAGS) $(INCLUDE_PATHS) -c -o $@ $<

$(BUILD_DIR)/%.o: %.cpp
	$(V)echo Compiling file using ++: $(notdir $<)
	$(V)$(CPP) $(CCFLAGS) $(CPPFLAGS) $(INCLUDE_PATHS) -c -o $@ $<

$(EXECUTABLE): $(PROJECT_OBJECTS)
	$(V)echo Linking target: $@
	$(V)$(LD) $(LDFLAGS) $(PROJECT_OBJECTS) -o $(EXECDIR)$(EXECUTABLE)
	# $(V)echo Disassembling target: $@
	# $(V)$(OBJDUMP) -d $(BUILD_DIR)/$@ > $(BUILD_DIR)/$(BUILD_TARGET).s

#
# Cleanning up
#
clean:
	@echo "Cleaning up..."
	rm -f build/*.o *.o *~
