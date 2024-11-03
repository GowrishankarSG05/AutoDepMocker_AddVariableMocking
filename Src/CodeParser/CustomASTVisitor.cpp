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

#include <iterator>

#include "CustomASTVisitor.hpp"
#include "CustomFrontendAction.hpp"
#include "CustomASTConsumer.hpp"
#include "Defines.hpp"

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

// Ignore buildin types, c++ std types and types which are defined in same source file
// Parse and mock only types which are defined in externel file
// Example: Foo obj; 
//          This function will be called when above line is parsed 
bool CustomASTVisitor::VisitVarDecl(clang::VarDecl* variableDecl) {
    if(! variableDecl) {
        logFile << "WARN: Invalid VarDecl handle received" << std::endl;
        return true;
    }
    logFile << "INFO: VisitVarDecl: " << variableDecl->getDeclName().getAsString() << std::endl;

    // Ignore buildin types
    if(variableDecl->getType()->isBuiltinType()) {
        logFile << "INFO: buildin type found, Skipping" << std::endl;
        return true;
    }

    // Ignore c++ std types
    if(variableDecl->isInStdNamespace()) {
        logFile << "INFO: c++ std type found, Skipping" << std::endl;
        return true;
    }

    // Skip if the declaration origin is from the same source file
    const std::string sourcefileName = getfileNameFromPath(
                m_sourceManager.getFileEntryForID(m_sourceManager.getMainFileID())->getName());
    const auto declFileName = getFileNameFromTypeDeclaration(const_cast<clang::Type*>(
                variableDecl->getType().getTypePtrOrNull())).value_or(std::string());
    if(declFileName.empty()) {
        logFile << "WARN: Unable to find declaration file name" << std::endl;
        return true;
    }

    const auto declFileNameStripped = getfileNameFromPath(declFileName);

    if(declFileNameStripped.empty()) {
        logFile << "WARN: Unable to get declaration file name, Skipping" << std::endl;
        return true;
    }

    if(sourcefileName == declFileNameStripped) {
        logFile << "INFO: Declaration origin is source file, Skipping" << std::endl;
        return true;
    }

    // Get declaration name
    std::string declName = getTypeNameFromQualifiedTypeName(
                                      variableDecl->getType().getDesugaredType(m_ASTContext).getAsString());

    if(! fileContentToBeMocked(declFileNameStripped, declName)) {
        logFile << "INFO: Not mocking file: " << declFileNameStripped << std::endl;
        return true;
    }

    // Incase if it is enum variable declaration
    if(variableDecl->getType().getTypePtrOrNull()->isEnumeralType() || 
                  variableDecl->getType().getTypePtrOrNull()->isScopedEnumeralType()) {
        parseEnum(clang::dyn_cast<clang::EnumDecl>(
                            getDeclFromType(const_cast<clang::Type*>(variableDecl->getType().getTypePtrOrNull()))));
        return true;
    }

    // Parse parent hierarchy information of variable type
    clang::DeclContext* declContext = getDeclContextFromType(const_cast<clang::Type*>(variableDecl->getType().getTypePtrOrNull()));
    if(! declContext) {
        logFile << "WARN: Unable to read decl context from type" << std::endl;
        return true;
    }

    processDeclContextHierarchy(declContext, declFileNameStripped);

    return true;
}

// It supports parsing C/C++ Enums
// Need not to use for parsing other types
bool CustomASTVisitor::VisitDeclRefExpr(const clang::DeclRefExpr* declRefExpr) {

    if(! declRefExpr) {
        logFile << "WARN: Invalid DeclRefExpr handle received" << std::endl;
        return true;
    }
    logFile << "INFO: VisitDeclRefExpr: " << declRefExpr->getNameInfo().getAsString() << std::endl;

    // operator overload functions show up as C function
    // This will be handled in parseCFunction()
    if(std::string::npos != declRefExpr->getNameInfo().getAsString().find("operator")) {
        logFile << "INFO: Operator overload function found in VisitDeclRefExpr, skipping" << std::endl;
        return true;
    }

    const clang::ValueDecl* valueDecl = declRefExpr->getDecl();
    if(! valueDecl) {
        logFile << "INFO: Unable to find declaration, Skipping" << std::endl;
        return false;
    }

    // BaseType Identifier is only meant for user defined types, including std types
    // int, char, ... does not have base type
    // Also C functions do not have basetype, So this would fail for c function
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
        auto enumTagDecl = getDeclFromType(declRefExpr->getType().getTypePtr());
        if(! enumTagDecl) {
            logFile << "WARN: Unable to get tag declaration from enum type" << std::endl;
            return true;
        }
        parseEnum(clang::dyn_cast<clang::EnumDecl>(enumTagDecl));
    }

    return true; // Parse only C/C++ Enum types
}

