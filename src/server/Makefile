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
CC=g++

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
LDFLAGS= -L/usr/lib -lboost_system -lpthread

SRC:= $(wildcard *.cpp)
# if client is integrated
# SRC+= $(wildcard server/*.cpp)
OBJECTS= $(SRC:.cpp=.o)
EXECUTABLE=server
EXECDIR=../../bin/
IDIR=../include

all: ${EXECUTABLE}

$(EXECUTABLE): $(OBJECTS)
	    $(CC) $(OBJECTS) -o $(EXECDIR)$(EXECUTABLE) $(LDFLAGS)
#     g++ -c -o AnnealControl.o -lboost_thread AnnealControl.cpp

#
# Cleanning up
#
clean:
	@echo "Cleaning up..."
	rm -f *.o *~

#
# RUN
#

run: $(EXE)
	@echo "Running" $(EXE) "..."
	@./$(EXECDIR)/$< $(EXE_OPTS)
