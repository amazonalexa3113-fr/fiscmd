# fiscmd C++20

## REQUIREMENT:

### linux
- install `g++` (GCC C++ compiler)
- install `cmake` if you want to use CMake instead

### windows
- install [MinGW-w64](https://www.mingw-w64.org/) to compile using GCC
- install [CMake](https://cmake.org/) if you want to use the CMake build system
- make sure `g++` and `cmake` are available in your PATH before compiling
- or just use WSL ig idk lmfao

## compilation

### linux (GCC)
use the `plscomp` script to compile the application:

```bash
chmod +x plscomp
./plscomp
```

### linux (CMake)
use the `plscompCMake` script to compile the Linux version using CMake:

```bash
chmod +x plscompCMake
./plscompCMake
```

### windows (MinGW-w64 / GCC)
use `plscomp.bat` to compile the application directly using MinGW-w64:

```bat
plscomp.bat
```

### windows (CMake)
use `plscompCMake.bat` to compile the application using CMake:

```bat
plscompCMake.bat
```

### windows (CMake + MinGW-w64)
use `plscompWINDOWS.bat` to compile the Windows version using CMake and MinGW-w64:

```bat
plscompWINDOWS.bat
```

### windows (Linux build scripts converted to batch)
use `plscompLinux.bat` for the Linux-named build script converted into Windows batch format:

```bat
plscompLinux.bat
```

make sure MinGW-w64's `g++` is available in your PATH before running it.

## notes
- linux build scripts are intended for Linux.
- `.bat` build scripts are intended for Windows.
- the CMake builds use the `Release` configuration.
- compiled executables are generated in the project directory or the respective build directory, depending on the build method.
