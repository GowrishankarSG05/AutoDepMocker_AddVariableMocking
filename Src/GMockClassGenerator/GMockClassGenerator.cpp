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

#include "GMockClassGenerator.hpp"

void GMockClassGenerator::constructMockClass(const std::string& fileName, 
        const std::list<MockInfoStorage>& mockInfoList, const std::list<std::string>& includeList) {
    m_gmockClassGeneratorImpl.constructMockClass(fileName, mockInfoList, includeList);
}

void GMockClassGenerator::finalizeMocking() {
    // finishMocking is just adding #endif at end of the file located in ./GeneratedMocks folder.
    // So finishMocking() can be called from any generator objects
    m_gmockClassGeneratorImpl.finishMocking();
}
