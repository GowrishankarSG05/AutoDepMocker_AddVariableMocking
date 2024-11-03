/**
  * @file: GMockClassGeneratorImpl.hpp
  * @brief: The GMockClassGeneratorImpl creates a mock class based on GMOCK, along with a wrapper class for operator overloading.
  *         It supports mocking enum types, field declaration and C functions as well.
  *         Additionally, it supports template classes.
  * @author: Gowrishankar Saminathan <gowrishankarstg@gmail.com>
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

#ifndef GMOCK_CLASS_GENERATOR_IMPL_HPP_
#define GMOCK_CLASS_GENERATOR_IMPL_HPP_

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <filesystem>
#include <list>

#include "GeneratorUtilities.hpp"
#include "MockGeneratorTypes.hpp"

class GMockClassGeneratorImpl : public GeneratorUtilities {
public:
    // Special member functions
    explicit GMockClassGeneratorImpl() = default;
    ~GMockClassGeneratorImpl() = default;

    void constructMockClass(const std::string& fileName, const std::list<MockInfoStorage>& mockInfoList, 
                            const std::list<std::string>& includeList);

private:
    void writeMockData(const std::string& fileName, const MockInfoStorage& mockInfo);
    std::string getOperatorName(const std::string& operatorId);
    std::string getIndentation();
    void constructCFunction(const std::string& fileName, const std::list<MethodInfo>& cFunctions);

    std::string mockFile;
};

#endif // GMOCK_CLASS_GENERATOR_IMPL_HPP_