bool CustomASTVisitor::VisitMemberExpr(const clang::MemberExpr* memberExpr) {
    if(! memberExpr) {
        logFile << "WARN: Invalid MemberExpr handle received" << std::endl;
        return true;
    }
    logFile << "INFO: VisitMemberExpr, member name: " << memberExpr->getMemberNameInfo().getAsString() << std::endl;
    logFile << "INFO: VisitMemberExpr, member type: " << memberExpr->getMemberDecl()->getType().getAsString() << std::endl;

    // Skip member function expression
    if(clang::isa<clang::FunctionDecl>(memberExpr->getMemberDecl())) {
        return true; // nothing to do
    }

    // Skip if the declaration origin is from the same source file
    const std::string sourcefileName = getfileNameFromPath(
                m_sourceManager.getFileEntryForID(m_sourceManager.getMainFileID())->getName());

    clang::ValueDecl* valueDecl = memberExpr->getMemberDecl();
    const std::string declFileNameStripped = getfileNameFromPath(getStrippedFilePath(
                                        m_sourceManager.getFilename(valueDecl->getLocation()).data()));

    if(sourcefileName == declFileNameStripped) {
        logFile << "INFO: Declaration origin is source file, Skipping" << std::endl;
        return true;
    }

    const std::string childInfo = memberExpr->getMemberDecl()->getType().getAsString() + 
                    PredefinedMockData::aSpace + memberExpr->getMemberNameInfo().getAsString();
    // Example:
    // enum Foo { EnumConst1, EnumConst2};
    // struct Bar {
    //     Foo enumObj;
    // };
    // Later, Bar barObj; barObj.enumObj = EnumConst1;
    // This function would be called when above line is hit. Basically member reference
    // So here "enum Foo" and "struct Bar" both should be processed and stored
    
    // Below code block would parse "Foo" information
    auto* declType = memberExpr->getMemberDecl()->getType().getTypePtrOrNull();
    if(declType->isEnumeralType()) {
        parseEnum(clang::dyn_cast<clang::EnumDecl>(getDeclFromType(declType)));
    } else {
        // For other types like struct, class
        if(getDeclFromType(declType)) {
            const std::string fileName = getfileNameFromPath(getStrippedFilePath(
                            m_sourceManager.getFilename(getDeclFromType(declType)->getLocation()).data()));
    
            if(fileContentToBeMocked(fileName, childInfo)) {
                clang::DeclContext* declContext = getDeclFromType(declType);
                processDeclContextHierarchy(declContext, fileName);
            } else {
                logFile << "INFO: Not mocking file: " << fileName << std::endl;
            }
        } else {
            logFile << "INFO: Unable to get decl from type(Might be build in type), skipping" << std::endl;
        }
    }

    // Below code block would parse "Bar" information
    if(fileContentToBeMocked(declFileNameStripped, childInfo)) {
        clang::DeclContext* declContext = valueDecl->getDeclContext();
        processDeclContextHierarchy(declContext, declFileNameStripped, true/*StoreInclude*/, FieldDeclInfo{childInfo});
    } else {
        logFile << "INFO: Not mocking file: " << declFileNameStripped << std::endl;
    }
    
    return true;
}

