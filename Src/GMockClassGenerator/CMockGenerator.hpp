/**
  * @file: CMockGenerator.hpp
  * @brief: The CMockGenerator creates C functions and GMOCK wrapper class for C functions
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

#ifndef CMOCK_GENERATOR_HPP_
#define CMOCK_GENERATOR_HPP_

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>

#include "GeneratorUtilities.hpp"
#include "MockGeneratorTypes.hpp"

class CMockGenerator : public GeneratorUtilities {
public:
    // Special member functions
    explicit CMockGenerator() = default;
    ~CMockGenerator() = default;
    CMockGenerator& operator =(const CMockGenerator&) = delete;
    CMockGenerator(const CMockGenerator&) = delete;

    void constructFunction(const std::string& fileName, const std::vector<MethodInfo>& methodsInfo);

private:
    std::string mockClass;

    void constructWrapperFunction(const std::string& fileName, const std::vector<MethodInfo>& methodsInfo);

    void constructMockFunction(const std::string& fileName, const std::vector<MethodInfo>& methodInfo);
};

#endif // MOCK_GENERATOR_HPP_
