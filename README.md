# Text PAD Setup and Compilation on Windows with MinGW

# Working Demo Video

https://github.com/user-attachments/assets/c8ec8d1c-219f-404b-a4c1-7084c749d65e


## Prerequisites
- Windows OS
- MinGW (64-bit recommended)
- Git Bash or MSYS2 (for Unix-like commands)
- CMake (for building FLTK)

## Step 1: Download and Install MinGW
1. Download MinGW-w64 from: [https://www.mingw-w64.org/downloads/](https://www.mingw-w64.org/downloads/)
2. Install MinGW-w64 and add `C:\mingw64\bin` to your system `PATH`.
3. Verify installation:
   ```sh
   g++ --version
   ```
   If it prints the version, MinGW is installed correctly.

## Step 2: Install CMake
1. Download CMake from: [https://cmake.org/download/](https://cmake.org/download/)
2. Install and add CMake to the system `PATH`.
3. Verify installation:
   ```sh
   cmake --version
   ```

## Step 3: Download and Build FLTK
1. Clone the FLTK repository:
   ```sh
   git clone https://github.com/fltk/fltk.git C:\FLTK
   ```
2. Create a build directory:
   ```sh
   mkdir C:\FLTK\build && cd C:\FLTK\build
   ```
3. Run CMake to configure the build:
   ```sh
   cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
   ```
4. Compile FLTK:
   ```sh
   mingw32-make
   ```
5. Verify FLTK build:
   ```sh
   ls C:\FLTK\build\lib
   ```
   You should see `libfltk.a` and other library files.

## Step 4: Compile and Run an FLTK Program
1. Navigate to your project directory:
   ```sh
   cd /d/Competative-Prog/CPP/text-editor
   ```
2. Compile the program using `fltk-config`:
   ```sh
   /c/FLTK/build/fltk-config --compile editor.cxx
   ```
3. Run the compiled program:
   ```sh
   ./editor.exe
   ```
   or in PowerShell:
   ```powershell
   .\editor.exe
   ```

## Troubleshooting
- If `fltk-config` is not found, run it with its full path:
  ```sh
  /c/FLTK/build/fltk-config --version
  ```
- If `FL/Fl.H` is missing, make sure FLTK is built correctly and include paths are set properly.
- If `mingw32-make` fails, ensure MinGW is installed and in your `PATH`.

## Conclusion
You have successfully installed FLTK and compiled an FLTK program on Windows using MinGW! 🎉
