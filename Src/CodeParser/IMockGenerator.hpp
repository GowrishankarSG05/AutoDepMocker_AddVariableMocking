/**
  * @file: IMockGenerator.hpp
  * @brief: Interface file of mock class generator
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
  * limitations under the License
  */

#ifndef I_MOCK_GENERATOR_HPP_
#define I_MOCK_GENERATOR_HPP_

#include <string>
#include <list>
#include <MockGeneratorTypes.hpp>

class IMockGenerator {
public:
    virtual ~IMockGenerator() = default;

public:
    /**
     * @brief Construct Mock class using given MockInfoStorage
     * @param fileName: The mock file name
     * @param mockInfoList: List of mock data identified in file fileName
     * @param includes: List of include information
     */
    virtual void constructMockClass(const std::string& fileName, const std::list<MockInfoStorage>& mockInfoList, 
                 const std::list<std::string>& includeList) = 0;

    /**
     * @brief finalize mocking process
     *        Generator can utilize this for finalizing the mock class writting
     */
    virtual void finalizeMocking() = 0;
};

#endif // I_MOCK_GENERATOR_HPP_
