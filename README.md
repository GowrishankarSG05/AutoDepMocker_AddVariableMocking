# AutoDepMocker

A simple utility that helps identifying dependencies of your test unit and generate Google Mock class out of that

## Why AutoDepMocker
Writting mock classes for test units can often be tedious and time-consuming. It requires the identification of the classes, methods, and declarations that need to be mocked

AutoDepMocker comes here to simplify the mocking process. The tool has the intelligence to identify the dependencies of your class and will generate google mock class

## How it works
AutoDepMocker is using clang frontend parsing libraries(libtooling, AST, ...) to parse the given input file and note downs each declarations and function calls. Once parsing is complete, it uses [MockClassGenerator](/Src/MockClassGenerator/) to construct mock class  
Please refer [CMakeLists.txt](/CMakeLists.txt) include list to know more about the list of clang libraries used in AutoDepMocker

## How to Build

## Build from source:
- Run `Build.sh` in your linux machine  
*This script takes care of installing the necessary packages required to build this utility, Finally the executable will be installed  in `~/.bin/` directory.*

## Prebuild binaries
- This support is yet to be added

## How to generate mock class

Basically AutoDepMocker provides below utilities  
1. <b>AutoDepMocker</b>  
   &emsp;*The core utility which takes care of parsing the given source file and generate mock class    
    &emsp;Note: AutoDepMocker expects right compilation settings to be passed along with source file like your compiler  
    &emsp;<b>So always use plugin script to execute AutoDepMocker until unless you know all such compilation settings of your test unit</b>*

2. <b>Python plugin</b>  
   &emsp;*A plugin which helps identifying the right include paths from cmake build output files and execute AutoDepMocker with necessary includes, sysroot and compiler settings*  
   &emsp;*A plugin which is developed for Yocto environment which uses CMake Ninja build system is available [here](/Scripts/AutoDepMocker_YoctoCMake.sh)*  
   &emsp;Please refer [here](#how-to-use-AutoDepMocker-on-other-build-environment) if you want to write a plugin suitable for your build environment  
   ### <u>Yocto project which uses CMake and Ninja</u>:  
   A python utility [AutoDepMocker_YoctoCMake.py](/Scripts/AutoDepMocker_YoctoCMake.sh) is written to parse CMake and ninja files for Yocto based project. It executes `AutoDepMocker` with necessary options
   1. Run AutoDepMocker_YoctoCMake.py:  
   Example: `AutoDepMocker_YoctoCMake.py -s MyProject/MySourceFile.cpp -r MyProject/ -sr /repo/out/MyProject/git/recipe-sysroot/`  
   *`-s`: The source file you intend to test.  
   `-r`: Typically your git repository path.  
   `-sr`: The Yocto recipe-sysroot path of your recipe.*  
   2. Choose either interactive or non-interactive mode.  
   3. That's all! Mock class would be generated and available in `./GeneratedMocks` directory


## How to use AutoDepMocker on other build environment  
### Consider AutoDepMocker as like any compiler. It needs to know all such compiler options to work correctly and since this has been developed using clang libraries, below clang compier options to be passed when executing  
### So You need to perform below steps to execute AutoDepMocker with right compilation options in your build environment  
1. Write a utility to fetch compilation settings information from build files  
<u>List of options to be read from build files and to be passed to AutoDepMocker</u>  
    1. Your test unit(either .c or .cpp file)  
    2. sysroot - sysroot directory. Example-*./MyRecipe/git/recipe-sysroot/*
    3. target - Target for which the file is developed for. Example-*arm-v5-nvidia-linux-hard*
    4. mfpu - FPU options for a specific processor or architecture. Example-*neon or vfp*
    5. mfloat-abi - Mostly either hard or soft
    6. march - Target architecture for which the code is being compiled
    7. List of include directories - Example- *-I/MyIncludeDir*  
    <u>Example:</u>
     ```AutoDepMocker MyFile.cpp -- --sysroot=/path/to/my/sysroot/ --target=arm-v5-nvidia-linux-hard -mfpu=neon -mfloat-abi=hard -march=armv7-a -mthumb --std=c++17 -I/MyInclude/Directory1/ -I/MyInclude/Directory2/```  
     <b>Note: All options should be passed only after the double dash(--) like above</b>

2. Execute `AutoDepMocker` with necessary compilation options like above from the utility  
    *Lets say you want to use it in yocto environment which has meson build system.  
    Write a utility to grab your compilation include information from build output/config files and pass it to `AutoDepMocker`. That's it!*

## How to use AutoDepMocker for other Mocking framework
- Current AutoDepMocker has [CodeGenUtils](/Src/MockClassGenerator//) which supports to build GMOCK classes
- Just replace this component with your own component to support for other mocking frameworks
- Task has been planned to move code generation part [CodeGenUtils](/Src/MockClassGenerator//) as a plugin for convenience

## Possible error when mocking
- Make sure you don't have previous `GeneratedMocks` directory
- Make sure your build directory is not broken if you are using any plugin

## Limitations
- AutoDepMocker does not offer support for mocking classes that lack methods
- AutoDepMocker supports only in mocking class methods, C/C++ enums and C functions
- AutoDepMocker does not provide support for mocking class member variables
- Due to the intricate nature of C++ standard include files, the inclusion of "bits/stdc++.h" becomes necessary in generated files
- The performance and effectiveness of AutoDepMocker rely entirely on the provided input

## Author
- [Gowrishankar Saminathan](Saminatham.Gowrishankar@in.bosch.com)

## Maintainer
- [Gowrishankar Saminathan](Saminatham.Gowrishankar@in.bosch.com)

## License
AutoDepMocker is open-sourced under the Apache-2.0 license. See the [LICENSE](LICENSE) file for details
