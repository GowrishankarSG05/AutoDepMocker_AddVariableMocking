/**
  * @file: GeneratorUtilities.cpp
  * @brief: Base class of mock class generator and it provides basic functionalities
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

#include <algorithm>

#include "GeneratorUtilities.hpp"

// Generate Include information
// Example: /usr/include/MyIncludes/include.hpp
//          Finally extract MyInclude/include.hpp - path without std include location
void GeneratorUtilities::constructIncludes(const std::string& fileName, const std::vector<std::string>& includes) {
    // Add fileInfo which includes copyright information
    mockFile.append(PredefinedMockData::fileInfo);

    // Add include guard
    mockFile.append(PredefinedMockData::newLine);
    mockFile.append(PredefinedMockData::ifndef); // #ifndef
    const std::string fileNameWithDashKeyWord = convertDashToUnderScore(fileName);
    mockFile.append(generateIncludeGuards(fileNameWithDashKeyWord));
    mockFile.append(PredefinedMockData::newLine);
    mockFile.append(PredefinedMockData::define); // #define
    mockFile.append(generateIncludeGuards(fileNameWithDashKeyWord));
    mockFile.append(PredefinedMockData::newLine + PredefinedMockData::newLine);

    // Add include files
    mockFile.append(PredefinedMockData::include + std::string("<gmock/gmock.h>") + PredefinedMockData::newLine);
    for(const std::string& each : includes) {
        // Make sure the same file should not get included
        if((each != fileName) && (std::string::npos == each.find(std::string("/")+fileName))) {
            mockFile.append(PredefinedMockData::include);
            mockFile.append(PredefinedMockData::angleBracketOpen + each + PredefinedMockData::angleBracketClose);
            mockFile.append(PredefinedMockData::newLine);
        }
    }
    mockFile.append(PredefinedMockData::newLine);

    writeToFile(fileName, mockFile);
    mockFile = {}; // Reset
}

// Open files generated in ./GeneratedMocks directory
// Append #endif at last line of the file
void GeneratorUtilities::finishMocking() {
    std::string directoryPath = "./GeneratedMocks/";
    // Iterate over each file in the directory
    for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
        const std::string filePath = entry.path().string();
        // Open the file in append mode
        std::ofstream file(filePath, std::ios::app);
        // Complete the file
        file << "#endif" << std::endl;
    }
}

std::string GeneratorUtilities::getOutFileName(const std::string& fileName) {
    std::filesystem::path path("./GeneratedMocks");
    std::filesystem::create_directory(path);
    return std::filesystem::current_path()/std::filesystem::path("GeneratedMocks")/std::filesystem::path(fileName);
}

std::string GeneratorUtilities::convertDashToUnderScore(const std::string &fileName) {
    std::string result = fileName;
    std::replace(result.begin(), result.end(), '-', '_');
    return result;
}

void GeneratorUtilities::writeToFile(const std::string& fileName, const std::string& content) {
    std::ofstream file(getOutFileName(fileName), std::ios::app);
    file << content;
    file.close();
}

std::string GeneratorUtilities::addIncludeGuard(const std::string& guardName) {
    std::string mockClass;
    // Add fileInfo
    mockClass.append(PredefinedMockData::fileInfo);

    // Add include guard
    mockClass.append(PredefinedMockData::newLine);
    mockClass.append(PredefinedMockData::ifndef); // #ifndef
    mockClass.append(generateIncludeGuards(guardName));
    mockClass.append(PredefinedMockData::newLine);
    mockClass.append(PredefinedMockData::define); // #define
    mockClass.append(generateIncludeGuards(guardName));
    mockClass.append(PredefinedMockData::newLine + PredefinedMockData::newLine);

    // Finally add <gmock/gmock.h>
    mockClass.append(PredefinedMockData::include + std::string("<gmock/gmock.h>") + PredefinedMockData::newLine);

    return mockClass;
}

std::string GeneratorUtilities::getClassNameFromFileName(const std::string& fileName) {
    return fileName.substr(0, fileName.find("."));
}

std::string GeneratorUtilities::getfileNameFromPath(const std::string& filePath) {
    return std::filesystem::path(filePath).filename();
}

bool GeneratorUtilities::isFileInfoRequired(const std::string& fileName) {
    if(std::filesystem::exists(getOutFileName(fileName))) {
        return std::filesystem::is_empty(std::filesystem::path(getOutFileName(fileName)));
    }
    return true;
}

// /usr/include/MyIncludes/MyHeader.hpp
// returns - MyHeader
std::string GeneratorUtilities::getFileNameFromFilePath(const std::string& fileName) {
    auto lastSlash = fileName.find_last_of('/');
    std::string fileNameWithDotH = fileName.substr(lastSlash+1);
    return fileNameWithDotH.substr(0, (fileNameWithDotH.size()-2));
}

// fileName - MockMe.hpp
// return - MOCKME_HPP_
std::string GeneratorUtilities::generateIncludeGuards(const std::string& fileName) {
    std::filesystem::path fileNameWithExt = fileName;
    std::string includeGuardName = fileNameWithExt.stem();

    for(int i=0; i < includeGuardName.size(); i++) {
        includeGuardName[i] = std::toupper(includeGuardName[i]);
    }

    return includeGuardName.append("_HPP_");
}

// argsCount -3
// return - MOCK_CONST_METHOD3
std::string GeneratorUtilities::generateMockFunctionNameFromArgsCount(const uint16_t argsCount, const bool isConst, const bool isTemplated) {
    std::string mockFunction;
    if(isConst) {
        mockFunction = PredefinedMockData::gmockConstFunctionName;
    } else {
        mockFunction = PredefinedMockData::gmockFunctionName;
    }
    mockFunction.append(std::to_string(argsCount));

    if(isTemplated) {
        mockFunction.append("_T");
    }

    return mockFunction;
}

// input - MyNamespace1::MyNamespace2::MyClass, Return - {MyNamespace1, MyNamespace2}
std::vector<std::string> GeneratorUtilities::getNamespaceInfofromfullyQualifiedClassName(const std::string& classWithNP) {
    const auto position = classWithNP.find("::");
    if(std::string::npos == position) {
        return {};
    }

    std::vector<std::string> NPList;

    std::size_t lastPosition = 0;
    std::size_t nextPosition = position; // contains something
    do {
        std::string namespaceString = classWithNP.substr(lastPosition, (nextPosition-lastPosition));
        NPList.push_back(std::move(namespaceString));
        lastPosition = nextPosition+2;
        nextPosition = classWithNP.find("::", lastPosition);
    } while(std::string::npos != nextPosition);

    return NPList;
}

std::string GeneratorUtilities::getEnumNameFromFullyQualifiedEnumName(const std::string& memberType) {
    auto position = memberType.find("::");
    if(std::string::npos == position) {
        return memberType;
    }

    std::size_t lastPostion;
    while(std::string::npos != position) {
        lastPostion = position;
        position = memberType.find("::", position+2);
    }

    return memberType.substr((lastPostion+2), (memberType.size()-(lastPostion+2)));
}

