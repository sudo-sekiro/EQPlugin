## Building instructions for Windows

### Dependencies

Install VS build tools and CMAKE
```
winget install Microsoft.VisualStudio.BuildTools
winget install Kitware.CMake
```
Clone JUCE framework
```
cd SIMPLEEQ
git clone https://github.com/juce-framework/JUCE.git
```
Copy the ASIO SDK to SIMPLEEQ/asiosdk_*
This can be downloaded from: https://www.steinberg.net/en/company/developers.html

### Run Cmake:
```
cmake -Bbuild (-GgeneratorName) (-DJUCE_BUILD_EXTRAS=ON) (-DJUCE_BUILD_EXAMPLES=ON)
cmake --build build --config Debug --target AudioPluginExample_Standalone
```

This guide allows for building the JUCE application without Visual Studio or Projucer.

Building can also be done using VS code and the CMake extension.
This simplifies the building process and allows you to build and launch
the app from VS code.
