/**
  * @file: CustomFrontendAction.hpp
  * @brief: When a frontend action starts, the CustomFrontendAction creates and provides a custom consumer
  *              for the Abstract Syntax Tree (AST)
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

#ifndef CUSTOMFRONTENDACTION_HPP
#define CUSTOMFRONTENDACTION_HPP

#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/CompilerInstance.h"

#include "CustomASTConsumer.hpp"
#include "MockGeneratorTypes.hpp"


class CustomFrontendAction : public clang::ASTFrontendAction {
public:

    explicit CustomFrontendAction() = default;
    ~CustomFrontendAction() = default;

    /** Create AST consumer
     * @brief: The function to be invoked in order to obtain a custom ASTConsumer
     * @arg ci: Instance of the compiler framework. It encapsulates the state and context of a compilation process
     * @return clang::ASTConsumer: Returns custom AST consumer
     */
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &ci, clang::StringRef /*inFile*/);

private:
    clang::SourceManager* m_sourceManager = nullptr;
};

#endif // CUSTOMFRONTENDACTION_HPP
