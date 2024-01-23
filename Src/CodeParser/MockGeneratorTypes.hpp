/**
  * @file: MockGeneratorTypes.hpp
  * @brief: File contains types used to store mock class information
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

#ifndef CUSTOM_TYPES_HPP_
#define CUSTOM_TYPES_HPP_

#include <string>
#include <vector>
#include <map>
#include <cstdint>

#include "clang/Basic/SourceManager.h"
#include "clang/AST/ASTContext.h"

// Contains includes information mapped with file name
using IncludeInfo = std::map<std::string, std::vector<std::string>>;

// Contains mock method(C and C++) information
struct MethodInfo {
    std::string name;
    std::string returnType;
    bool isConst = false;
    bool isOperatorOverloading = false;
    bool isTemplated = false;
    std::vector<std::string> args;
};

// Contains mock class information
struct ClassInfo {
    std::string name;
    std::string fullName;
    std::string filename;
    std::vector<std::string> namespaceInfo;
    bool isTemplateClass = false;
    std::vector<std::string> templateParams;
};

using ClassInfoType = std::map<std::string, ClassInfo>; // contains className and details about the class
using ClassMethodInfoType = std::map<std::string, std::vector<MethodInfo>>; // contains className with methods info

// C functions - store function and filename, filename is key
using CFunctionInfoType = std::map<std::string, std::vector<MethodInfo>>;

// C and CPP Enum information
struct enumProperties {
    std::string enumName; // Unique
    std::string enumFullName; // with namespace
    std::vector<std::string> enumValues;
    bool isScopedEnum = false;
};

using EnumInfo = std::map<std::string/*fileName*/, std::vector<enumProperties>>;

#endif // CUSTOM_TYPES_HPP_
