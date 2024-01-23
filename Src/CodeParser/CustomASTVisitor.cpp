/**
  * @file: CustomASTVisitor.cpp
  * @brief: The CustomASTVisitor class implements the visitor methods from clang::ASTVisitor.
  *         And gather information about methods and declarations from AST.
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

#include "CustomASTVisitor.hpp"
#include "CustomFrontendAction.hpp"
#include "CustomASTConsumer.hpp"

CustomASTVisitor::CustomASTVisitor(clang::ASTContext& ASTContext, clang::SourceManager& sourceManager)
    : m_ASTContext(ASTContext)
    , m_sourceManager(sourceManager) {

    // Truncate and open the log file
    logFile.open("AutoDepMocker.log", std::ofstream::out | std::ofstream::trunc);
    std::cout << "\33[1;35m\nInteractive mode provides the flexibility to select which files to mock based on your preferences" << std::endl;
    std::cout << "So would you like to execute in interative mode?[y/n]\033[0m: ";
    std::string input;
    std::cin >> input;
    if(std::string("y") == input) {
        askUserConfirmation = true;
    } else {
        askUserConfirmation = false;
    }
}

CustomASTVisitor::~CustomASTVisitor() {
    logFile.close();
}

// It supports parsing C/C++ Enums
// @ToDo: Add support for C++, C declaration
bool CustomASTVisitor::VisitDeclRefExpr(const clang::DeclRefExpr* declRefExpr) {

    logFile << "INFO: VisitDeclRefExpr: " << declRefExpr->getNameInfo().getAsString() << std::endl;

    // operator overload functions show up as C function
    if(std::string::npos != declRefExpr->getNameInfo().getAsString().find("operator")) {
        logFile << "INFO: Operator overload function found in VisitDeclRefExpr, skipping" << std::endl;
        return true;
    }

    // BaseType Identifier is only meant for user defined types, including std types
    // int, char, ... does not have base type
    // Also C functions do not have basetype, So this would collapse incase of c file
    const clang::ValueDecl* valueDecl = declRefExpr->getDecl();
    if(! valueDecl) {
        logFile << "INFO: Unable to find declaration, Skipping" << std::endl;
        return false;
    }

    auto* baseType = valueDecl->getType().getBaseTypeIdentifier();
    if(! baseType) {
        logFile << "INFO: build in type found, Skipping" << std::endl;
        return true;
    }

    const clang::Type* declType = declRefExpr->getType().getTypePtr();
    if(! declType) {
        logFile << "WARN: Unable to get declaration type" << std::endl;
        return true;
    }

    // Parse enum
    if(declType->isEnumeralType() || declType->isScopedEnumeralType()) {
        parseEnum(declRefExpr, declType);
    }

    return true; // Parse only C/C++ Enum types
}

// This gets called first for all callExpression
bool CustomASTVisitor::VisitCallExpr(clang::CallExpr* callExpression) {

    // @Note: getDirectCallee() seems to return nullptr for cast expression. But callExpression->getCalleeDecl() would work
    // However cast expressions can be skipped
    if(! callExpression->getDirectCallee()) {
        logFile << "WARN: Suspecious CallExpr found, Skipping" << std::endl;
        return true;
    }
    logFile << "INFO: VisitCallExpr, callee " << callExpression->getDirectCallee()->getNameAsString().c_str() << std::endl;

    clang::Expr* calleeExpr = callExpression->getCallee();
    if(! calleeExpr) {
        logFile << "WARN: Unable to get callee from call expression" << std::endl;
        return true;
    }

    if(clang::Expr::LValueClassification::LV_MemberFunction == calleeExpr->ClassifyLValue(m_ASTContext)) {
        parseCXXMemberExpression(callExpression);
    } else {
        parseCFunction(callExpression);
    }

    return true;
}

// Getter function for getting C++ class and methods information
std::tuple<ClassInfoType, ClassMethodInfoType> CustomASTVisitor::getMockclassInfoAndMethods() {
    return {m_mockClassInfo, m_mockCPPMethodInfo};
}

// Getter function for getting C functions information
const CFunctionInfoType& CustomASTVisitor::getCMockFunctions() {
    return m_CFunctionInfo;
}

// Getter function for getting C and C++ Enums
const EnumInfo& CustomASTVisitor::getEnumInfo() {
    return m_enumInfo;
}

// Getter function for getting include files information
const IncludeInfo& CustomASTVisitor::getIncludeInfo() {
    return m_includes;
}

// Parse C++ member expression and store class-method information
void CustomASTVisitor::parseCXXMemberExpression(clang::CallExpr* callEpr) {

    if(! callEpr) {
        logFile << "WARN: Invalid call expression" << std::endl;
        return;
    }

    clang::CXXMethodDecl* methodDecl = clang::dyn_cast<clang::CXXMethodDecl>(callEpr->getDirectCallee());
    if(! methodDecl) {
        logFile << "WARN: Unable to get CXXMethodDeclaration from callee" << std::endl;
        return;
    }

    StoreClassAndMethodInfo(clang::dyn_cast<clang::CXXMethodDecl>(callEpr->getDirectCallee()));
}

// VisitCallExpr, callee read
// ImplicitCastExpr 0x44970c8 'ssize_t (*)(int, void *, size_t)' <FunctionToPointerDecay>
// `-DeclRefExpr 0x4497050 'ssize_t (int, void *, size_t)' lvalue Function 0x3d9e0b8 'read' 'ssize_t (int, void *, size_t)'
void CustomASTVisitor::parseCFunction(clang::CallExpr* callExpr) {

    if(! callExpr || ! callExpr->getDirectCallee()) {
        logFile << "WARN: callExpr or callee is invalid" << std::endl;
        return;
    }

    // @Note: operator overload functions show up as C function
    if(callExpr->getDirectCallee()->isOverloadedOperator()) { // Right way
        logFile << "INFO: Operator overload function found" << std::endl;
        clang::CXXMethodDecl* cxxMethodDec = clang::dyn_cast_or_null<clang::CXXMethodDecl>(callExpr->getDirectCallee()->getAsFunction());
        if(cxxMethodDec) {
            ParseOperatorOverloading(cxxMethodDec);
        }
        return;
    }

    clang::Expr* calleeExpr = callExpr->getCallee();
    if(! calleeExpr) {
        logFile << "WARN: Unable to get callee from call expression" << std::endl;
        return;
    }

    // Get Declaration from callee Expression
    clang::Decl* refDecl = calleeExpr->getReferencedDeclOfCallee();
    if(! refDecl) {
        logFile << "WARN: Unable to cast to reference declaration" << std::endl;
        return;
    }

    // Get function declaration from declaration
    clang::FunctionDecl* functionDecl = refDecl->getAsFunction();
    if(! functionDecl) {
        logFile << "WARN: Unable to cast to function declaration" << std::endl;
        return;
    }

    // Functions originating from the same source file
    const std::string SourcefileName = getfileNameFromPath(m_sourceManager.getFileEntryForID(m_sourceManager.getMainFileID())->getName());
    const std::string currentFileName = getfileNameFromPath(m_sourceManager.getFilename(functionDecl->getLocation())).data();
    std::string currentFileNameStripped = currentFileName.substr(0, currentFileName.find(".")); // To include header file as well
    logFile << "INFO: File stripped: " << currentFileNameStripped << std::endl;
    if(std::string::npos != SourcefileName.find(currentFileNameStripped)) {
        logFile << "INFO: Source file function found, skipping" << std::endl;
        return;
    }

    // Is it from c++ std
    if(functionDecl->getDeclContext()->isStdNamespace()) {
        logFile << "INFO: std function found, skipping" << std::endl;
        return;
    }

    // @Note: Sometimes data shows up empty
    if(! m_sourceManager.getFilename(functionDecl->getLocation()).data()) {
        logFile << "WARN: Couldn't find file name, skipping" << std::endl;
        return;
    }

    std::string fileName = getfileNameFromPath(m_sourceManager.getFilename(functionDecl->getLocation()).data());
    if(! fileContentToBeMocked(m_sourceManager.getFilename(functionDecl->getLocation()).data(), functionDecl->getNameAsString())) {
        logFile << "INFO: Not mocking - " << functionDecl->getNameAsString() << std::endl;
        return;
    }

    // If new entry, Reserve a place
    if(! m_CFunctionInfo.count(fileName)) {
        m_CFunctionInfo[fileName] = {};
    }

    // Is function information already noted
    for(const auto& each : m_CFunctionInfo.at(fileName)) {
        if(each.name == functionDecl->getNameAsString()) {
            logFile << "INFO: Function information already present, Skipping" << std::endl;
            return;
        }
    }

    // Finally store it
    MethodInfo methodInfo;
    methodInfo.name = functionDecl->getNameAsString();
    methodInfo.returnType = checkBool(functionDecl->getReturnType().getAsString());
    logFile << "INFO: Store the file name of return type defined: " << methodInfo.returnType.c_str() << std::endl;
    storeIncludeInformation(const_cast<clang::Type*>(functionDecl->getReturnType().getTypePtr()), fileName);
    // get args
    std::vector<std::string> argsInfo;
    for(int i=0; i<functionDecl->getNumParams(); i++) {
        argsInfo.push_back(checkBool(functionDecl->getParamDecl(i)->getType().getAsString()));
        logFile << "INFO: Store the file name of function arg defined: " <<
                   functionDecl->getParamDecl(i)->getType().getAsString().c_str() << std::endl;
        storeIncludeInformation(const_cast<clang::Type*>(functionDecl->getParamDecl(i)->getType().getTypePtr()), fileName);
    }
    methodInfo.args = argsInfo;

    // store C function information with fileName(key)
    m_CFunctionInfo[fileName].push_back(methodInfo);
}

// Parse C and C++ scoped enum types
// Example: enum name { ONE, TWO }; || enum class name { ONE, TWO };
void CustomASTVisitor::parseEnum(const clang::DeclRefExpr* declRefExpr, const clang::Type* declType/*helper*/) {
    if(! declRefExpr || ! declRefExpr->getDecl()) {
        logFile << "WARN: Invalid declaration reference expression" << std::endl;
        return;
    }

    // @Note: Using clang::NamespaceDecl is the proper way, But unfortunatly it didn't work
    //        Finding the right declaration might help to resolve this issue. Until that we can use the below way
    const std::string enumFullName = declRefExpr->getDecl()->getType().getUnqualifiedType().getDesugaredType(m_ASTContext).
            getAsString().substr(5); // Remove "Enum" keyword

    // Check declaration belonging to Main file
    const std::string sourceFileName = getfileNameFromPath(m_sourceManager.getFileEntryForID(m_sourceManager.getMainFileID())->getName());
    const std::string currentfileName = getfileNameFromPath(m_sourceManager.getFilename(declRefExpr->getDecl()->getLocation()).data());
    const std::string fileNameStripped = currentfileName.substr(0, currentfileName.find("."));
    if(std::string::npos != sourceFileName.find(fileNameStripped)) {
        logFile << "INFO: Enum belonging to Main source file, Skipping" << std::endl;
        return;
    }

    const std::string enumNameFound = getEnumNameFromFullyQualifiedEnumName(enumFullName);
    logFile << "INFO: Enum name" << enumNameFound << ", Enum full name: " << enumFullName << std::endl;
    const std::string valueFound = declRefExpr->getNameInfo().getAsString();

    bool isEnumStored = false;
    bool isEnumValueStored = false;
    enumProperties* enumRef = {};

    if(m_enumInfo.count(currentfileName)) {
        // File name already stored
        for(auto& eachEnumStored : m_enumInfo.at(currentfileName)) {
            if(eachEnumStored.enumName == enumNameFound) {
                // Enum name already stored
                isEnumStored = true;
                enumRef = &eachEnumStored;
                // Traverse each enum value
                for(const auto enumValueStored : eachEnumStored.enumValues) {
                    if(enumValueStored == valueFound) {
                        // Enum value already stored
                        isEnumValueStored = true;
                    }
                }
                // New enum value in existing enum, Store it
                if(! isEnumValueStored) {
                    eachEnumStored.enumValues.push_back(valueFound);
                    return;
                }
                break;
            }
        }
    }

    // Enum and Enum value already stored, Nothing to do
    if(isEnumStored && isEnumValueStored) {
        logFile << "INFO: Enum value is already stored, skipping" << std::endl;
        return;
    }

    // Enum and enum value are not stored yet
    if(! fileContentToBeMocked(m_sourceManager.getFilename(declRefExpr->getDecl()->getLocation()).data(), enumNameFound)) {
        return; // Not mocking as user not interested
    }

    // Make entry in global enum
    if(! m_enumInfo.count(currentfileName)) {
        m_enumInfo[currentfileName] = {};
    }

    if(! isEnumStored) {
        enumProperties properties = {};
        properties.enumName = enumNameFound;
        properties.enumFullName = enumFullName;
        properties.enumValues.push_back(valueFound);
        properties.isScopedEnum = declType->isScopedEnumeralType();
        m_enumInfo.at(currentfileName).push_back(properties);
    } else {
        enumRef->enumValues.push_back(valueFound);
    }
    return;
}

