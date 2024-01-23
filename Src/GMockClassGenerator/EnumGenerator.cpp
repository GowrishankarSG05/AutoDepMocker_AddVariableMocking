/**
  * @file: EnumGenerator.cpp
  * @brief: The EnumGenerator generates enum declarations and provides support for both C and C++ scoped enums.
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

#include "EnumGenerator.hpp"

void EnumGenerator::constructEnum(const std::string& fileName, const std::vector<enumProperties>& enumProp) {
    if(isFileInfoRequired(fileName)) {
        mockEnum.append(addIncludeGuard(fileName));
    }

    for(const enumProperties& each : enumProp) {
        // Add namespace
        const std::vector<std::string> namespaceInfo = getNamespaceInfofromfullyQualifiedClassName(each.enumFullName);
        if(namespaceInfo.size()) {
            mockEnum.append(PredefinedMockData::newLine);
            mockEnum.append(PredefinedMockData::newLine);
            for(int i=0; i<namespaceInfo.size(); i++) {
                mockEnum.append(PredefinedMockData::nameSpace); // namespace
                mockEnum.append(namespaceInfo[i]); // namespace Name
                mockEnum.append(PredefinedMockData::openBraces); // namespace Name{
                mockEnum.append(PredefinedMockData::newLine);
            }
        }

        // Add Enum
        mockEnum.append(PredefinedMockData::newLine);
        if(each.isScopedEnum) {
            mockEnum.append(PredefinedMockData::scopedEnum_);
        } else {
            mockEnum.append(PredefinedMockData::enum_);
        }
        mockEnum.append(getEnumNameFromFullyQualifiedEnumName(each.enumName));
        mockEnum.append(PredefinedMockData::aSpace);
        mockEnum.append(PredefinedMockData::openBraces);
        mockEnum.append(PredefinedMockData::newLine);
        for(const std::string eachEV : each.enumValues) {
            mockEnum.append(PredefinedMockData::tab);
            mockEnum.append(eachEV);
            mockEnum.append(PredefinedMockData::semicolon);
            mockEnum.append(PredefinedMockData::newLine);
        }
        mockEnum.append(PredefinedMockData::closeBraces);
        mockEnum.append(PredefinedMockData::semicolon);
        mockEnum.append(PredefinedMockData::newLine);

        // End the enum namespace
        for(int i=0; i<namespaceInfo.size(); i++) { // i unused
            mockEnum.append(PredefinedMockData::closeBraces);
            mockEnum.append(PredefinedMockData::newLine);
        }
    }
    mockEnum.append(PredefinedMockData::newLine);

    writeToFile(fileName, mockEnum);
    mockEnum = {};
}
