/**
  * @file: CustomASTConsumer.hpp
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


#ifndef CUSTOMASTCONSUMER_HPP
#define CUSTOMASTCONSUMER_HPP

#include <memory>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "CustomASTVisitor.hpp"
#include "EnumGenerator.hpp"
#include "CPPMockGenerator.hpp"
#include "CMockGenerator.hpp"

class CustomASTConsumer : public clang::ASTConsumer {
public:

    explicit CustomASTConsumer(clang::SourceManager& sourceManager);
    ~CustomASTConsumer() = default;

    /** Handle translation unit
     * @brief: The function is invoked when AST is generated. And then it would invoke the visitors of CustomASTVisitor to handle
     *         methods and declarations
     * @arg context: AST context
     */
    // This function is called only once when ast is generated, Ready to traverse generated ast
    void HandleTranslationUnit(clang::ASTContext& context) override;

private:
    /** Generate mock files
     * @brief: Handle the generation of mock files which include enums, C++ methods, and C functions
     */
    void generateMockFiles();

    // ASTContext
    clang::SourceManager& m_sourceManager;

    std::unique_ptr<CustomASTVisitor> m_customASTvisitor = {};
};

#endif // CUSTOMASTCONSUMER_HPP
