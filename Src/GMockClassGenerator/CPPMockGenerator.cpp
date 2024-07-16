/**
  * @file: CPPMockGenerator.cpp
  * @brief: The CPPMockGenerator creates a mock class based on GMOCK, along with a wrapper class for operator overloading.
  *         Additionally, it supports template classes.
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

#include "CPPMockGenerator.hpp"

void CPPMockGenerator::constructClass(const ClassInfo& classInfo, const std::vector<MethodInfo>& calleeInfo) {
    if(isFileInfoRequired(classInfo.filename)) {
        mockClass.append(addIncludeGuard(classInfo.name));
    }

    // Wrapper class for operator overloading function
    constructWrapperClass(classInfo, calleeInfo);

    // Add namespace
    const std::vector<std::string> namespaceInfo = getNamespaceInfofromfullyQualifiedClassName(classInfo.fullName);
    if(classInfo.namespaceInfo.size()) {
        //mockClass.append(PredefinedMockData::newLine);
        mockClass.append(PredefinedMockData::newLine);
        for(int i=0; i<classInfo.namespaceInfo.size(); i++) {
            mockClass.append(PredefinedMockData::nameSpace); // namespace
            mockClass.append(classInfo.namespaceInfo[i] + PredefinedMockData::aSpace); // namespace Name
            mockClass.append(PredefinedMockData::openBraces); // namespace Name{
            mockClass.append(PredefinedMockData::newLine);
        }
    }

    // Add template class information
    if(classInfo.isTemplateClass && classInfo.templateParams.size()) {
        mockClass.append(PredefinedMockData::newLine + std::string("// Template mock class"));
        mockClass.append(PredefinedMockData::newLine + PredefinedMockData::template_ + PredefinedMockData::angleBracketOpen);
        for(int i=0; i<classInfo.templateParams.size(); i++) {
            mockClass.append(PredefinedMockData::typename_ + classInfo.templateParams[i]);
            if((i + 1) == classInfo.templateParams.size()) { // End
                mockClass.append(PredefinedMockData::angleBracketClose);
                break;
            }
            mockClass.append(PredefinedMockData::comma + PredefinedMockData::aSpace);
        }
    }

    // Add class
    mockClass.append(PredefinedMockData::newLine);
    mockClass.append(classInfo.declKindName);
    mockClass.append(classInfo.name);
    mockClass.append(PredefinedMockData::aSpace);
    mockClass.append(PredefinedMockData::openBraces);
    mockClass.append(PredefinedMockData::newLine);
    mockClass.append(PredefinedMockData::public_);

    // Add static method - getInstance()
    mockClass.append(PredefinedMockData::tab + PredefinedMockData::static_);
    mockClass.append(classInfo.name + PredefinedMockData::getInstance);
    mockClass.append(PredefinedMockData::openParentheses + PredefinedMockData::closeParentheses);
    mockClass.append(PredefinedMockData::aSpace + PredefinedMockData::openBraces);
    mockClass.append(PredefinedMockData::newLine + PredefinedMockData::tab + PredefinedMockData::tab);
    mockClass.append(PredefinedMockData::return_ + PredefinedMockData::thisPtr + PredefinedMockData::semicolon);
    mockClass.append(PredefinedMockData::newLine);
    mockClass.append(PredefinedMockData::tab + PredefinedMockData::closeBraces);
    mockClass.append(PredefinedMockData::newLine + PredefinedMockData::newLine);
    mockClass.append(PredefinedMockData::tab + PredefinedMockData::static_);
    mockClass.append(classInfo.name + PredefinedMockData::pointer);
    mockClass.append(PredefinedMockData::thisPtr + PredefinedMockData::semicolon);
    mockClass.append(PredefinedMockData::newLine + PredefinedMockData::newLine);

    // Add mock methods
    if(calleeInfo.size()) {
        for(int i=0; i<calleeInfo.size(); i++) {
            if(! calleeInfo[i].isOperatorOverloading) {
                const auto mockFuncName = generateMockFunctionNameFromArgsCount(calleeInfo[i].args.size(), calleeInfo[i].isConst, calleeInfo[i].isTemplated);
                mockClass.append(PredefinedMockData::tab);
                mockClass.append(mockFuncName);
                mockClass.append(PredefinedMockData::openParentheses);
                mockClass.append(calleeInfo[i].name);
                mockClass.append(PredefinedMockData::commaAndSpace);
                mockClass.append(calleeInfo[i].returnType);
                mockClass.append(PredefinedMockData::openParentheses);
                auto calleeArgs = calleeInfo[i].args;
                for(int j=0; j<calleeArgs.size(); j++) {
                    mockClass.append(calleeArgs[j]);
                    if(calleeArgs.size() != (j+1)) { // last arg, Do not add comma
                        mockClass.append(PredefinedMockData::commaAndSpace);
                    }
                }
                mockClass.append(PredefinedMockData::closeParentheses);
                mockClass.append(PredefinedMockData::closeParentheses);
                mockClass.append(PredefinedMockData::semicolon);
                mockClass.append(PredefinedMockData::newLine);
            }
        }

        // Write operator overloading functions atlast
        // Reason for writing it sepeartly is to make sure opertor overloading functions are grouped together
        for(int i=0; i<calleeInfo.size(); i++) {
            if(calleeInfo[i].isOperatorOverloading) {
                mockClass.append(PredefinedMockData::newLine);
                mockClass.append(PredefinedMockData::tab);
                mockClass.append(calleeInfo[i].returnType + PredefinedMockData::aSpace);
                mockClass.append(calleeInfo[i].name + PredefinedMockData::openParentheses);
                auto calleeArgs = calleeInfo[i].args;
                for(int j=0; j<calleeArgs.size(); j++) {
                    mockClass.append(calleeArgs[j]);
                    mockClass.append(std::string(" arg") + std::to_string(j+1));
                    if(calleeArgs.size() != (j+1)) { // last arg, Do not add comma
                        mockClass.append(PredefinedMockData::commaAndSpace);
                    }
                }
                mockClass.append(PredefinedMockData::closeParentheses);
                mockClass.append(PredefinedMockData::aSpace + PredefinedMockData::openBraces + PredefinedMockData::newLine);
                mockClass.append(PredefinedMockData::tab + PredefinedMockData::tab);
                mockClass.append(classInfo.name + std::string("_WrapperInstance->"));
                mockClass.append(getOperatorName(calleeInfo[i].name));
                mockClass.append(PredefinedMockData::openParentheses);
                for(int j=0; j<calleeArgs.size(); j++) {
                    mockClass.append(std::string("arg") + std::to_string(j+1));
                    if(calleeArgs.size() != (j+1)) { // last arg, Do not add comma
                        mockClass.append(PredefinedMockData::commaAndSpace);
                    }
                }
                mockClass.append(PredefinedMockData::closeParentheses + PredefinedMockData::semicolon);
                mockClass.append(PredefinedMockData::newLine + PredefinedMockData::tab + PredefinedMockData::closeBraces);
                mockClass.append(PredefinedMockData::newLine);
            }
        }
    }

    // End the class
    mockClass.append(PredefinedMockData::closeBraces + PredefinedMockData::semicolon + PredefinedMockData::newLine);

    // End the namespace
    for(int i=0; i<classInfo.namespaceInfo.size(); i++) { // i unused
        mockClass.append(PredefinedMockData::closeBraces);
        mockClass.append(PredefinedMockData::newLine);
    }

    mockClass.append(PredefinedMockData::newLine);
    mockClass.append(PredefinedMockData::newLine);

    writeToFile(classInfo.filename, mockClass);
    mockClass.clear();
}

// Function to construct wrapper class to support mocking operator overload function
void CPPMockGenerator::constructWrapperClass(const ClassInfo& classInfo, const std::vector<MethodInfo>& calleeInfo) {

    // Check if the calleeInfo contains overloaded operator function
    bool operatorOLExists = false;
    bool templateFunctionExists = false;
    for(const auto& each : calleeInfo) {
        if(each.isOperatorOverloading) {
            operatorOLExists = true;
        }
        if(each.isTemplated) {
            templateFunctionExists = true;
        }
    }
    if(! operatorOLExists) { // No need for wrapper class
        return;
    }

    // calleeInfo has operator overload functions
    // Construct wrapper class

    if(isFileInfoRequired(classInfo.filename)) {
        addIncludeGuard(classInfo.name);
    }

    // No need to have namespace for template wrapper class

    // Check if it is a Template class
    if(classInfo.isTemplateClass) {
        mockClass.append(PredefinedMockData::newLine + std::string("// Wrapper for template mock class"));
        mockClass.append(PredefinedMockData::newLine + PredefinedMockData::template_ + PredefinedMockData::angleBracketOpen);
        for(int i=0; i<classInfo.templateParams.size(); i++) {
            mockClass.append(PredefinedMockData::typename_ + classInfo.templateParams[i]);
            if((i + 1) == classInfo.templateParams.size()) { // End
                mockClass.append(PredefinedMockData::angleBracketClose);
                break;
            }
            mockClass.append(PredefinedMockData::comma + PredefinedMockData::aSpace);
        }
    }

    // Add class
    mockClass.append(PredefinedMockData::newLine);
    mockClass.append(std::string("// Wrapper class for ") + classInfo.name + std::string(" operator overloading functions"));
    mockClass.append(PredefinedMockData::newLine);
    mockClass.append(PredefinedMockData::class_);
    mockClass.append(classInfo.name + std::string("_wrapper"));
    mockClass.append(PredefinedMockData::aSpace);
    mockClass.append(PredefinedMockData::openBraces);
    mockClass.append(PredefinedMockData::newLine);
    mockClass.append(PredefinedMockData::public_);

    // Add GMOCK for operator overloading methods
    for(const auto& each : calleeInfo) {
        if(each.isOperatorOverloading) {
            const auto mockFuncName = generateMockFunctionNameFromArgsCount(each.args.size(), each.isConst, each.isTemplated);
            mockClass.append(PredefinedMockData::tab + mockFuncName);
            mockClass.append(PredefinedMockData::openParentheses);
            mockClass.append(getOperatorName(each.name));
            mockClass.append(PredefinedMockData::commaAndSpace);
            mockClass.append(each.returnType);
            mockClass.append(PredefinedMockData::openParentheses);
            auto calleeArgs = each.args;
            for(int j=0; j<calleeArgs.size(); j++) {
                mockClass.append(calleeArgs[j]);
                if(calleeArgs.size() != (j+1)) { // last arg, Do not add comma
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
    mockClass.append(PredefinedMockData::closeBraces + PredefinedMockData::semicolon + PredefinedMockData::newLine);

    // Add extern for accessing wrapper class from actual mock class
    mockClass.append(PredefinedMockData::newLine);
    mockClass.append(classInfo.name + std::string("_wrapper* "));
    mockClass.append(classInfo.name + std::string("_WrapperInstance") + PredefinedMockData::initialization);
    mockClass.append(PredefinedMockData::newLine);
}

// Workaround to get operator name in string
// @FiMe: Find a way to get this information in easiest way
// Input: Operator+
// Output: OperatorAdd
std::string CPPMockGenerator::getOperatorName(const std::string& operatorId) {
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
