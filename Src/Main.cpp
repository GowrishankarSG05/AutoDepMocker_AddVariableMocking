/**
  * @file: Main.cpp
  * @brief: File contains main function that serves as the main entry point for the AutoDepMocker component.
  *         Main function gets compilation settings from command line arguments, loads compilation database
  *         and runs clang frontend action on the given source file with CustomFrontendAction
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


#include "clang/AST/ASTContext.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

#include "CustomFrontendAction.hpp"

// Helpers
llvm::cl::OptionCategory FindDeclCategory("main options");
static char FindDeclUsage[] = "AutoDepMocker <source file> --";

int main(int argc, const char **argv) {

    // locates and loads a compilation command database
    clang::tooling::CommonOptionsParser optionParser(argc, argv, FindDeclCategory,
                                                     FindDeclUsage);

    // Expect to get only one source file for mock generation
    const auto sourceFiles = optionParser.getSourcePathList();

    // ClangTool - Utility to run a FrontendAction over a set of files.
    clang::tooling::ClangTool tool(optionParser.getCompilations(), sourceFiles);

// Print all compile commad - For debugging
//    std::vector<clang::tooling::CompileCommand> cc = optionParser.getCompilations().getCompileCommands(sourceFiles.at(0));
//    for(const auto& each : cc) {
//        std::cout << "\nfileName : " << each.Filename << std::endl;
//        for(const auto& command : each.CommandLine) {
//            std::cout << "\ncommand: " << command << std::endl;
//        }
//    }

    // Run would start FrontEnd action on the given source file with compile commands
    tool.run(clang::tooling::newFrontendActionFactory<CustomFrontendAction>().get());

    return 0;
}
