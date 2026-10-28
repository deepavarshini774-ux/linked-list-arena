@echo off
REM ===========================================================================
REM  LINKED LIST ARENA - one-click build for Windows 11 + MinGW-w64 (g++)
REM
REM  EDIT THIS ONE LINE if you extracted raylib somewhere else:
REM ===========================================================================
set RAYLIB_PATH=C:\raylib\raylib-5.5_win64_mingw-w64

if not exist "%RAYLIB_PATH%\include\raylib.h" (
    echo.
    echo  ERROR: raylib.h not found at %RAYLIB_PATH%\include
    echo  Edit RAYLIB_PATH at the top of build.bat so it points at the folder
    echo  that contains \include\raylib.h and \lib\libraylib.a
    echo.
    pause
    exit /b 1
)

echo Building LINKED LIST ARENA ...
g++ -std=c++17 -O2 -Wall src\*.cpp -o LinkedListArena.exe ^
    -I "%RAYLIB_PATH%\include" ^
    -L "%RAYLIB_PATH%\lib" ^
    -lraylib -lopengl32 -lgdi32 -lwinmm

if errorlevel 1 (
    echo.
    echo  BUILD FAILED - scroll up for the first error message.
    pause
    exit /b 1
)

echo.
echo  Build OK.  Starting the game...
echo.
LinkedListArena.exe
