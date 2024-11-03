/**
  * @file: CustomASTVisitor.hpp
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

#ifndef CUSTOMASTVISITOR_HPP
#define CUSTOMASTVISITOR_HPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <utility>
#include <set>
#include <filesystem>
#include <tuple>
#include <fstream>
#include <optional>

#include "clang/AST/RecursiveASTVisitor.h"

#include "MockGeneratorTypes.hpp"

class CustomASTVisitor : public clang::RecursiveASTVisitor<CustomASTVisitor>, public NonCopyableMovable {
public:

    explicit CustomASTVisitor(clang::ASTContext& ASTContext, clang::SourceManager& sourceManager);

    ~CustomASTVisitor();

    /** Visitor for variable declaration
     * @brief: This gets called for each variable declaration
     *           Example: Foo object; <- Here object is variable declaration
     *         Parse and get information about variable declaration and store it to generate mock class
     * @arg: fieldDecl: Contains information about variable declaration
     * @return: bool: True on success, False on failure
    */ 
    bool VisitVarDecl(clang::VarDecl *variableDecl);

    /** Visitor for declaration reference expression
     * @brief: This gets called for each declaration reference expression
     *           Example: catch(ExceptionClass obj) <- Here ExceptionClass is reference expression
     *         Parse and get information about the declaration and store it to generate mock class
     * @arg declRefExpr: Contains information about the declaraton expression
     * @return bool: True on success, False on failure
     */
    bool VisitDeclRefExpr(const clang::DeclRefExpr* declRefExpr);

    /***
     * Visitor for member expression
     * @brief: This gets called for each member expression
     *          Example; FooObj.x = 10; <- member expression
     *         Parse and get member and parent information to generate mock class
     * @arg: memberExpr: Contains details about the member expression
     * @return bool: True on success, false on failure
     */
    bool VisitMemberExpr(const clang::MemberExpr* memberExpr);

    /** Visitor for declaration reference expression
     * @brief: This gets called for each c and c++ method call present in the AST
     *           Example: myObject.foo() <- CallExpression which has caller(myObject) and callee(foo)
     *         Parse and get information about the the caller(class) and callee(method)
     * @note:  Callee information is stored only for c++ methods
     * @arg callExpression: Contains information about the call expression
     * @return bool: True on success, False on failure
     */
    bool VisitCallExpr(clang::CallExpr* callExpression);

    // Getter function for retrieving include files information. Call this once parsing is completely done
    const IncludeInfo& getIncludeInfo();

    // Getter function for mock storage retrieval
    const std::map<std::string/*fileName*/, std::list<MockInfoStorage>>& getMockInfoStorage();

private:

    // Parse C++ member expression
    void parseCXXMemberExpression(clang::CallExpr* callEpr);

    // Parse C function call expression
    void parseCFunction(clang::CallExpr* callExpr);

    // Parse enum
    void parseEnum(const clang::EnumDecl* enumDecl);

    // Parse c++ operator overloading
    void ParseOperatorOverloading(clang::CXXMethodDecl* cxxMethodDec);

    // Read class and method information from clang::CXXMethodDecl
    // Supports C++ method, operator and method overloading and template class
    void parseClassAndMethodInfo(clang::CXXMethodDecl* methodDecl, bool operatorOverloadingType = false);

    // Function to workaround _Bool types
    // Clang reports bool type as _Bool. So convert _Bool to bool
    std::string checkBool(const std::string typeName);

    // Returns fileName where the given clang::Type is defined
    // List of type possible:
    //   -> Pointer type, Reference Type and just Type
    // Limitation: Some std files are defined in different location and gets included
    //             from a wrapper file(ex: string -> basic_string.h)
    //             WorkAround: Include bits/stdc++.h to blindly include everything
    std::optional<std::string> getFileNameFromTypeDeclaration(clang::Type* type);

    // Get declaration context from of clang::Type
    clang::DeclContext* getDeclContextFromType(clang::Type* type);

    // Get declaration from of clang::Type
    clang::TagDecl* getDeclFromType(const clang::Type* type);

    // Generic function to parse decl context information
    bool processDeclContextInfo(const clang::DeclContext* declContext, MockInfoStorageType& parentInfo, std::string* storeFileName = nullptr) noexcept;

    // Parse given declaration context and parent hierarchy as well
    // Example:
    // Namespace Foo {
    //   class Bar {
    //   }
    // }
    // Incase Bar context is passed, this function would parse Bar and Foo information
    void processDeclContextHierarchy(const clang::DeclContext* declContext, const std::string& fileName,
                                    bool storeIncludeFileName = false,
                                    std::optional<MockInfoStorageType> appendMockData = {}) noexcept;

    // Utility function to remove "/usr/include" prefix
    // Input: /usr/include/Header.hpp
    // Outpur: Header.hpp
    std::string getStrippedFilePath(const std::string fullPath);

    // Find the given namespace contains "std" string or not
    // Workaround to find out c++ stds
    bool isStdNamespace(const std::string namespaceInfo);

    // Determine whether to mock the given file or not
    // Mock or not to Mock is decided based on the file
    // Once file is choosen to not mock, Then content of that file will not be mocked in further findings
    bool fileContentToBeMocked(const std::string& fileName, const std::string& className);

    std::string getfileNameFromPath(const std::string& filePath);

    // Qualified type name - struct ns::foo::bar::buz
    // Return - struct buz
    std::string getTypeNameFromQualifiedTypeName(const std::string& qualifiedTypeName);

    // Fetch filename in which clang::Type is defined and store it
    void storeIncludeInformation(clang::Type* type, const std::string fileName);

    // Store include information
    void storeIncludeInformation(const std::string& fileName, const std::string& includeFileName);

    // Identify right node and store data in it
    void storeMockData(const std::string& fileName, MockInfoStorageType mockData) noexcept;

    // Reset current node to root node of filename in the map
    void resetCurrentStorageNode(const std::string& fileName) noexcept;

    // To obtain file-related information(Example: filename, location) utilize Clang ASTContext and SourceManager
    clang::ASTContext& m_ASTContext;
    clang::SourceManager& m_sourceManager;

    // Include file information
    IncludeInfo m_includes;

    // Storage container for storing mock data
    std::map<std::string/*fileName*/, std::list<MockInfoStorage>> m_mockInfoStorage = {};
    std::list<MockInfoStorage>* m_mockInfoStorageRef = {};

    std::vector<std::string> notTobeMockedFiles = {"include/c++/", "include/x86_64-linux-gnu/c++"};
    std::vector<std::string> tobeMockedFiles = {};

    bool askUserConfirmation = true;

    // Logger file
    std::ofstream logFile;
};

#endif // CUSTOMASTVISITOR_HPP