void CustomASTVisitor::ParseOperatorOverloading(clang::CXXMethodDecl* cxxMethodDec) {
    if(! cxxMethodDec->getParent()) {
        logFile << "WARN: Unable to get parent of CXXMethodDecl" << std::endl;
        return;
    }

    StoreClassAndMethodInfo(cxxMethodDec, true/*operator overloading*/);
}

void CustomASTVisitor::StoreClassAndMethodInfo(clang::CXXMethodDecl* methodDecl, bool operatorOverloadingType) {
    if(! methodDecl || ! methodDecl->getParent()) {
        logFile << "WARN: Unable to get parent declaration of CXXMethodDecl" << std::endl;
        return;
    }

    // @Note: Fetching FileId didn't work. So workaround has been added to get file name and compare against the source file
    const std::string SourcefileName = getfileNameFromPath(m_sourceManager.getFileEntryForID(m_sourceManager.getMainFileID())->getName());
    const std::string currentFileName = getfileNameFromPath(m_sourceManager.getFilename(methodDecl->getParent()->getLocation())).data();
    std::string currentFileNameStripped = currentFileName.substr(0, currentFileName.find("."));
    logFile << "INFO: File stripped: " << currentFileNameStripped << std::endl;
    if(std::string::npos != SourcefileName.find(currentFileNameStripped)) {
        logFile << "INFO: Source class member function found, skipping" << std::endl;
        return;
    }

    const std::string className = methodDecl->getParent()->getNameAsString();

    if(! fileContentToBeMocked(m_sourceManager.getFilename(methodDecl->getParent()->getLocation()).data(), className)) {
        return;
    }

    ClassInfo classInfo = {};
    classInfo.name = methodDecl->getParent()->getNameAsString();
    classInfo.fullName = methodDecl->getParent()->getQualifiedNameAsString(); // Useless

    // Read Namespace information
    const clang::DeclContext* declContext = methodDecl->getParent()->getEnclosingNamespaceContext();
    if (const clang::NamespaceDecl* namespaceDecl = clang::dyn_cast<clang::NamespaceDecl>(declContext)) {
        // Make sure namespace information is stored in the right order
        classInfo.namespaceInfo.insert(classInfo.namespaceInfo.begin(), namespaceDecl->getNameAsString());
        // If there are parent namespaces, Add them too
        const clang::DeclContext* parentDeclContext = namespaceDecl->getParent();
        while (parentDeclContext && clang::isa<clang::NamespaceDecl>(parentDeclContext)) { // Loop over to fetch namespace information
            namespaceDecl = clang::cast<clang::NamespaceDecl>(parentDeclContext);
            classInfo.namespaceInfo.insert(classInfo.namespaceInfo.begin(), namespaceDecl->getNameAsString());
            parentDeclContext = namespaceDecl->getParent();
        }
    }

    // Check if the class is template class
    if(methodDecl->getParent()->getTemplateInstantiationPattern()) {
        if(! methodDecl->getParent()->getTemplateInstantiationPattern()) {
            logFile << "WARN: Unable to get Template instantiation pattern of CXXRecordDecl" << std::endl;
            return;
        }
        if(! methodDecl->getParent()->getTemplateInstantiationPattern()->getDescribedClassTemplate()) {
            logFile << "WARN: Unable to get Described Class Template from CXXRecordDecl" << std::endl;
            return;
        }
        if(! methodDecl->getParent()->getTemplateInstantiationPattern()->getDescribedClassTemplate()->getTemplateParameters()) {
            logFile << "WARN: Unable to get Template parameter list from ClassTempDecl" << std::endl;
            return;
        }
        clang::TemplateParameterList* templateParamList = methodDecl->getParent()->getTemplateInstantiationPattern()->getDescribedClassTemplate()->getTemplateParameters();
        std::vector<std::string> tempParamList;
        for(int i=0; i<templateParamList->size(); i++) {
            tempParamList.push_back(templateParamList->getParam(i)->getNameAsString());
        }

        classInfo.isTemplateClass = true;
        classInfo.templateParams = tempParamList;
    }

    classInfo.filename = getfileNameFromPath(m_sourceManager.getFilename(methodDecl->getParent()->getLocation()));
    logFile << "INFO: Filename: " << classInfo.filename << std::endl;
    logFile << "INFO: Class full name: " << classInfo.fullName << std::endl;

    m_mockClassInfo[classInfo.name] = classInfo;

    // Store callee information, RVALUE
    if(! m_mockCPPMethodInfo.count(classInfo.name)) {
        m_mockCPPMethodInfo[classInfo.name] = {};
    }
    MethodInfo methodInfo = {};
    clang::FunctionDecl* functionDecl = {};
    if(classInfo.isTemplateClass) { // UnWrap the template instance
        functionDecl = methodDecl->getTemplateInstantiationPattern();
    } else {
        functionDecl = clang::dyn_cast_or_null<clang::FunctionDecl>(methodDecl);
    }
    if(! functionDecl) {
        logFile << "WARN: Unable to get function declaration for member expression" << std::endl;
        return;
    }

    // Is Method information already stored
    for(const auto& eachMethod : m_mockCPPMethodInfo.at(classInfo.name)) {
        if(eachMethod.name == functionDecl->getNameAsString()) {
            // Fill up arguments for comparision
            std::vector<std::string> argsInfo;
            for(int i=0; i<functionDecl->getNumParams(); i++) {
                argsInfo.push_back(checkBool(functionDecl->getParamDecl(i)->getType().getAsString())); // Decl always has a type
            }
            if(argsInfo == eachMethod.args) {
                logFile << "INFO: callee Information is already present, skipping" << std::endl;
                return;
            }
        }
    }

    // Store method information
    methodInfo.name = functionDecl->getNameAsString();
    methodInfo.returnType = checkBool(functionDecl->getReturnType().getAsString());
    logFile << "INFO: Store the file name of return type defined: " << methodInfo.returnType.c_str() << std::endl;
    storeIncludeInformation(const_cast<clang::Type*>(functionDecl->getReturnType().getTypePtr()), classInfo.filename);
    methodInfo.isConst = methodDecl->isConst();
    methodInfo.isTemplated = functionDecl->isTemplated();
    std::vector<std::string> argsInfo;
    for(int i=0; i<functionDecl->getNumParams(); i++) {
        argsInfo.push_back(checkBool(functionDecl->getParamDecl(i)->getType().getAsString())); // Decl always has a type
        logFile << "INFO: Store the file name of function arg defined: " << functionDecl->getParamDecl(i)->getType().getAsString().c_str() << std::endl;
        storeIncludeInformation(const_cast<clang::Type*>(functionDecl->getParamDecl(i)->getType().getTypePtr()), classInfo.filename);
    }
    methodInfo.args = argsInfo;
    methodInfo.isOperatorOverloading = operatorOverloadingType;

    // Finally link callee information with caller
    m_mockCPPMethodInfo[classInfo.name].push_back(methodInfo);
}

