/**
  * @file: CPPMockGenerator.hpp
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

#ifndef MOCK_GENERATOR_HPP_
#define MOCK_GENERATOR_HPP_

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <filesystem>

#include "GeneratorUtilities.hpp"
#include "MockGeneratorTypes.hpp"

class CPPMockGenerator : public GeneratorUtilities {
public:
    // Special member functions
    explicit CPPMockGenerator() = default;
    ~CPPMockGenerator() = default;
    CPPMockGenerator& operator =(const CPPMockGenerator&) = delete;
    CPPMockGenerator(const CPPMockGenerator&) = delete;

    void constructClass(const ClassInfo& classInfo, const std::vector<MethodInfo>& calleeInfo);

private:
    std::string mockClass;

    // Function to construct wrapper class for supporting operator overload functions
    void constructWrapperClass(const ClassInfo& classInfo, const std::vector<MethodInfo>& calleeInfo);

    // Workaround to get operator name in string format
    // Example:
    // Input  : Operator+
    // Return : OperatorAdd
    std::string getOperatorName(const std::string& operatorId);
};

#endif // MOCK_GENERATOR_HPP_
