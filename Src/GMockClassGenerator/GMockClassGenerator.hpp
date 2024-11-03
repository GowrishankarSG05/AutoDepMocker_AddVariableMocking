/**
  * @file: GMockClassGenerator.hpp
  * @brief: Google mock class generator
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

#ifndef GMOCK_CLASS_GENERATOR_HPP_
#define GMOCK_CLASS_GENERATOR_HPP_

#include "IMockGenerator.hpp"
#include "GMockClassMockGeneratorImpl.hpp"

class GMockClassGenerator : public IMockGenerator, public NonCopyableMovable {
public:
    // Special member functions
    explicit GMockClassGenerator() = default;
    ~GMockClassGenerator() = default;

    // IMockGenerator interface
    void constructMockClass(const std::string& fileName, const std::list<MockInfoStorage>& mockInfoList, const std::list<std::string>& includeList) override;
    void finalizeMocking() override;

private:
    // Generators
    GMockClassGeneratorImpl m_gmockClassGeneratorImpl;

};

#endif // GMOCK_CLASS_GENERATOR_HPP_
