#!/usr/bin/env python

#  @file: AutoDepMocker_YoctoCMake.py
#  @brief: AutoDepMocker extension for Yocto and CMake(ninja) based build environment
#          This script would parse and find out necessary compilation includes from cmake build files
#  @author: Gowrishankar Saminathan <Saminatham.Gowrishankar@in.bosch.com>
#
#  Copyright [2023-present] [Bosch Global Software Technologies]

#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at

#      http://www.apache.org/licenses/LICENSE-2.0

#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.

# @Note:
# ------
#  1. There is a limit for command line arguments in linux system
#     It will fail when a project's cmake has huge include and link dependencies.
#       Todo: Do not include the full path, Instead create a symbolic link to recipe-sysroot and use that

import os
import subprocess
import argparse

#  1. Find out Include directories from build.ninja. CMakeCache.txt might not contain everything
#  2. Include right c++ include directory from CMakeOutput.log (One could have many versions of c++ std libraries in /usr/include/c++, So find the right one)
#  3. Include project directories
#  4. Finally form compiler options and run AutoDepMocker
class CompilerSetting:
    # Form compiler setting based on given sysroot directory
    def __init__(self, fileName, sourceDir, sysrootDir):
        if sysrootDir is None: #x86
            self.__compilerSetting = "~/.bin/AutoDepMocker " + fileName
        else:
            # Parse CMakeOutput.txt file
            CXXIncludeDir = self.__getCXXIncludeDirs(sysrootDir+"../build/CMakeFiles/CMakeOutput.log")

            # Parse source directory and get all subdirectories
            sourceDirList = self.__getSourceCodeIncludeDirs(sourceDir)

            # Necessary for gen. files(Example: dbus)
            buildDirlist = self.__getSourceCodeIncludeDirs(sysrootDir+"/../build")

            # build.ninja directories
            buildNijaIncludes = self.__getBuildInclude(sysrootDir+"/../build/build.ninja")

            # Finally construct compiler setting
            self.__compilerSetting = self.__formCompilerSetting(fileName, sysrootDir, CXXIncludeDir, sourceDirList, buildDirlist, buildNijaIncludes)

    def getCompilerSetting(self):
        return self.__compilerSetting

    # @Note:
    # Current implemetation simply includes "include directory" if -I/ keyword is found
    # @Todo: Propery way would be to generate compilation_database.json and fetch include directories from that file.
    def __getIncludeDirs(self, filePath):
        with open(filePath, 'r') as file:
            includeDirlist = []
            for line in file:
                if -1 != line.find("-I/"):
                # Keyword found
                    splitIncList = line.split('-I/')
                    for item in splitIncList[1:]:
                        item = '/' + item
                        item = item.strip()
                        if(includeDirlist.count(item.replace(';', ''))):
                            continue
                        includeDirlist.append(item.replace(';', ''))
                elif -1 != line.find('Path to a file'):
                    # Keyword found, Move to next line to get include dirs
                    for nextLine in file:
                          if 0 == includeDirlist.count((nextLine.partition(":PATH=")[2]).strip()):
                              includeDirlist.append((nextLine.partition(":PATH=")[2]).strip())
                              break

        return includeDirlist

    def __getCXXIncludeDirs(self, filePath):
        with open(filePath, 'r') as file:
            includeDirsList = []
            for line in file:
                if -1 == line.find("#include <...> search starts here:"):
                    continue
                #keyword found
                for nextLine in file:
                    if -1 != nextLine.find("End of search list"):
                        break
                    # Exact line found
                    nextLine = nextLine.strip('ignore line: [').strip(']')
                    includeDirsList.append(nextLine.strip(" ").strip().strip(']'));
        return includeDirsList

    def __getSourceCodeIncludeDirs(self, filePath):
        dirList = []
        for root, dirs, files in os.walk(filePath):
            if ((-1 != root.find(".git")) or (-1 != root.find("Test"))):
                continue

            if(-1 != root.find("CMakeFiles")):
                continue

            dirList.append(root)
        return dirList

    def __getBuildInclude(self, filePath):
         with open(filePath, 'r') as file:
            finalList = []
            for line in file:
                if -1 == line.find("INCLUDES = "):
                    continue
                #keyword found
                #nextLine = file.readline()
                line = line.strip('INCLUDES =')

                # ISSUE: There is a limit for command line arguments in linux system.
                #        We run out of space if we simply include everything from build.ninja
                #        So avoid adding duplicate "includes" in global list to avoid above issue

                # line is a string, Split to list
                newList = line.split()
                # new list might contain both -isystem and -I
                for i, item in enumerate(newList):
                    if -1 != item.find('-I/'):
                        if item not in finalList:
                            finalList.append(item)
                    elif -1 != item.find('-isystem'): # Store '-isystem /a/b/c'
                        isystem = item
                        systemInc = isystem + ' ' + newList[i+1]
                        if systemInc not in finalList:
                            finalList.append(systemInc)

            return finalList

    def __formCompilerSetting(self, fileName, sysrootDir, cxxIncludeDirs, sourceFileDirs, buildDirlist, buildNijaIncludes):
        compilerSetting = "~/.bin/AutoDepMocker " + fileName + " -- --sysroot=" + sysrootDir + " --target=arm-v5-nvidia-linux-hard -mfpu=neon -mfloat-abi=hard -march=armv7-a -mthumb -ferror-limit=1000000 -I" \
                           + sysrootDir.strip() + "/usr/include/"

        if -1 != fileName.find(".cpp"):
            compilerSetting += " --std=c++17 "

        for item in cxxIncludeDirs:
            compilerSetting += " -I" + item
        for item in sourceFileDirs:
            compilerSetting += " -I" + item
        for item in buildDirlist:
            compilerSetting += " -I" + item
        for item in buildNijaIncludes:
            compilerSetting += " " + item

        return compilerSetting

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="=>Default mock class generator")
    parser.add_argument('-s', '--sourcefile', help="Source file", required=True)
    parser.add_argument('-r', '--rootdir', help="Root directory of your project", required=True)
    parser.add_argument('-sr', '--sysrootdir', help="Sysroot directory")
    args = vars(parser.parse_args())

    obj = CompilerSetting(args['sourcefile'], args['rootdir'], args['sysrootdir'])

    compilationSetting = obj.getCompilerSetting();
    subprocess.call(compilationSetting, shell=True)
