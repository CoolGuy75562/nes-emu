# nes-emu

NES Emulator (WIP)

CPU output is correct when running nestest.nes in automatic mode. PPU and memory both have issues which will be addressed after graphical frontend is implemented.

## Dependencies

Building and running this project currently requires CMake, CTest, the Boost.Units library, and unistd.h. unistd.h is only needed for getopt in src/app/main.cpp, and is only temporary until a proper frontend is implemented in Qt.

## Installation

Clone the repository
```bash
git clone https://github.com/CoolGuy75562/nes-emu.git
```

In the project root directory, create a build folder and navigate to it
```bash
mkdir build
cd build
```

From the root of the build directory, run CMake on the project root directory, then run the makefile that CMake has just created
```bash
cmake ..
make
```

The makefile should successfully build everything and run some tests, and the main programme executable nes-test should be located in the root of the build directory.

## Usage

```bash
./nes-emu -r [.nes file] [-n] [-c] [-p] [-m]
```

-n is used for tests/nestest.nes in automatic mode which requires the cpu to be initialised with some slightly different values.

-c, -p, -m suppress cpu, ppu, and memory logs respectively.