// Workaround to convert _Bool to bool
// Clang reports bool type as _Bool
std::string CustomASTVisitor::checkBool(const std::string typeName) {
    if(std::string("_Bool") == typeName) {
        return "bool";
    }
    return typeName;
}

void CustomASTVisitor::storeIncludeInformation(clang::Type* type, const std::string fileName) {
    std::optional<std::string> includeFileName = getFileNameFromTypeDeclaration(type);
    if(includeFileName.has_value()) {
        if(! m_includes.count(fileName)) { // First include in the file
            m_includes[fileName] = {};
        }

        // @Note: It appears that there is no straightforward method to determine the precise C++ standard
        // header file usually used corresponding to each standard declaration
        // Example: std::string -> /usr/include/c++/string
        // So workaround has been added to include below std file to cover any std file
        if(std::string::npos != includeFileName.value().find("c++/")) {
            // Change the include filename to <bits/stdc++.h>
            includeFileName.value() = "bits/stdc++.h";
        }

        // Is include file already noted
        bool found = false;
        for(const auto includeFile : m_includes.at(fileName)) {
            if(includeFile == includeFileName.value()) {
                found = true;
            }
        }
        if(! found) {
            m_includes.at(fileName).push_back(includeFileName.value());
        }
    }
}

// Returns fileName where the given type is defined
// What are all the type possible ?
//   -> Pointer type, Reference Type, Type
// Limitation: Some std files are defined in different location and gets included
//             from a wrapper file(ex: string -> basic_string.h)
//             WorkAround: Include bits/stdc++.h to simply include everything
std::optional<std::string> CustomASTVisitor::getFileNameFromTypeDeclaration(clang::Type* type) {
    if(! type) {
        logFile << "WARN: Type is empty, Unable to process. Skipping" << std::endl;
        return {};
    }
    if(type->isBuiltinType()) {
        logFile << "INFO: Build in type found, Skipping" << std::endl;
        return {};
    }

    // Type could be pointer, reference or pure type
    // Below code unwraps pointer and reference type to pure type
    clang::Type* typePtr = {};

    if(type->isReferenceType()) {
        const clang::ReferenceType* referType = type->getAs<clang::ReferenceType>();
        if(! referType) {
            logFile << "WARN: Unable to get reference type from type" << std::endl;
            return {};
        }
        typePtr = const_cast<clang::Type*>(referType->getPointeeType().getTypePtr());
        if(! typePtr) {
            logFile << "WARN: Unable to get type pointer from pointee type" << std::endl;
            return {};
        }
    } else if (type->isPointerType()) {
        const clang::PointerType* pointerType = type->getAs<clang::PointerType>();
        if(! pointerType) {
            logFile << "WARN: Unable to get pointer type from type" << std::endl;
            return {};
        }
        typePtr = const_cast<clang::Type*>(pointerType->getPointeeType().getTypePtr());
        if(! typePtr) {
            logFile << "WARN: Unable to get type pointer from pointee type" << std::endl;
            return {};
        }
    }

    if(typePtr) {
        // Finally get declaration tagged with type
        const clang::TagType* tagType = typePtr->getAs<clang::TagType>();
        if(! tagType) {
            logFile << "WARN: Unable to get tag type from type pointer" << std::endl;
            logFile << "WARN: Is in build type: " << typePtr->isBuiltinType() << std::endl;
            return {};
        }

        if(! tagType->getDecl()) {
            logFile << "WARN: Unable to get declaration from tag type" << std::endl;
            return {};
        }

        // File location would show up invalid for unknown files
        if(! m_sourceManager.getFilename(tagType->getDecl()->getLocation()).data()) {
            logFile << "WARN: Unable to get file location from tag type declaration" << std::endl;
            return {};
        }

        return getStrippedFilePath(m_sourceManager.getFilename(tagType->getDecl()->getLocation()).data());
    }

    // Not wrapped
    const clang::TagType* tagType = type->getAs<clang::TagType>();
    if(! tagType) {
        logFile << "WARN: Unable to get tag type from type" << std::endl;
        return {};
    }

    if(! tagType->getDecl()) {
        logFile << "WARN: Unable to get declaration from tag type" << std::endl;
        return {};
    }

    // File location would show up invalid for unknown files
    if(! m_sourceManager.getFilename(tagType->getDecl()->getLocation()).data()) {
        logFile << "WARN: Unable to get file location from tag type declaration" << std::endl;
        return {};
    }

    return getStrippedFilePath(m_sourceManager.getFilename(tagType->getDecl()->getLocation()).data());
}

