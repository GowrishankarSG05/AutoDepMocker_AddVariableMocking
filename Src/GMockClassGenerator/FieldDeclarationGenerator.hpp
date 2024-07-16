/**
  * @file: FiledDeclarationGenerator.hpp
  * @brief: Generate field(struct, class, ...) declaration in mock file
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

#include "GeneratorUtilities.hpp"
#include "MockGeneratorTypes.hpp"

#include <list>

class FieldDeclarationGenerator final : public GeneratorUtilities {
public:

    // Special member functions
    FieldDeclarationGenerator() = default;
    ~FieldDeclarationGenerator() = default;
    FieldDeclarationGenerator& operator =(const FieldDeclarationGenerator&) = delete;
    FieldDeclarationGenerator(const FieldDeclarationGenerator&) = delete;

    void constructFieldDeclaration(const std::string& fileName, const std::list<VariableInfoHierarchy>& fieldInfo);

private:
    const std::pair<std::string, const bool/*record decl*/> appendDeclarationSuffix(const std::string& filedDeclaration);

    // Write declaration to a new file
    void writeDeclatation(const VariableInfoHierarchy& fieldInfoList);

    unsigned int getLastIncludePosition(const std::string fileName);
  
    // Declaration is already present partially, write missing items
    void writeDeclatationInsideDeclaration(const VariableInfoHierarchy& varInfo, std::list<std::string>::iterator& itr);

    // Find the right place to insert declaration in file which already has some declaration
    void findPlaceAndInsertDeclarationInsideDeclaration(const std::string fileName, const VariableInfoHierarchy& varDecInfo);

    // File is present but doesn't have declaration, write it after #include
    void writeDeclatationAfterInclude(const VariableInfoHierarchy& fieldInfoList,
                                     std::list<std::string> fileContent, std::list<std::string>::iterator& itr);

    std::string m_mockClass;
};