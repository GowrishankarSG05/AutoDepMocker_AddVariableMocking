/**
  * @file: GMockClassGeneratorImpl.cpp
  * @brief: The GMockClassGeneratorImpl creates a mock class based on GMOCK, along with a wrapper class for operator overloading.
  *         It supports mocking enum types, field declaration and C functions as well.
  *         Additionally, it supports template classes.
  * @author: Gowrishankar Saminathan <gowrishankarstg@gmail.com>
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

#include "GMockClassMockGeneratorImpl.hpp"

namespace {
    unsigned long int indentation = 0;
    // C functions
    std::list<MethodInfo> cFunctionsList = {};
}

void GMockClassGeneratorImpl::constructMockClass(const std::string& fileName, const std::list<MockInfoStorage>& mockInfoList, 
                            const std::list<std::string>& includeList) {

    // Write file header
    mockFile.append(addIncludeGuard(fileName));

    // Write #include information
    for(const std::string& eachIncludeFile : includeList) {
        // Make sure the same file should not get included
        if((eachIncludeFile != fileName) && (std::string::npos == eachIncludeFile.find(std::string("/")+fileName))) {
            mockFile.append(PredefinedMockData::include);
            mockFile.append(PredefinedMockData::angleBracketOpen + eachIncludeFile + PredefinedMockData::angleBracketClose);
            mockFile.append(PredefinedMockData::newLine);
        }
    }
    mockFile.append(PredefinedMockData::newLine);

    for(const auto& each : mockInfoList) {
        writeMockData(fileName, each);
    }

    // Construct C functions if present
    if(! cFunctionsList.empty()) {
        constructCFunction(fileName, cFunctionsList);
        cFunctionsList.clear();
    }

    mockFile.append(PredefinedMockData::newLine);

    writeToFile(fileName, mockFile);

    mockFile = {}; // Reset
}

void GMockClassGeneratorImpl::writeMockData(const std::string& fileName, const MockInfoStorage& mockData) {
    std::list<MethodInfo> operatorOLMethodList = {};
    switch(mockData.data.index()) {
        case 0: { // NamespaceInfo
            NamespaceInfo namespaceInfo = std::get<NamespaceInfo>(mockData.data);
            // Example: Namespace foo {
            mockFile.append(getIndentation() + PredefinedMockData::nameSpace + namespaceInfo.namespaceName 
                            + PredefinedMockData::aSpace + PredefinedMockData::openBraces + PredefinedMockData::newLine);
            // Set indentation to one tab
            ++indentation;
            for(const auto& childNodeMockData : mockData.childData) {
                writeMockData(fileName, childNodeMockData);
            }
            // Check if C functions present
            if(! cFunctionsList.empty()) {
                constructCFunction(fileName, cFunctionsList);
                cFunctionsList.clear(); // Make sure to clean the list if functions defined inside namespace
            }
            // Set indentation to one tab less
            --indentation;
            mockFile.append(PredefinedMockData::newLine);
            mockFile.append(getIndentation() + PredefinedMockData::closeBraces + PredefinedMockData::newLine);
        }
        break;
        case 1: { // ClassStructUnionInfo
            ClassStructUnionInfo classInfo = std::get<ClassStructUnionInfo>(mockData.data);

            // Add template class information
            if(classInfo.isTemplateClass && classInfo.templateParams.size()) {
                mockFile.append(PredefinedMockData::newLine);
                mockFile.append(getIndentation() + std::string("// Template mock class") + PredefinedMockData::newLine);
                mockFile.append(getIndentation() + PredefinedMockData::template_ + PredefinedMockData::angleBracketOpen);
                for(int i=0; i<classInfo.templateParams.size(); i++) {
                    mockFile.append(PredefinedMockData::typename_ + classInfo.templateParams[i]);
                    if((i + 1) == classInfo.templateParams.size()) { // End
                        mockFile.append(PredefinedMockData::angleBracketClose);
                        break;
                    }
                    mockFile.append(PredefinedMockData::comma + PredefinedMockData::aSpace);
                }
            }

            // Add class
            mockFile.append(PredefinedMockData::newLine);
            mockFile.append(getIndentation() + classInfo.declKindName); // class/struct/union
            mockFile.append(classInfo.name);
            mockFile.append(PredefinedMockData::aSpace);
            mockFile.append(PredefinedMockData::openBraces);
            mockFile.append(PredefinedMockData::newLine);
            ++indentation;
            mockFile.append(getIndentation() + PredefinedMockData::public_);

            // Add static method - getInstance()
            mockFile.append(getIndentation() + PredefinedMockData::static_);
            mockFile.append(classInfo.name + PredefinedMockData::getInstance);
            mockFile.append(PredefinedMockData::openParentheses + PredefinedMockData::closeParentheses);
            mockFile.append(PredefinedMockData::aSpace + PredefinedMockData::openBraces);
            mockFile.append(PredefinedMockData::newLine);
            ++indentation;
            mockFile.append(getIndentation() + PredefinedMockData::return_ +
                            PredefinedMockData::thisPtr + PredefinedMockData::semicolon);
            mockFile.append(PredefinedMockData::newLine);
            --indentation;
            mockFile.append(getIndentation() + PredefinedMockData::closeBraces);
            mockFile.append(PredefinedMockData::newLine + PredefinedMockData::newLine);
            mockFile.append(getIndentation() + PredefinedMockData::static_);
            mockFile.append(classInfo.name + PredefinedMockData::pointer);
            mockFile.append(PredefinedMockData::thisPtr + PredefinedMockData::semicolon);
            mockFile.append(PredefinedMockData::newLine + PredefinedMockData::newLine);

            for(const auto& childNodeMockData : mockData.childData) {
                writeMockData(fileName, childNodeMockData);
            }

            // Now add operator overloading methods
            // Because this can't be mocked using gmock
            if(operatorOLMethodList.size()) {
                for(const auto& OperatorOLMethod : operatorOLMethodList) {
                    mockFile.append(PredefinedMockData::newLine);
                    mockFile.append(getIndentation() + OperatorOLMethod.returnType + PredefinedMockData::aSpace);
                    mockFile.append(OperatorOLMethod.name + PredefinedMockData::openParentheses);
                    auto methodArgs = OperatorOLMethod.args;
                    for(int j=0; j<methodArgs.size(); j++) {
                        mockFile.append(methodArgs[j]);
                        mockFile.append(std::string(" arg") + std::to_string(j+1));
                        if(methodArgs.size() != (j+1)) { // last arg, Do not add comma
                            mockFile.append(PredefinedMockData::commaAndSpace);
                        }
                    }
                    mockFile.append(PredefinedMockData::closeParentheses);
                    mockFile.append(PredefinedMockData::aSpace + PredefinedMockData::openBraces + PredefinedMockData::newLine);
                    ++indentation;
                    mockFile.append(getIndentation() + getOperatorName(OperatorOLMethod.name));
                    mockFile.append(PredefinedMockData::openParentheses);
                    for(int j=0; j<methodArgs.size(); j++) {
                        mockFile.append(std::string("arg") + std::to_string(j+1));
                        if(methodArgs.size() != (j+1)) { // last arg, Do not add comma
                            mockFile.append(PredefinedMockData::commaAndSpace);
                        }
                    }
                    mockFile.append(PredefinedMockData::closeParentheses + PredefinedMockData::semicolon);
                    --indentation;
                    mockFile.append(PredefinedMockData::newLine);
                    mockFile.append(getIndentation() + PredefinedMockData::closeBraces);
                    mockFile.append(PredefinedMockData::newLine);
                }
            }

            --indentation;
            mockFile.append(PredefinedMockData::newLine);
            mockFile.append(getIndentation() + PredefinedMockData::closeBraces + PredefinedMockData::semicolon);
            mockFile.append(PredefinedMockData::newLine);
            break;
        }
        case 2: { // EnumInfo
            enumProperties enumInfo = std::get<enumProperties>(mockData.data);
            
            // Add Enum
            mockFile.append(PredefinedMockData::newLine);
            if(enumInfo.isScopedEnum) {
                mockFile.append(getIndentation() + PredefinedMockData::scopedEnum_);
            } else {
                mockFile.append(getIndentation() + PredefinedMockData::enum_);
            }
            mockFile.append(getEnumNameFromFullyQualifiedEnumName(enumInfo.enumName));
            mockFile.append(PredefinedMockData::aSpace);
            mockFile.append(PredefinedMockData::openBraces);
            mockFile.append(PredefinedMockData::newLine);
            ++indentation;
            for(const std::string eachEV : enumInfo.enumValues) {
                mockFile.append(getIndentation() + eachEV);
                mockFile.append(PredefinedMockData::comma);
                mockFile.append(PredefinedMockData::newLine);
            }
            --indentation;
            mockFile.append(getIndentation() + PredefinedMockData::closeBraces);
            mockFile.append(PredefinedMockData::semicolon);
            mockFile.append(PredefinedMockData::newLine);

            break;
        }
        case 3: { // MethodInfo
            MethodInfo methodInfo = std::get<MethodInfo>(mockData.data);

            // Normal method
            if(methodInfo.isCFunction) {
                cFunctionsList.push_back(methodInfo); // This will be constructed later, either end of namespace or file
            } else if(methodInfo.isOperatorOverloading) {
                // Operator overloading function
                // At end of class, all operator overloading functions will be grouped and constructed
                operatorOLMethodList.push_back(methodInfo);
            } else {
                const auto mockFuncName = generateMockFunctionNameFromArgsCount(methodInfo.args.size(), methodInfo.isConst, methodInfo.isTemplated);
                mockFile.append(getIndentation() + mockFuncName);
                mockFile.append(PredefinedMockData::openParentheses);
                methodInfo.isOperatorOverloading ?
                    mockFile.append(getOperatorName(methodInfo.name)) : mockFile.append(methodInfo.name);
                mockFile.append(PredefinedMockData::commaAndSpace);
                mockFile.append(methodInfo.returnType);
                mockFile.append(PredefinedMockData::openParentheses);
                auto calleeArgs = methodInfo.args;
                for(int j=0; j<calleeArgs.size(); j++) {
                    mockFile.append(calleeArgs[j]);
                    if(calleeArgs.size() != (j+1)) { // last arg, Do not add comma
                        mockFile.append(PredefinedMockData::commaAndSpace);
                    }
                }
                mockFile.append(PredefinedMockData::closeParentheses);
                mockFile.append(PredefinedMockData::closeParentheses);
                mockFile.append(PredefinedMockData::semicolon);
                mockFile.append(PredefinedMockData::newLine);
            }
            break;
        }
        case 4: { // FieldDeclInfo
            FieldDeclInfo fieldDeclInfo = std::get<FieldDeclInfo>(mockData.data);
            mockFile.append(PredefinedMockData::newLine);
            mockFile.append(getIndentation() + fieldDeclInfo.declName + PredefinedMockData::semicolon);
            break;
        }
    }
}

// Workaround to get operator name in string
// @FiMe: Find a way to get this information in easiest way
// Input: Operator+
// Output: OperatorAdd
std::string GMockClassGeneratorImpl::getOperatorName(const std::string& operatorId) {
    if (operatorId == "operator+") {
        return "OperatorAdd";
    } else if (operatorId == "operator-") {
        return "OperatorSubtract";
    } else if (operatorId == "operator*") {
        return "OperatorMultiplier";
    } else if (operatorId == "operator/") {
        return "OperatorDivider";
    } else if (operatorId == "operator%") {
        return "OperatorModulo";
    } else if (operatorId == "operator^") {
        return "OperatorBitWiseXOR";
    } else if (operatorId == "operator&") {
        return "OperatorAnd";
    } else if (operatorId == "operator|") {
        return "OperatorOR";
    } else if (operatorId == "operator~") {
        return "OperatorTilde";
    } else if (operatorId == "operator!") {
        return "OperatorNot";
    } else if (operatorId == "operator=") {
        return "OperatorEqual";
    } else if (operatorId == "operator<") {
        return "OperatorLesser";
    } else if (operatorId == "operator>") {
        return "OperatorGreater";
    } else if (operatorId == "operator+=") {
        return "OperatorAdditionAssignment";
    } else if (operatorId == "operator-=") {
        return "OperatorSubAssignment";
    } else if (operatorId == "operator*=") {
        return "OperatorMultiAssign";
    } else if (operatorId == "operator/=") {
        return "OperatorDivideAssign";
    } else if (operatorId == "operator%=") {
        return "OperatorModuloAssign";
    } else if (operatorId == "operator^=") {
        return "OperatorXORAssign";
    } else if (operatorId == "operator&=") {
        return "OperatorAndAssign";
    } else if (operatorId == "operator|=") {
        return "OperatorORAssign";
    } else if (operatorId == "operator<<") {
        return "OperatorLeftShift";
    } else if (operatorId == "operator>>") {
        return "OperatorRightShift";
    } else if (operatorId == "operator<<=") {
        return "OperatorLeftShiftAssign";
    } else if (operatorId == "operator>>=") {
        return "OperatorRightShiftAssign";
    } else if (operatorId == "operator==") {
        return "OperatorEquality";
    } else if (operatorId == "operator!=") {
        return "OperatorNotQual";
    } else if (operatorId == "operator<=") {
        return "OperatorLesserEqual";
    } else if (operatorId == "operator>=") {
        return "OperatorGreaterEqual";
    } else if (operatorId == "operator&&") {
        return "OperatorLogicalAND";
    } else if (operatorId == "operator||") {
        return "OperatorLogicalOR";
    } else if (operatorId == "operator++") {
        return "OperatorLogicalPlus";
    } else if (operatorId == "operator--") {
        return "OperatorLogicalMinus";
    } else if (operatorId == "operator,") {
        return "OperatorComma";
    } else if (operatorId == "operator->*") {
        return "OperatorPointerAccess";
    } else if (operatorId == "operator->") {
        return "OperatorPointerToMemberAccess";
    } else if (operatorId == "operator()") {
        return "OperatorFuncationCall";
    } else if (operatorId == "operator[]") {
        return "OperatorArraySubscript";
    } else if (operatorId == "operator new") {
        return "OperatorNew";
    } else if (operatorId == "operator delete") {
        return "OperatorDelete";
    } else if (operatorId == "operator new[]") {
        return "OperatorNewArray";
    } else if (operatorId == "operator delete[]") {
        return "OperatorDeleteArraySubscript";
    }

    return "operator_UNKNOWN";
}

std::string GMockClassGeneratorImpl::getIndentation() {
    std::string indentationString = {};
    for(int i=0; i < indentation; i++) {
        indentationString += PredefinedMockData::tab;
    }

    return indentationString;
}

void GMockClassGeneratorImpl::constructCFunction(const std::string& fileName, const std::list<MethodInfo>& cFunctions) {

    mockFile.append(PredefinedMockData::newLine);

    // Add wrapper class for C functions, because C functions can't be directly mocked
    mockFile.append(getIndentation() + "// Wrapper class for mocking C functions");
    mockFile.append(PredefinedMockData::newLine);
    mockFile.append(getIndentation() + PredefinedMockData::class_);
    mockFile.append(getClassNameFromFileName(convertDashToUnderScore(getfileNameFromPath(fileName))) + PredefinedMockData::wrapperClassForCFunctions);
    mockFile.append(PredefinedMockData::aSpace + PredefinedMockData::openBraces + PredefinedMockData::newLine);
    mockFile.append(getIndentation() + PredefinedMockData::public_);

    indentation++;

    // Construct mock methods
    for(const auto& methodInfo : cFunctions) {
        const auto mockFuncName = generateMockFunctionNameFromArgsCount(methodInfo.args.size());
        mockFile.append(getIndentation() + mockFuncName);
        mockFile.append(PredefinedMockData::openParentheses);
        mockFile.append(methodInfo.name);
        mockFile.append(PredefinedMockData::commaAndSpace);
        mockFile.append(methodInfo.returnType);
        mockFile.append(PredefinedMockData::openParentheses);
        auto methodArgs = methodInfo.args;
        for(int j=0; j<methodArgs.size(); j++) {
            mockFile.append(methodArgs[j]);
            if(methodArgs.size() != (j+1)) { // last arg, Do not add comma
                mockFile.append(PredefinedMockData::commaAndSpace);
            }
        }
        mockFile.append(PredefinedMockData::closeParentheses);
        mockFile.append(PredefinedMockData::closeParentheses);
        mockFile.append(PredefinedMockData::semicolon);
        mockFile.append(PredefinedMockData::newLine);
    }

    indentation--;

    // End the class
    mockFile.append(getIndentation() + PredefinedMockData::closeBraces);
    mockFile.append(PredefinedMockData::semicolon);
    mockFile.append(PredefinedMockData::newLine);

    // Create pointer of above class for accessing in C functions
    mockFile.append(PredefinedMockData::newLine);
    mockFile.append(getIndentation() + getClassNameFromFileName(convertDashToUnderScore(getfileNameFromPath(fileName))) + PredefinedMockData::wrapperClassForCFunctions);
    mockFile.append(PredefinedMockData::pointer + PredefinedMockData::aSpace + PredefinedMockData::thisPtr + PredefinedMockData::wrapperClassForCFunctions);
    mockFile.append(PredefinedMockData::initialization);
    mockFile.append(PredefinedMockData::newLine + PredefinedMockData::newLine);

    mockFile.append(getIndentation() + "// C functions which internally use above class for mocking");
    mockFile.append(PredefinedMockData::newLine);

    mockFile.append(getIndentation() + PredefinedMockData::extern_ + "\"C\"" + PredefinedMockData::aSpace);
    mockFile.append(PredefinedMockData::openBraces);

    // Define actual C functions
    for(const auto& methodInfo : cFunctions) {
        mockFile.append(PredefinedMockData::newLine);
        mockFile.append(getIndentation() + methodInfo.returnType);
        mockFile.append(PredefinedMockData::aSpace);
        mockFile.append(methodInfo.name);
        mockFile.append(PredefinedMockData::openParentheses);
        int argsSize = 0;
        for(const auto& arg : methodInfo.args) {
            mockFile.append(arg + " ");
            ++argsSize;
            mockFile.append(std::string("arg") + std::to_string(argsSize));
            if(methodInfo.args.size() != argsSize) {
                mockFile.append(PredefinedMockData::comma + " ");
            }
        }
        mockFile.append(PredefinedMockData::closeParentheses);
        mockFile.append(PredefinedMockData::aSpace + PredefinedMockData::openBraces);
        mockFile.append(PredefinedMockData::newLine);
        mockFile.append(getIndentation() + PredefinedMockData::tab + PredefinedMockData::return_);
        mockFile.append(PredefinedMockData::thisPtr + PredefinedMockData::wrapperClassForCFunctions);
        mockFile.append(PredefinedMockData::pointer_access);
        mockFile.append(methodInfo.name);
        mockFile.append(PredefinedMockData::openParentheses);
        for(int i=1; i<=methodInfo.args.size(); i++) {
            mockFile.append(std::string("arg") + std::to_string(i));
            if(methodInfo.args.size() != i) {
                mockFile.append(PredefinedMockData::comma + PredefinedMockData::aSpace);
            }
        }
        mockFile.append(PredefinedMockData::closeParentheses);
        mockFile.append(PredefinedMockData::semicolon + PredefinedMockData::newLine);
        mockFile.append(getIndentation() + PredefinedMockData::closeBraces);
    }

    // Close "extern" braces
    mockFile.append(PredefinedMockData::newLine);
    mockFile.append(getIndentation() + PredefinedMockData::closeBraces);
    mockFile.append(PredefinedMockData::newLine);
}