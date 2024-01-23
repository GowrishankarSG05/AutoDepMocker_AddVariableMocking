/**
  * @file: GeneratorUtilities.hpp
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

#ifndef GENERATOR_UTILITIES_HPP
#define GENERATOR_UTILITIES_HPP

#include <string>
#include <fstream>
#include <filesystem>
#include <vector>

#include "Defines.hpp"

// Basic utilities for generating mock class
class GeneratorUtilities {
public:

    // Special member functions
    GeneratorUtilities() = default;
    ~GeneratorUtilities() = default;
    GeneratorUtilities& operator =(const GeneratorUtilities&) = delete;
    GeneratorUtilities(const GeneratorUtilities&) = delete;

    // Wrtie Include information to given file
    // Example: Given: {/usr/include/MyIncludes/include1.hpp, /usr/include/MyIncludes/include2.hpp}
    //          Written: MyInclude/include1.hpp
    //                   MyInclude/include2.hpp
    void constructIncludes(const std::string& fileName, const std::vector<std::string>& includes);

    // Open files generated in ./GeneratedMocks directory
    // Append #endif at last line of the file
    void finishMocking();

protected:
    std::string getOutFileName(const std::string& fileName);

    std::string convertDashToUnderScore(const std::string& fileName);

    void writeToFile(const std::string& fileName, const std::string& content);

    std::string addIncludeGuard(const std::string& guardName);

    std::string getClassNameFromFileName(const std::string& fileName);

    std::string getfileNameFromPath(const std::string& filePath);

    bool isFileInfoRequired(const std::string& fileName);

    // fileName - /usr/include/MyInclude/MyHeader.hpp
    // returns - MyHeader
    std::string getFileNameFromFilePath(const std::string& fileName);

    // fileName - MockMe.hpp
    // return - MOCKME_HPP_
    std::string generateIncludeGuards(const std::string& fileName);

    // argsCount -3
    // return - MOCK_CONST_METHOD3
    std::string generateMockFunctionNameFromArgsCount(const uint16_t argsCount, const bool isConst = false, const bool isTemplated = false);

    // input - Namespace1::Namespace2::MyClass, Return - {Namespace1, Namesapce2}
    std::vector<std::string> getNamespaceInfofromfullyQualifiedClassName(const std::string& classWithNP);

    // Utility function to get enum name from fully qualified name
    // Used only for enum types
    // Example:
    // --------
    // Input: MyNamespace1::MyNamespace2::MyEnum
    // Return: MyEnum
    std::string getEnumNameFromFullyQualifiedEnumName(const std::string& memberType);

private:
    std::string mockFile;
};

#endif // GENERATOR_UTILITIES_HPP
