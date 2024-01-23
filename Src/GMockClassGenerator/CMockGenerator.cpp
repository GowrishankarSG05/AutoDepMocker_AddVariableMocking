/**
  * @file: CMockGenerator.cpp
  * @brief: The CMockGenerator creates C functions and GMOCK wrapper class for C functions
  * @author: Gowrishankar Saminathan <Saminatham.Gowrishankar@in.bosch.com>
  *
  * Copyright [2023-present] [Bosch Global Software Technologies]

  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at

  *     http://www.apache.org/licenses/LICENSE-2.0

  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  */

#include "CMockGenerator.hpp"

void CMockGenerator::constructFunction(const std::string& fileName, const std::vector<MethodInfo>& methodsInfo) {
    // Wrapper that uses gmock class
    constructWrapperFunction(fileName, methodsInfo);

    // C function
    constructMockFunction(fileName, methodsInfo);
}

void CMockGenerator::constructWrapperFunction(const std::string& fileName, const std::vector<MethodInfo>& methodsInfo) {
    if(isFileInfoRequired(getfileNameFromPath(fileName))) {
        mockClass.append(addIncludeGuard(fileName));
    }

    // Add wrapper class
    mockClass.append(PredefinedMockData::class_);
    mockClass.append(getClassNameFromFileName(convertDashToUnderScore(getfileNameFromPath(fileName))));
    mockClass.append(PredefinedMockData::aSpace);
    mockClass.append(PredefinedMockData::openBraces);
    mockClass.append(PredefinedMockData::newLine);
    mockClass.append(PredefinedMockData::public_);


    // Add mock methods
    if(methodsInfo.size()) {
        for(int i=0; i<methodsInfo.size(); i++) {
            const auto mockFuncName = generateMockFunctionNameFromArgsCount(methodsInfo[i].args.size());
            mockClass.append(PredefinedMockData::tab);
            mockClass.append(mockFuncName);
            mockClass.append(PredefinedMockData::openParentheses);
            mockClass.append(methodsInfo[i].name);
            mockClass.append(PredefinedMockData::commaAndSpace);
            mockClass.append(methodsInfo[i].returnType);
            mockClass.append(PredefinedMockData::openParentheses);
            auto methodArgs = methodsInfo[i].args;
            for(int j=0; j<methodArgs.size(); j++) {
                mockClass.append(methodArgs[j]);
                if(methodArgs.size() != (j+1)) { // last arg, Do not add comma
                    mockClass.append(PredefinedMockData::commaAndSpace);
                }
            }
            mockClass.append(PredefinedMockData::closeParentheses);
            mockClass.append(PredefinedMockData::closeParentheses);
            mockClass.append(PredefinedMockData::semicolon);
            mockClass.append(PredefinedMockData::newLine);
        }
    }

    // End the class
    mockClass.append(PredefinedMockData::closeBraces);
    mockClass.append(PredefinedMockData::semicolon);

    writeToFile(std::filesystem::path(fileName).filename(), mockClass);

    mockClass.clear();
}

void CMockGenerator::constructMockFunction(const std::string& fileName, const std::vector<MethodInfo>& methodInfo) {
    std::string wrapper;

    if(isFileInfoRequired(getfileNameFromPath(fileName))) {
        // Add fileInfo
        wrapper.append(PredefinedMockData::fileInfo);
        wrapper.append(PredefinedMockData::newLine);
    }

    wrapper.append(PredefinedMockData::newLine+PredefinedMockData::newLine);
    wrapper.append(getClassNameFromFileName(convertDashToUnderScore(getfileNameFromPath(fileName))));
    wrapper.append(PredefinedMockData::pointer);

    // Define pointer name
    std::string pointerName = getClassNameFromFileName(convertDashToUnderScore(getfileNameFromPath(fileName)));
    wrapper.append(pointerName);
    wrapper.append(PredefinedMockData::initialization + PredefinedMockData::semicolon + PredefinedMockData::newLine);
    wrapper.append(PredefinedMockData::newLine);
    wrapper.append(PredefinedMockData::extern_ + "\"C\"");
    wrapper.append(PredefinedMockData::newLine);
    wrapper.append(PredefinedMockData::openBraces);

    // Define functions
    for(const MethodInfo& each : methodInfo) {
        wrapper.append(PredefinedMockData::newLine);
        wrapper.append(PredefinedMockData::tab);
        wrapper.append(each.returnType);
        wrapper.append(PredefinedMockData::aSpace);
        wrapper.append(each.name);
        wrapper.append(PredefinedMockData::openParentheses);
        int argsSize = 0;
        for(const auto& arg : each.args) {
            wrapper.append(arg + " ");
            ++argsSize;
            wrapper.append(std::string("arg") + std::to_string(argsSize));
            if(each.args.size() != argsSize) {
                wrapper.append(PredefinedMockData::comma + " ");
            }
        }
        wrapper.append(PredefinedMockData::closeParentheses);
        wrapper.append(PredefinedMockData::aSpace + PredefinedMockData::openBraces);
        wrapper.append(PredefinedMockData::newLine);
        wrapper.append(PredefinedMockData::tab + PredefinedMockData::tab);
        wrapper.append(PredefinedMockData::return_);
        wrapper.append(pointerName);
        wrapper.append(PredefinedMockData::pointer_access);
        wrapper.append(each.name);
        wrapper.append(PredefinedMockData::openParentheses);
        for(int i=1; i<=each.args.size(); i++) {
            wrapper.append(std::string("arg") + std::to_string(i));
            if(each.args.size() != i) {
                wrapper.append(PredefinedMockData::comma + PredefinedMockData::aSpace);
            }
        }
        wrapper.append(PredefinedMockData::closeParentheses);
        wrapper.append(PredefinedMockData::semicolon + PredefinedMockData::newLine);
        wrapper.append(PredefinedMockData::tab + PredefinedMockData::closeBraces);
    }

    // Finally close the braces
    wrapper.append(PredefinedMockData::newLine + PredefinedMockData::closeBraces);
    wrapper.append(PredefinedMockData::newLine);
    writeToFile(std::filesystem::path(fileName).filename(), wrapper);
    wrapper = {};
}
