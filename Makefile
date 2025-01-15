CXX = g++
CC = gcc

SRCDIR = ./src
INCDIR = ./include

CFLAGS = -g -I$(INCDIR) -Wall -Wpedantic -Wextra
CXXFLAGS = -g -I$(INCDIR) -Wall -Wpedantic -Wextra

CSOURCES = $(wildcard $(SRCDIR)/*.c)
CXXSOURCES = $(wildcard $(SRCDIR)/*.cpp)
COBJECTS = $(CSOURCES:$(SRCDIR)/%.c=$(SRCDIR)/%.o)
CXXOBJECTS = $(CXXSOURCES:$(SRCDIR)/%.cpp=$(SRCDIR)/%.o)

all: nes-emu

$(SRCDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(SRCDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

nes-emu: $(COBJECTS) $(CXXOBJECTS)
	$(CXX) $(CXXFLAGS) $(COBJECTS) $(CXXOBJECTS) -o nes-emu

.PHONY: clean

clean:
	rm -f $(SRCDIR)/*.o nes-emu
