@echo off
setlocal

REM Create build directory if it doesn't exist
if not exist build (
    mkdir build
)

REM Enter build directory
cd build

REM Configure for Release
cmake -DCMAKE_BUILD_TYPE=RelWithDbgInfo -DCMAKE_POLICY_VERSION_MINIMUM=3.5 ..

REM Build using all processors
cmake --build . --config RelWithDbgInfo --parallel

REM Go back to root directory
cd ..

REM Ensure rootDir exists
if not exist rootDir (
    mkdir rootDir
)

REM Copy the built executable and overwrite if it exists
copy /Y "build\src\RelWithDbgInfo\Starlight.exe" "rootDir\Starlight.exe"

echo Done. Output Path: rootDir/Starlight.exe

endlocal
pause