// This gets called first for all callExpression
bool CustomASTVisitor::VisitCallExpr(clang::CallExpr* callExpression) {
    if(! callExpression) {
        logFile << "WARN: Invalid CallExpr handle received" << std::endl;
        return true;
    }

    // @Note: getDirectCallee() seems to return nullptr for cast expression. But callExpression->getCalleeDecl() would work
    // However cast expressions can be skipped
    if(! callExpression->getDirectCallee()) {
        logFile << "WARN: Suspecious CallExpr found, Skipping" << std::endl;
        return true;
    }
    logFile << "INFO: VisitCallExpr, callee name: " << callExpression->getDirectCallee()->getNameAsString().c_str() << std::endl;

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

// Getter function for getting include files information
const IncludeInfo& CustomASTVisitor::getIncludeInfo() {
    return m_includes;
}

const std::map<std::string/*fileName*/, std::list<MockInfoStorage>>& CustomASTVisitor::getMockInfoStorage() {
    return m_mockInfoStorage;
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

    parseClassAndMethodInfo(methodDecl);
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

    // Check if function originating from the same source file
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
    if(! fileContentToBeMocked(fileName, functionDecl->getNameAsString())) {
        logFile << "INFO: Not mocking - " << fileName << std::endl;
        return;
    }

    // Parse and get C function information
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
    methodInfo.isCFunction = true;

    const clang::DeclContext* parentDeclContext = functionDecl->getParent();
    processDeclContextHierarchy(parentDeclContext, fileName, false ,methodInfo);
}

// Parse C and C++ scoped enum types
// And also parse enum parent information
void CustomASTVisitor::parseEnum(const clang::EnumDecl* enumDecl) {
    if(! enumDecl) {
        logFile << "WARN: Invalid EnumDecl received, unable to parse eum declaration" << std::endl;
        return;
    }

    // Get actual input file name
    const std::string sourceFileName = getfileNameFromPath(m_sourceManager.getFileEntryForID(m_sourceManager.getMainFileID())->getName());

    // Get file name from EnumDecl
    const std::string declFileName = getfileNameFromPath(m_sourceManager.getFilename(enumDecl->getLocation()).data());

    if(sourceFileName == declFileName) {
        logFile << "INFO: Enum belonging to main source file, Skipping" << std::endl;
        return; // do nothing
    }

    if(! fileContentToBeMocked(declFileName, enumDecl->getNameAsString())) {
        logFile << "INFO: Not mocking file: " << declFileName << std::endl;
        return;
    }

    resetCurrentStorageNode(declFileName);

    // Parse enum information
    enumProperties enumProps = {};
    enumProps.enumName = enumDecl->getNameAsString();
    enumProps.isScopedEnum = enumDecl->isScoped();
    logFile << "INFO: Enum name: " << enumProps.enumName << std::endl;

    for(const auto* enumConst : enumDecl->enumerators()) {
        enumProps.enumValues.push_back(enumConst->getNameAsString());
        logFile << "Enum const name: " << enumConst->getNameAsString() << std::endl;
    }

    // Now get enum parent information
    const clang::DeclContext* parentDeclContext = enumDecl->getParent();
    processDeclContextHierarchy(parentDeclContext, declFileName, false, enumProps);
}

void CustomASTVisitor::ParseOperatorOverloading(clang::CXXMethodDecl* cxxMethodDec) {
    if(! cxxMethodDec->getParent()) {
        logFile << "WARN: Unable to get parent of CXXMethodDecl" << std::endl;
        return;
    }

    parseClassAndMethodInfo(cxxMethodDec, true/*operator overloading*/);
}

void CustomASTVisitor::parseClassAndMethodInfo(clang::CXXMethodDecl* methodDecl, bool operatorOverloadingType) {
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

    if(! fileContentToBeMocked(currentFileName, methodDecl->getParent()->getNameAsString())) {
        logFile << "INFO: Not mocking file: " << currentFileName << std::endl;
        return;
    }

    // Parse method information
    MethodInfo methodInfo = {};
    clang::FunctionDecl* functionDecl = {};
    if(methodDecl->getParent()->getTemplateInstantiationPattern()) { // UnWrap the template instance
        functionDecl = methodDecl->getTemplateInstantiationPattern();
    } else {
        functionDecl = clang::dyn_cast_or_null<clang::FunctionDecl>(methodDecl);
    }
    if(! functionDecl) {
        logFile << "WARN: Unable to get function declaration for member expression" << std::endl;
        return;
    }

    methodInfo.name = functionDecl->getNameAsString();
    methodInfo.returnType = checkBool(functionDecl->getReturnType().getAsString());
    logFile << "INFO: Store the file name of return type defined: " << methodInfo.returnType.c_str() << std::endl;
    storeIncludeInformation(const_cast<clang::Type*>(functionDecl->getReturnType().getTypePtr()), currentFileName);
    methodInfo.isConst = methodDecl->isConst();
    methodInfo.isTemplated = functionDecl->isTemplated();
    std::vector<std::string> argsInfo;
    for(int i=0; i<functionDecl->getNumParams(); i++) {
        argsInfo.push_back(checkBool(functionDecl->getParamDecl(i)->getType().getAsString())); // Decl always has a type
        logFile << "INFO: Store the file name of function arg defined: " << functionDecl->getParamDecl(i)->getType().getAsString().c_str() << std::endl;
        storeIncludeInformation(const_cast<clang::Type*>(functionDecl->getParamDecl(i)->getType().getTypePtr()), currentFileName);
    }
    methodInfo.args = argsInfo;
    methodInfo.isOperatorOverloading = operatorOverloadingType;

    // Now parse parent information of this method
    clang::DeclContext* parentDeclContext = methodDecl->getParent();
    processDeclContextHierarchy(parentDeclContext, currentFileName, false, methodInfo);
}

// Workaround to convert _Bool to bool
// Clang reports bool type as _Bool
std::string CustomASTVisitor::checkBool(const std::string typeName) {
    if(std::string("_Bool") == typeName) {
        return "bool";
    }
    return typeName;
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

clang::DeclContext* CustomASTVisitor::getDeclContextFromType(clang::Type* type) {
    if(! type) {
        logFile << "WARN: Type is empty, Unable to process. Skipping" << std::endl;
        return nullptr;
    }
    if(type->isBuiltinType()) {
        logFile << "INFO: Build in type found, Skipping" << std::endl;
        return nullptr;
    }

    // Type could be pointer, reference or pure type
    // Below code unwraps pointer and reference type to pure type
    clang::Type* typePtr = type;

    if(type->isReferenceType()) {
        const clang::ReferenceType* referType = type->getAs<clang::ReferenceType>();
        if(! referType) {
            logFile << "WARN: Unable to get reference type from type" << std::endl;
            return nullptr;
        }
        typePtr = const_cast<clang::Type*>(referType->getPointeeType().getTypePtr());
        if(! typePtr) {
            logFile << "WARN: Unable to get type pointer from pointee type" << std::endl;
            return nullptr;
        }
    } else if (type->isPointerType()) {
        const clang::PointerType* pointerType = type->getAs<clang::PointerType>();
        if(! pointerType) {
            logFile << "WARN: Unable to get pointer type from type" << std::endl;
            return nullptr;
        }
        typePtr = const_cast<clang::Type*>(pointerType->getPointeeType().getTypePtr());
        if(! typePtr) {
            logFile << "WARN: Unable to get type pointer from pointee type" << std::endl;
            return nullptr;
        }
    }

    // Finally get declaration tagged with type
    const clang::TagType* tagType = typePtr->getAs<clang::TagType>();
    if(! tagType) {
        logFile << "WARN: Unable to get tag type from type pointer" << std::endl;
        logFile << "WARN: Is in build type: " << typePtr->isBuiltinType() << std::endl;
        return nullptr;
    }
    if(! tagType->getDecl()) {
        logFile << "WARN: Unable to get declaration from tag type" << std::endl;
        return nullptr;
    }
    

    // Return parent information
    return tagType->getDecl()->getDeclContext();
}

clang::TagDecl* CustomASTVisitor::getDeclFromType(const clang::Type* type) {
    if(! type) {
        logFile << "WARN: Type is empty, Unable to process. Skipping" << std::endl;
        return nullptr;
    }
    if(type->isBuiltinType()) {
        logFile << "INFO: Build in type found, Skipping" << std::endl;
        return nullptr;
    }

    // Type could be pointer, reference or pure type
    // Below code unwraps pointer and reference type to pure type
    const clang::Type* typePtr = type;

    if(type->isReferenceType()) {
        const clang::ReferenceType* referType = type->getAs<clang::ReferenceType>();
        if(! referType) {
            logFile << "WARN: Unable to get reference type from type" << std::endl;
            return nullptr;
        }
        typePtr = const_cast<clang::Type*>(referType->getPointeeType().getTypePtr());
        if(! typePtr) {
            logFile << "WARN: Unable to get type pointer from pointee type" << std::endl;
            return nullptr;
        }
    } else if (type->isPointerType()) {
        const clang::PointerType* pointerType = type->getAs<clang::PointerType>();
        if(! pointerType) {
            logFile << "WARN: Unable to get pointer type from type" << std::endl;
            return nullptr;
        }
        typePtr = const_cast<clang::Type*>(pointerType->getPointeeType().getTypePtr());
        if(! typePtr) {
            logFile << "WARN: Unable to get type pointer from pointee type" << std::endl;
            return nullptr;
        }
    }

    // Finally get declaration tagged with type
    const clang::TagType* tagType = typePtr->getAs<clang::TagType>();
    if(! tagType) {
        logFile << "WARN: Unable to get tag type from type pointer" << std::endl;
        logFile << "WARN: Is in build type: " << typePtr->isBuiltinType() << std::endl;
        return nullptr;
    }
    if(! tagType->getDecl()) {
        logFile << "WARN: Unable to get declaration from tag type" << std::endl;
        return nullptr;
    }

    // Return parent information
    return tagType->getDecl();
}

// Parse given declaration context and fill appropriate data in declData.
// This function will parse the declaration and determine whether the declaration 
// context is a Namespace or a Class/Struct/Union
bool CustomASTVisitor::processDeclContextInfo(const clang::DeclContext* declContext, MockInfoStorageType& declData, std::string* storeFileName) noexcept {
    if(! declContext) {
        return false;
    }

    // Check parent is of any MockInfoStorageType type
    if(clang::isa<clang::NamespaceDecl>(declContext)) {
        const std::string namespaceName = clang::dyn_cast_or_null<clang::NamedDecl>(declContext)->getNameAsString();
        logFile << "INFO: processDeclContextInfo: namespace found: " << namespaceName << std::endl;
        declData = NamespaceInfo{namespaceName};
        return true;
    } else if(clang::isa<clang::RecordDecl>(declContext) || clang::isa<clang::CXXRecordDecl>(declContext)) {
        const clang::NamedDecl* namedDecl = clang::dyn_cast_or_null<clang::NamedDecl>(declContext);
        const clang::RecordDecl* recordDecl = clang::dyn_cast<clang::RecordDecl>(declContext);
        std::string typeString = {};
        if(recordDecl->isStruct()) {
            typeString = "struct ";
        } else if(recordDecl->isClass()) {
            typeString = "class ";
        } else if(recordDecl->isUnion()) {
            typeString = "union ";
        }
        
        ClassStructUnionInfo info = {};
        info.declKindName = typeString;
        info.name = namedDecl->getNameAsString();
        auto* parentCXXRecordDecl = llvm::dyn_cast<clang::CXXRecordDecl>(declContext);
        if(parentCXXRecordDecl) {
            // Check if the class is template class
            if(parentCXXRecordDecl->getTemplateInstantiationPattern()) {
                if(! parentCXXRecordDecl->getTemplateInstantiationPattern()) {
                    logFile << "WARN: Unable to get Template instantiation pattern of CXXRecordDecl" << std::endl;
                    return false;
                }
                if(! parentCXXRecordDecl->getTemplateInstantiationPattern()->getDescribedClassTemplate()) {
                    logFile << "WARN: Unable to get Described Class Template from CXXRecordDecl" << std::endl;
                    return false;
                }
                if(! parentCXXRecordDecl->getTemplateInstantiationPattern()->getDescribedClassTemplate()->getTemplateParameters()) {
                    logFile << "WARN: Unable to get Template parameter list from ClassTempDecl" << std::endl;
                    return false;
                }
                clang::TemplateParameterList* templateParamList = parentCXXRecordDecl->getTemplateInstantiationPattern()->getDescribedClassTemplate()->getTemplateParameters();
                std::vector<std::string> tempParamList;
                for(int i=0; i<templateParamList->size(); i++) {
                    tempParamList.push_back(templateParamList->getParam(i)->getNameAsString());
                }
        
                info.isTemplateClass = true;
                info.templateParams = tempParamList;
            }
        }
        info.filename = getfileNameFromPath(m_sourceManager.getFilename(parentCXXRecordDecl->getLocation()));
        if(storeFileName) {
            *storeFileName = info.filename;
        }
        declData = info;
        return true;
    }

    return false;
}

// Parse given declaration context and parent hierarchy as well
// Example:
// Namespace Foo {
//   class Bar {
//   }
// }
// Incase Bar context is passed, this function would parse Bar and Foo information
void CustomASTVisitor::processDeclContextHierarchy(const clang::DeclContext* declContext, const std::string& fileName,
                       bool storeIncludeFileName, std::optional<MockInfoStorageType> appendMockData) noexcept {
    if(! declContext) {
        logFile << "WARN: Invalid DeclContext received, not processing" << std::endl;
        return;
    }

    MockInfoStorageType mockInfo = {};
    std::list<MockInfoStorageType> mockInfoList = {};
    if(appendMockData.has_value()) {
        mockInfoList.push_back(appendMockData.value());
    }
    while(declContext) {
        std::string includeFileName = {};
        if(processDeclContextInfo(declContext, mockInfo, storeIncludeFileName ? &includeFileName : nullptr)) {
            mockInfoList.push_front(mockInfo);
            declContext = declContext->getParent();
            if(!includeFileName.empty()) {
                storeIncludeInformation(fileName, includeFileName);
            }
        } else {
            break; // Nothing to parse anymore
        }
    }

    resetCurrentStorageNode(fileName);
    for(auto eachNode : mockInfoList) {
        storeMockData(fileName, eachNode);
    }
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

bool CustomASTVisitor::isStdNamespace(const std::string namespaceInfo) {
    return (std::string::npos == namespaceInfo.find("std::")) ? false : true;
}

// Mock or not to Mock is decided based on the file
// Once file is choosen to not mock, Then content of that file will not be mocked in further findings
// Make sure to use only filename alone or with full path. Don't mix both
bool CustomASTVisitor::fileContentToBeMocked(const std::string& fileName, const std::string& helperIdentifier) {

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
    logFile << "INFO: To be mocked? fileName: " << fileName << ", element: " << helperIdentifier << "" << std::endl;
    std::string input = "y";
    if(askUserConfirmation) {
        static bool askOnce = false;
        if(! askOnce) {
            askOnce = true;
            std::cout << "\n\33[1;43mBelow are the list of files identified as dependencies to your source file\033[0m\n";
            std::cout << "\33[1;43mSo press \"y\" if you want to mock the file content, \"n\" otherwise\033[0m\n" << std::endl;
        }
        std::cout << "\33[1m" <<fileName << "(" << helperIdentifier << "): \033[0m";
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

// Qualified type name - struct ns::foo::bar::buz
// Return - struct buz
std::string CustomASTVisitor::getTypeNameFromQualifiedTypeName(const std::string& qualifiedTypeName) {
    // Nothing to strip
    if(std::string::npos == qualifiedTypeName.find("::")) {
        return qualifiedTypeName;
    }

    // Get first word(example: struct)
    const std::string firstWord = qualifiedTypeName.substr(0, qualifiedTypeName.find(" "));
    // Store actual type name
    const std::string actualTypeName = qualifiedTypeName.substr((qualifiedTypeName.find_last_of("::") + 1),
                              (qualifiedTypeName.size() - (qualifiedTypeName.find_last_of("::") + 1)));

    return firstWord + " " + actualTypeName;
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

void CustomASTVisitor::storeIncludeInformation(const std::string& fileName, const std::string& includeFileName) {

    if(! m_includes.count(fileName)) { // First include in the file
        m_includes[fileName] = {};
    }

    // @Note: It appears that there is no straightforward method to determine the precise C++ standard
    // header file usually used corresponding to each standard declaration
    // Example: std::string -> /usr/include/c++/string
    // So workaround has been added to include below std file to cover any std file
    std::string includeFileNameToBeStored = includeFileName;
    if(std::string::npos != includeFileName.find("c++/")) {
        // Change the include filename to <bits/stdc++.h>
        includeFileNameToBeStored = "bits/stdc++.h";
    }
    // Is include file already noted
    bool found = false;
    for(const auto eachFile : m_includes.at(fileName)) {
        if(eachFile == includeFileNameToBeStored) {
            found = true;
        }
    }
    if(! found) {
        m_includes.at(fileName).push_back(includeFileNameToBeStored);
    }
}

// 1. Storage points to first node of mock tree after every resetNode
// 2. So compare the node and if matches then skip adding new node, otherwise add new node and update current node
void CustomASTVisitor::storeMockData(const std::string& fileName, MockInfoStorageType mockData) noexcept {

    // First entry for the file, add it right away
    if(! m_mockInfoStorage.count(fileName)) {
        m_mockInfoStorage[fileName].push_back({mockData, {}});
        m_mockInfoStorageRef = &m_mockInfoStorage.at(fileName).front().childData;
        return;
    }

    // File entry has data already
    // Traverse the node from current node and insert it in the right place
    bool nodeExist = false;
    for(MockInfoStorage& eachNode : *m_mockInfoStorageRef) {

        if(eachNode.data.index() == mockData.index()) { // Exact node data type found
            switch(mockData.index()) {
                case 0: { // NamespaceInfo
                    if(std::get<NamespaceInfo>(eachNode.data).namespaceName == std::get<NamespaceInfo>(mockData).namespaceName) {
                        // Mock data already exists, do nothing
                        nodeExist = true;
                    }
                    break;
                }
                case 1: { // ClassStructUnionInfo
                    if(std::get<ClassStructUnionInfo>(eachNode.data).name == std::get<ClassStructUnionInfo>(mockData).name) {
                        // Mock data already exists, do nothing
                        nodeExist = true;
                    }
                    break;
                }
                case 2: { // EnumInfo
                    if(std::get<enumProperties>(eachNode.data).enumName == std::get<enumProperties>(mockData).enumName) {
                        // Mock data already exists, do nothing
                        nodeExist = true;
                    }
                    break;
                }
                case 3: { // MethodInfo
                    if(std::get<MethodInfo>(eachNode.data).name == std::get<MethodInfo>(mockData).name) {
                        // Check arguments as well
                        auto storedMethodInfo = std::get<MethodInfo>(eachNode.data);
                        auto newMethodInfo = std::get<MethodInfo>(mockData);
                        if((0 == storedMethodInfo.args.size()) && (0 == newMethodInfo.args.size())) {
                            nodeExist = true;
                            break;
                        }
                        if(storedMethodInfo.args.size() == newMethodInfo.args.size()) {
                            for(int i=0; i< storedMethodInfo.args.size(); i++) {
                                if(storedMethodInfo.args[i] == newMethodInfo.args[i]) {
                                    nodeExist = true;
                                    continue;
                                } else {
                                    nodeExist = false;
                                    break;
                                }
                            }
                        }
                    }
                    break;
                }
                case 4: { // FieldDeclInfo
                    if(std::get<FieldDeclInfo>(eachNode.data).declName == std::get<FieldDeclInfo>(mockData).declName) {
                        // Mock data already exists, do nothing
                        nodeExist = true;
                    }
                    break;
                }
                default: { // This would result undefined behavior
                    logFile << "WARN: Invalid data index received, index: " << mockData.index() << std::endl;
                    return;
                }
            }
        }
        if(nodeExist) {
            m_mockInfoStorageRef = &(eachNode.childData);
            break;
        }
    }

    // Mock data doesn't exists, store it
    if(! nodeExist) {
        m_mockInfoStorageRef->push_back({mockData, {}});
        // Now set current node to recent entry
        m_mockInfoStorageRef = &m_mockInfoStorageRef->back().childData;
    }
}

void CustomASTVisitor::resetCurrentStorageNode(const std::string& fileName) noexcept {
    if(m_mockInfoStorage.count(fileName)) {
        m_mockInfoStorageRef = &m_mockInfoStorage.at(fileName);
    }
}