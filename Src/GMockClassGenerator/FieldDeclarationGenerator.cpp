/**
  * @file: FiledDeclarationGenerator.cpp
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

#include "FieldDeclarationGenerator.hpp"

namespace {
    unsigned int blockOpened = 0; // To keep track of number of blocks opened ("{")
    int lastFoundLine = -1;
    int currentLine = 0;
    std::list<std::string> existingFileContent;
}

void FieldDeclarationGenerator::constructFieldDeclaration(const std::string& fileName, const std::list<VariableInfoHierarchy>& fieldInfoList) {

    m_mockClass = {};
    if(isFileInfoRequired(fileName)) {
        m_mockClass.append(addIncludeGuard(fileName));

        // Write variables information
        for(auto const& each : fieldInfoList) {
            blockOpened = 0;
            m_mockClass.append(PredefinedMockData::newLine);
            writeDeclatation(each);
        }
        writeToFile(fileName, m_mockClass);
        m_mockClass = {};
    } else {
        // Find a place to write variable detains in an existing file
        // If variable information is not present in file, then write it after #include
        // If variable information is present partially then write missing details
        for(const auto& each : fieldInfoList) {
            m_mockClass = {};
            bool found = false;
            std::ifstream fileHandle(getOutFileName(fileName));
            std::string line;
            while (std::getline(fileHandle, line)) {
                if(line.find(each.variableInfo) != std::string::npos) {
                    found = true;
                    break;
                }
            }


            lastFoundLine = -1;
            currentLine = 0;
            blockOpened = 0;
            existingFileContent.clear();
            std::ifstream file(getOutFileName(fileName));
            while (std::getline(file, line)) {
                existingFileContent.push_back(line);
            }

            // @ToDo: Below functions shares almost same logic. Make it as single function
            if(found) {
                findPlaceAndInsertDeclarationInsideDeclaration(getOutFileName(fileName), each);
            } else {
                auto insertPosition = getLastIncludePosition(getOutFileName(fileName));
                auto itr = existingFileContent.begin();
                std::advance(itr, insertPosition);
                writeDeclatationAfterInclude(each, existingFileContent, itr);
            }

            m_mockClass = {};
            for(const auto& each : existingFileContent) {
                m_mockClass.append(each + PredefinedMockData::newLine);
            }
            writeToFileOverWrite(getOutFileName(fileName), m_mockClass);
        }
    }
}

const std::pair<std::string, const bool> FieldDeclarationGenerator::appendDeclarationSuffix(const std::string& fieldDeclaration) {
    if((std::string::npos != fieldDeclaration.find("struct ")) || 
      (std::string::npos != fieldDeclaration.find("class ")) ||
      (std::string::npos != fieldDeclaration.find("union ")) ||
      (std::string::npos != fieldDeclaration.find("namespace "))) {
        return {fieldDeclaration + " {", true};
    }

    // Just add ";" at the end for simple types
    return {(fieldDeclaration + ";"), false};
}

void FieldDeclarationGenerator::writeDeclatation(const VariableInfoHierarchy& fieldInfoList) {

    // Write field data first and the process the child nodes
    const auto fieldName = appendDeclarationSuffix(fieldInfoList.variableInfo);
    // Calculate index position
    std::string index;
    for (int i = 0; i < blockOpened; ++i) {
        index += PredefinedMockData::tab;
    }

    bool isBlockOpened = false;
    if(fieldName.second) {
        isBlockOpened = true;
        blockOpened++;
    }

    m_mockClass.append(index + fieldName.first + PredefinedMockData::newLine);

    for(const auto& eachChild : fieldInfoList.variableInfoHierarchyList) {
        writeDeclatation(eachChild);
    }
    if(isBlockOpened) {
        blockOpened--;
        m_mockClass.append(index + PredefinedMockData::closeBraces + PredefinedMockData::semicolon + PredefinedMockData::newLine);
    }
}

unsigned int FieldDeclarationGenerator::getLastIncludePosition(const std::string fileName) {
    std::ifstream file(fileName);

    std::string line;
    int lastIncludeLine = -1;
    int currentLineLocal = 0;

    while (std::getline(file, line)) {
        currentLineLocal++;
        if (line.find("#include") != std::string::npos) {
            lastIncludeLine = currentLineLocal;
        }
    }

    return lastIncludeLine;
}

void FieldDeclarationGenerator::writeDeclatationInsideDeclaration(const VariableInfoHierarchy& varInfo, std::list<std::string>::iterator& itr) {
    // itr points to somewhere in the vector
    // Add data to next line
    const auto fieldName = appendDeclarationSuffix(varInfo.variableInfo);
    auto preLineIndex = std::prev(itr)->find_first_not_of(" ");
    if(preLineIndex == std::string::npos) {
        preLineIndex = 0;
    }

    std::string index = {};
    for(int i=0; i<preLineIndex; i++) {
        index.append(PredefinedMockData::aSpace);
    }
    if(index.empty()) {
        index.append(PredefinedMockData::tab);
    }

    for (int i = 0; i < blockOpened; ++i) {
        index += PredefinedMockData::tab;
    }

    const auto filedName = appendDeclarationSuffix(varInfo.variableInfo);
    bool isBlockOpened = false;
    if(fieldName.second) {
        isBlockOpened = true;
        blockOpened++;
    }

    itr = existingFileContent.insert(std::next(itr), (index + filedName.first));

    for(auto& each : varInfo.variableInfoHierarchyList) {
        writeDeclatationInsideDeclaration(each, itr);
    }
    if(isBlockOpened) {
        blockOpened--;
        std::advance(itr, 1);
        existingFileContent.insert(itr, index + PredefinedMockData::closeBraces + PredefinedMockData::semicolon);
        if(blockOpened == 0) { // Add an empty line
            existingFileContent.insert(itr, PredefinedMockData::newLine);
        }
    }
}

void FieldDeclarationGenerator::findPlaceAndInsertDeclarationInsideDeclaration(const std::string fileName, const VariableInfoHierarchy& varDecInfo) {
    bool fieldFound = false;
    static std::ifstream file(fileName);
    std::string line;

    while (std::getline(file, line)) {
        currentLine++;
        if (line.find(varDecInfo.variableInfo) != std::string::npos) {
            fieldFound = true;
            lastFoundLine = currentLine;
            for(auto& each : varDecInfo.variableInfoHierarchyList) {
                findPlaceAndInsertDeclarationInsideDeclaration(fileName, each);
            }
        }
    }

    if(! fieldFound) {
        // write remainig data into the file after certain line number
        auto itr = existingFileContent.begin();
        std::advance(itr, lastFoundLine);
        writeDeclatationInsideDeclaration(varDecInfo, itr);
        lastFoundLine++; // To write next field after current field
    }
}

void FieldDeclarationGenerator::writeDeclatationAfterInclude(const VariableInfoHierarchy& fieldInfoList, 
                            std::list<std::string> fileContent, std::list<std::string>::iterator& itr) {

    const auto fieldName = appendDeclarationSuffix(fieldInfoList.variableInfo);
    // Calculate index position
    std::string index;
    for (int i = 0; i < blockOpened; ++i) {
        index += PredefinedMockData::tab;
    }

    bool isBlockOpened = false;
    if(fieldName.second) {
        blockOpened++;
        isBlockOpened = true;
    }

    itr = fileContent.insert(std::next(itr), index + fieldName.first);

    for(const auto& eachChild : fieldInfoList.variableInfoHierarchyList) {
        writeDeclatationAfterInclude(eachChild, fileContent, itr);
    }

    if(isBlockOpened) {
        blockOpened--;
        std::advance(itr, 1);
        itr = fileContent.insert(itr, index + PredefinedMockData::closeBraces + PredefinedMockData::semicolon);
        if(blockOpened == 0) { // Add an empty line at the end
            std::advance(itr, 1);
            fileContent.insert(itr, PredefinedMockData::newLine);
        }
    }
}