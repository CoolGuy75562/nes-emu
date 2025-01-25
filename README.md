# nes-emu

NES Emulator (WIP)

CPU output is correct when running nestest.nes in automatic mode, and passes a suite of 10,000 tests for each instruction found here: https://github.com/SingleStepTests/65x02 

PPU is incomplete and memory has some issues, probably to do with mapper.

## Dependencies

Building and running this project currently requires CMake, CTest, Boost.Test, Qt5, and optionally RapidJSON to run the Tom Harte cpu tests (see below).

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

### Tom Harte CPU tests (optional)

Download the nes test files from here https://github.com/SingleStepTests/65x02 and in the root project directory, create a file named harte_tests_dir_path.txt containing the absolute path to the folder containing the tests:

```bash
echo "\"/home/path/to/test/folder\"" > harte_tests_dir_path.txt
```

Building the project as above, in /tests in the build folder there should be a symlink to the directory containing the Harte tests you gave above, and an executable harte_tests. Executing it will run 10,000 test cases for each instruction that has been implemented.

## Usage

In the build directory, run the programme

```bash
./nes-emu
```

You will be prompted to open a .nes file, and if it has been read successfully you can press "start" to begin execution, and "stop" to stop execution. You will see the contents of the CPU and the current instruction being executed, and the contents of the PPU. The OpenGL widget will probably not show anything interesting because the PPU is still being worked on.

