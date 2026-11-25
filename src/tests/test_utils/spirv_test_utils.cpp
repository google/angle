//
// Copyright 2026 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "test_utils/spirv_test_utils.h"

#include <sstream>

#include <spirv-tools/libspirv.hpp>

#include "common/span.h"

namespace angle
{
namespace
{
constexpr size_t kHeaderWords = 5;

testing::AssertionResult ParseSpirv(const std::vector<uint32_t> &binary,
                                    void *userData,
                                    spv_parsed_instruction_fn_t parseInstruction)
{
    if (binary.size() <= kHeaderWords)
    {
        return testing::AssertionFailure() << "Missing SPIR-V header or instructions";
    }

    // The universal grammar covers both SPIR-V 1.3 and 1.4 output.
    spvtools::Context context(SPV_ENV_UNIVERSAL_1_6);
    spv_diagnostic diagnostic = nullptr;
    const spv_result_t result =
        spvBinaryParse(context.CContext(), userData, binary.data(), binary.size(), nullptr,
                       parseInstruction, &diagnostic);
    testing::AssertionResult assertion = testing::AssertionSuccess();
    if (result != SPV_SUCCESS)
    {
        assertion = testing::AssertionFailure();
        assertion << "Invalid SPIR-V: "
                  << (diagnostic ? diagnostic->error : "instruction parsing failed");
    }
    spvDiagnosticDestroy(diagnostic);
    return assertion;
}

struct DecorationCount
{
    spv::Op instruction;
    spv::Decoration decoration;
    size_t count = 0;
};

spv_result_t CountDecoration(void *userData, const spv_parsed_instruction_t *instruction)
{
    DecorationCount &query = *static_cast<DecorationCount *>(userData);
    // SAFETY: SPIRV-Tools supplies num_words words and checks the operands before this callback.
    const auto words = ANGLE_UNSAFE_BUFFERS(Span(instruction->words, instruction->num_words));
    if (instruction->opcode == query.instruction &&
        words[query.instruction == spv::OpDecorate ? 2 : 3] == query.decoration)
    {
        ++query.count;
    }
    return SPV_SUCCESS;
}
}  // namespace

testing::AssertionResult GetSpirvFromDebugInfo(const std::string &debugInfo,
                                               std::vector<uint32_t> *binary)
{
    binary->clear();

    const size_t start = debugInfo.find("0x07230203");
    if (start == std::string::npos)
    {
        return testing::AssertionFailure() << "No SPIR-V payload in translated shader output";
    }

    std::istringstream stream(debugInfo.substr(start));
    stream >> std::hex;
    std::vector<uint32_t> words;
    uint32_t word;
    char comma;
    // ShaderVk writes each word as 0xXXXXXXXX followed by a comma, including the last word.
    while (stream >> word >> comma)
    {
        if (comma != ',')
        {
            return testing::AssertionFailure()
                   << "Expected a comma-terminated SPIR-V hex word at word " << words.size();
        }
        words.push_back(word);
    }

    testing::AssertionResult result = ParseSpirv(words, nullptr, nullptr);
    if (result)
    {
        binary->swap(words);
    }
    return result;
}

testing::AssertionResult VerifySpirvDecorationCount(const std::vector<uint32_t> &binary,
                                                    spv::Op instruction,
                                                    spv::Decoration decoration,
                                                    size_t expectedCount)
{
    if (instruction != spv::OpDecorate && instruction != spv::OpMemberDecorate)
    {
        return testing::AssertionFailure() << "Expected OpDecorate or OpMemberDecorate";
    }
    DecorationCount query{instruction, decoration};
    testing::AssertionResult result = ParseSpirv(binary, &query, CountDecoration);
    if (!result)
    {
        return result;
    }
    if (query.count != expectedCount)
    {
        return testing::AssertionFailure()
               << "Expected " << expectedCount << " Op" << spvOpcodeString(instruction)
               << " decorations with value " << static_cast<uint32_t>(decoration) << ", found "
               << query.count;
    }
    return testing::AssertionSuccess();
}
}  // namespace angle
