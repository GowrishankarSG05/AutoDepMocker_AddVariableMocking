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
#include <MockGeneratorTypes.hpp>

class IMockGenerator {
public:
    virtual ~IMockGenerator() = default;

public:
    /**
     * @brief Write include information to mock file
     * @example: <string>, <vector>, <External.hpp>
     * @param fileName: The mock file name
     * @param includes: List of include information
     */
    virtual void constructIncludes(const std::string& fileName, const std::vector<std::string>& includes) = 0;

    /**
     * @brief Write enum information to mock file
     * @param fileName: The mock file name
     * @param enumProp: List of enum information
     */
    virtual void constructEnum(const std::string& fileName, const std::vector<enumProperties>& enumProp) = 0;

    /**
     * @brief Write class information to mock file. Contains c++ class, method, function and operator
     *        overloading and templates
     * @param classInfo: Class information - @ref CustomType.hpp
     * @param calleeInfo: Method information - @ref CustomType.hpp
     */
    virtual void constructClass(const ClassInfo& classInfo, const std::vector<MethodInfo>& calleeInfo) = 0;

    /**
     * @brief Write C function information to mock file
     * @param fileName: The mock file name
     * @param methodsInfo: List of functions present in the file(fileName)
     */
    virtual void constructCFunction(const std::string& fileName, const std::vector<MethodInfo>& methodsInfo) = 0;

    /**
     * @brief Write field declartion to mock file
     * @param fileNme: The mock file name
     * @param fieldInfo: list of field information
     * @example struct foo, int x, char y
     */
    virtual void constructFieldDeclation(const std::string& fileName, const std::list<VariableInfoHierarchy>& fieldInfo) = 0;

    /**
     * @brief finalize mocking process
     *        Generator can utilize this for finalizing the mock class writting
     */
    virtual void finalizeMocking() = 0;
};

#endif // I_MOCK_GENERATOR_HPP_
