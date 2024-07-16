#Ensures that the latest cmake version and features are available
cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

#Project
set(PROJECT_NAME AutoDepMocker)
project(${PROJECT_NAME} CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s")

#Source files for compilation
set(sourceFiles
    Src/Main.cpp
    Src/CodeParser/CustomFrontendAction.cpp
    Src/CodeParser/CustomASTConsumer.cpp
    Src/CodeParser/CustomASTVisitor.cpp
    Src/GMockClassGenerator/GMockClassGenerator.cpp
    Src/GMockClassGenerator/GeneratorUtilities.cpp
    Src/GMockClassGenerator/CPPMockGenerator.cpp
    Src/GMockClassGenerator/CMockGenerator.cpp
    Src/GMockClassGenerator/EnumGenerator.cpp
    Src/GMockClassGenerator/FieldDeclarationGenerator.cpp
    )

#Executable
add_executable(${PROJECT_NAME} ${sourceFiles})

#Dependency directories
target_include_directories(${PROJECT_NAME} PRIVATE
    /usr/lib/llvm-9/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/CodeParser
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/GMockClassGenerator
    )

target_link_libraries(${PROJECT_NAME} PUBLIC
    #Libraries of libtooling
    /usr/lib/llvm-9/lib/libclangTooling.a
    /usr/lib/llvm-9/lib/libclangFrontendTool.a
    /usr/lib/llvm-9/lib/libclangFrontend.a
    /usr/lib/llvm-9/lib/libclangDriver.a
    /usr/lib/llvm-9/lib/libclangSerialization.a
    /usr/lib/llvm-9/lib/libclangCodeGen.a
    /usr/lib/llvm-9/lib/libclangParse.a
    /usr/lib/llvm-9/lib/libclangSema.a
    /usr/lib/llvm-9/lib/libclangStaticAnalyzerFrontend.a
    /usr/lib/llvm-9/lib/libclangStaticAnalyzerCheckers.a
    /usr/lib/llvm-9/lib/libclangStaticAnalyzerCore.a
    /usr/lib/llvm-9/lib/libclangAnalysis.a
    /usr/lib/llvm-9/lib/libclangARCMigrate.a
    /usr/lib/llvm-9/lib/libclangRewrite.a
    /usr/lib/llvm-9/lib/libclangRewriteFrontend.a
    /usr/lib/llvm-9/lib/libclangEdit.a
    /usr/lib/llvm-9/lib/libclangAST.a
    /usr/lib/llvm-9/lib/libclangLex.a
    /usr/lib/llvm-9/lib/libclangBasic.a
    /usr/lib/llvm-9/lib/libclang.so
    /usr/lib/llvm-9/lib/libLLVM-9.so
)
