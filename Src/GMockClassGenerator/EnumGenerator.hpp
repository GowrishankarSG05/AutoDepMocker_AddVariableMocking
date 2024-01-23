/**
  * @file: EnumGenerator.hpp
  * @brief: The EnumGenerator generates enum declarations and provides support for both C and C++ scoped enums.
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

#ifndef ENUM_GENERATOR_HPP_
#define ENUM_GENERATOR_HPP_

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>

#include "GeneratorUtilities.hpp"
#include "MockGeneratorTypes.hpp"

class EnumGenerator : public GeneratorUtilities {
public:
    // Special member functions
    explicit EnumGenerator() = default;
    ~EnumGenerator() = default;
    EnumGenerator& operator =(const EnumGenerator&) = delete;
    EnumGenerator(const EnumGenerator&) = delete;

    void constructEnum(const std::string& fileName, const std::vector<enumProperties>& enumProp);

private:
    std::string mockEnum;
};

#endif // MOCK_GENERATOR_HPP_
