/**
  * @file: CustomASTConsumer.cpp
  * @brief: The CustomASTConsumer receives generated AST and invokes CustomASTVisitor for each declaration found in the generated AST.
  *         Once traversing of AST is done, it provides mock class information to mock class generator
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

#include <iostream>

#include "CustomASTConsumer.hpp"
#include "GMockClassGenerator.hpp"

CustomASTConsumer::CustomASTConsumer(clang::SourceManager& sourceManager)
    : m_sourceManager(sourceManager) {
}

// This method is called only once when ast is generated, Ready to traverse generated ast
void CustomASTConsumer::HandleTranslationUnit(clang::ASTContext& context) {

    // Create CustomASTConsumer
    m_customASTvisitor = std::make_unique<CustomASTVisitor>(context, m_sourceManager);

    // Get all declaration and visit one by one
    auto decList = context.getTranslationUnitDecl()->decls();

    for(auto &each : decList) {
        if(context.getSourceManager().getMainFileID() != context.getSourceManager().getFileID(each->getLocation())) {
            // Visit only the given source file
            continue;
        }

        // Relavant "visitor" gets called in CustomASTVisitor
        m_customASTvisitor->TraverseDecl(each);
    }

    // Parsing done, Generate Mock class
    generateMockFiles();
}

// Get necessary information from CustomASTVisitor and invoke MockGenerator
void CustomASTConsumer::generateMockFiles() {

    GMockClassGenerator gmockGenerator;
    IMockGenerator& mockGenerator = gmockGenerator;

    // Write include information first
    const IncludeInfo& includeInfo = m_customASTvisitor->getIncludeInfo();
    for(const auto& each : includeInfo) {
        mockGenerator.constructIncludes(each.first, each.second);
    }

    // Write Enums
    const EnumInfo& enumInfo = m_customASTvisitor->getEnumInfo();
    for(const auto& itr : enumInfo) {
        mockGenerator.constructEnum(itr.first, itr.second);
    }

    // Write C++ classes
    const auto [classInfo, classMethodsInfo] = m_customASTvisitor->getMockclassInfoAndMethods();
    for(const auto& itr : classInfo) {
        mockGenerator.constructClass(itr.second, classMethodsInfo.at(itr.first));
    }

    // Write C functions
    const CFunctionInfoType& cFunctionsInfo = m_customASTvisitor->getCMockFunctions();
    for(const auto& each : cFunctionsInfo) {
        mockGenerator.constructCFunction(each.first, each.second);
    }

    // Write field declaration
    const std::map<std::string/*fileName*/, std::list<VariableInfoHierarchy>>& varInfo = m_customASTvisitor->getVariableInfoContainer();
   for(const auto& each : varInfo) {
        mockGenerator.constructFieldDeclation(each.first, each.second);
   }

    // Finish mocking
    mockGenerator.finalizeMocking();

    std::cout << "\33[1;35m\nMock files have been generated to GeneratedMocks folder. Feel free to customize the content of these files to suit the specific requirements of your project.\033[0m\n";
    std::cout << "\33[1;35m\nCopyright information is left blank in generated files. Please add it according to your project.\033[0m\n";
    std::cout << "\33[1;35m\nHappy Mocking!\033[0m\n";
}