// Utility function to remove "/usr/include"
// Input: /usr/include/Header.hpp
// Outpur: Header.hpp
std::string CustomASTVisitor::getStrippedFilePath(const std::string fullPath) {
    if(std::string::npos == fullPath.find("/usr/include/")) {
        return getfileNameFromPath(fullPath); // Could be project include
    }

    // Some headers might have its own directory
    // Example - /usr/include/MyInc/include.hpp
    return fullPath.substr(fullPath.find("/usr/include/") + 13); // 13 - Strip /usr/include/
}

// Used only for enum types
std::string CustomASTVisitor::getEnumNameFromFullyQualifiedEnumName(const std::string& memberType) {
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

bool CustomASTVisitor::isStdNamespace(const std::string namespaceInfo) {
    return (std::string::npos == namespaceInfo.find("std::")) ? false : true;
}

// Mock or not to Mock is decided based on the file
// Once file is choosen to not mock, Then content of that file will not be mocked in further findings
bool CustomASTVisitor::fileContentToBeMocked(const std::string& fileName, const std::string& className) {

    // include/c++/7.5.0 => c++ std files
    // Already user confirmed files

    // Do mock
    for(const auto& each : tobeMockedFiles) {
        int pos = 0;
        pos = fileName.find(each);
        if(std::string::npos != pos) {
            // File content can be mocked
            return true;
        }
    }

    // Do not mock
    for(const auto& each : notTobeMockedFiles) {
        int pos = 0;
        pos = fileName.find(each);
        if(std::string::npos != pos) {
            // File content should not be mocked
            return false;
        }
    }

    // New file found, Ask user
    logFile << "INFO: To be mocked? fileName: " << fileName << ", className: " << className << "" << std::endl;
    std::string input = "y";
    if(askUserConfirmation) {
        static bool askOnce = false;
        if(! askOnce) {
            askOnce = true;
            std::cout << "\n\33[1;43mBelow are the list of files identified as dependencies to your source file\033[0m\n";
            std::cout << "\33[1;43mSo press \"y\" if you want to mock the file content, \"n\" otherwise\033[0m\n" << std::endl;
        }
        std::cout << "\33[1m" <<fileName << "(" << className << "): \033[0m";
        std::cin >> input;
        std::cout << std::endl;
    }
    if(std::string("y") == input) {
        tobeMockedFiles.push_back(std::string(fileName));
        return true;
    }
    notTobeMockedFiles.push_back(std::string(fileName));
    return false;
}

std::string CustomASTVisitor::getfileNameFromPath(const std::string& filePath) {
        return std::filesystem::path(filePath).filename();
}
