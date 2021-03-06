/*
 * Copyright (c) 2017-2019 ARM Limited.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "arm_compute/core/Types.h"
#include "arm_compute/runtime/NEON/functions/NEQuantizationLayer.h"
#include "arm_compute/runtime/Tensor.h"
#include "arm_compute/runtime/TensorAllocator.h"
#include "tests/NEON/Accessor.h"
#include "tests/PaddingCalculator.h"
#include "tests/datasets/ShapeDatasets.h"
#include "tests/framework/Asserts.h"
#include "tests/framework/Macros.h"
#include "tests/framework/datasets/Datasets.h"
#include "tests/validation/Validation.h"
#include "tests/validation/fixtures/QuantizationLayerFixture.h"

namespace arm_compute
{
namespace test
{
namespace validation
{
namespace
{
/** Tolerance for quantization */
constexpr AbsoluteTolerance<uint8_t>  tolerance_u8(1);
constexpr AbsoluteTolerance<uint16_t> tolerance_u16(1);

const auto QuantizationSmallShapes = concat(datasets::Small3DShapes(), datasets::Small4DShapes());
const auto QuantizationLargeShapes = concat(datasets::Large3DShapes(), datasets::Large4DShapes());
} // namespace

TEST_SUITE(NEON)
TEST_SUITE(QuantizationLayer)

// *INDENT-OFF*
// clang-format off
DATA_TEST_CASE(Validate, framework::DatasetMode::ALL, zip(zip(
               framework::dataset::make("InputInfo", { TensorInfo(TensorShape(16U, 16U, 16U, 5U), 1, DataType::QASYMM8),  // Wrong input data type
                                                       TensorInfo(TensorShape(16U, 16U, 16U, 5U), 1, DataType::F32),  // Wrong output data type
                                                       TensorInfo(TensorShape(16U, 16U, 2U, 5U), 1, DataType::F32),  // Missmatching shapes
                                                       TensorInfo(TensorShape(16U, 16U, 16U, 5U), 1, DataType::F32),  // Valid
                                                     }),
               framework::dataset::make("OutputInfo",{ TensorInfo(TensorShape(16U, 16U, 16U, 5U), 1, DataType::F32),
                                                       TensorInfo(TensorShape(16U, 16U, 16U, 5U), 1, DataType::U16),
                                                       TensorInfo(TensorShape(16U, 16U, 16U, 5U), 1, DataType::QASYMM8),
                                                       TensorInfo(TensorShape(16U, 16U, 16U, 5U), 1, DataType::QASYMM8),
                                                     })),
               framework::dataset::make("Expected", { false, false, false, true})),
               input_info, output_info, expected)
{
    ARM_COMPUTE_EXPECT(bool(NEQuantizationLayer::validate(&input_info.clone()->set_is_resizable(false), &output_info.clone()->set_is_resizable(false))) == expected, framework::LogLevel::ERRORS);
}
// clang-format on
// *INDENT-ON*

DATA_TEST_CASE(Configuration, framework::DatasetMode::ALL, combine(QuantizationSmallShapes, framework::dataset::make("DataType", DataType::F32)), shape, data_type)
{
    // Create tensors
    Tensor src = create_tensor<Tensor>(shape, data_type);
    Tensor dst = create_tensor<Tensor>(shape, DataType::QASYMM8);

    ARM_COMPUTE_EXPECT(src.info()->is_resizable(), framework::LogLevel::ERRORS);
    ARM_COMPUTE_EXPECT(dst.info()->is_resizable(), framework::LogLevel::ERRORS);

    // Create and configure function
    NEQuantizationLayer quant_layer;
    quant_layer.configure(&src, &dst);

    // Validate valid region
    const ValidRegion valid_region = shape_to_valid_region(shape);
    validate(src.info()->valid_region(), valid_region);
    validate(dst.info()->valid_region(), valid_region);

    // Validate padding
    validate(src.info()->padding(), PaddingSize());
    validate(dst.info()->padding(), PaddingSize());
}

template <typename T>
using NEQuantizationLayerQASYMM8Fixture = QuantizationValidationFixture<Tensor, Accessor, NEQuantizationLayer, T, uint8_t>;
template <typename T>
using NEQuantizationLayerQASYMM8SignedFixture = QuantizationValidationFixture<Tensor, Accessor, NEQuantizationLayer, T, int8_t>;
template <typename T>
using NEQuantizationLayerQASYMM16Fixture = QuantizationValidationFixture<Tensor, Accessor, NEQuantizationLayer, T, uint16_t>;

TEST_SUITE(Float)
TEST_SUITE(FP32)
FIXTURE_DATA_TEST_CASE(RunSmallQASYMM8, NEQuantizationLayerQASYMM8Fixture<float>, framework::DatasetMode::PRECOMMIT, combine(combine(combine(QuantizationSmallShapes,
                       framework::dataset::make("DataType", DataType::F32)),
                       framework::dataset::make("DataTypeOut", { DataType::QASYMM8 })),
                       framework::dataset::make("QuantizationInfo", { QuantizationInfo(0.5f, 10) })))
{
    // Validate output
    validate(Accessor(_target), _reference, tolerance_u8);
}
FIXTURE_DATA_TEST_CASE(RunSmallQASYMM8Signed, NEQuantizationLayerQASYMM8SignedFixture<float>, framework::DatasetMode::ALL, combine(combine(combine(QuantizationSmallShapes,
                       framework::dataset::make("DataType", DataType::F32)),
                       framework::dataset::make("DataTypeOut", { DataType::QASYMM8_SIGNED })),
                       framework::dataset::make("QuantizationInfo", { QuantizationInfo(0.5f, 10) })))
{
    // Validate output
    validate(Accessor(_target), _reference, tolerance_u8);
}
FIXTURE_DATA_TEST_CASE(RunSmallQASYMM16, NEQuantizationLayerQASYMM16Fixture<float>, framework::DatasetMode::PRECOMMIT, combine(combine(combine(QuantizationSmallShapes,
                       framework::dataset::make("DataType", DataType::F32)),
                       framework::dataset::make("DataTypeOut", { DataType::QASYMM16 })),
                       framework::dataset::make("QuantizationInfo", { QuantizationInfo(0.5f, 10) })))
{
    // Validate output
    validate(Accessor(_target), _reference, tolerance_u16);
}
FIXTURE_DATA_TEST_CASE(RunLargeQASYMM8, NEQuantizationLayerQASYMM8Fixture<float>, framework::DatasetMode::NIGHTLY, combine(combine(combine(QuantizationLargeShapes,
                       framework::dataset::make("DataType", DataType::F32)),
                       framework::dataset::make("DataTypeOut", { DataType::QASYMM8 })),
                       framework::dataset::make("QuantizationInfo", { QuantizationInfo(0.5f, 10) })))
{
    // Validate output
    validate(Accessor(_target), _reference, tolerance_u8);
}
FIXTURE_DATA_TEST_CASE(RunLargeQASYMM16, NEQuantizationLayerQASYMM16Fixture<float>, framework::DatasetMode::NIGHTLY, combine(combine(combine(QuantizationLargeShapes,
                       framework::dataset::make("DataType", DataType::F32)),
                       framework::dataset::make("DataTypeOut", { DataType::QASYMM16 })),
                       framework::dataset::make("QuantizationInfo", { QuantizationInfo(0.5f, 10) })))
{
    // Validate output
    validate(Accessor(_target), _reference, tolerance_u16);
}
TEST_SUITE_END() // FP32
#ifdef __ARM_FEATURE_FP16_VECTOR_ARITHMETIC
TEST_SUITE(FP16)
FIXTURE_DATA_TEST_CASE(RunSmallQASYMM8, NEQuantizationLayerQASYMM8Fixture<half>, framework::DatasetMode::PRECOMMIT, combine(combine(combine(QuantizationSmallShapes,
                       framework::dataset::make("DataType", DataType::F16)),
                       framework::dataset::make("DataTypeOut", { DataType::QASYMM8 })),
                       framework::dataset::make("QuantizationInfo", { QuantizationInfo(0.5f, 10) })))
{
    // Validate output
    validate(Accessor(_target), _reference, tolerance_u8);
}
FIXTURE_DATA_TEST_CASE(RunSmallQASYMM8Signed, NEQuantizationLayerQASYMM8SignedFixture<half>, framework::DatasetMode::ALL, combine(combine(combine(QuantizationSmallShapes,
                       framework::dataset::make("DataType", DataType::F16)),
                       framework::dataset::make("DataTypeOut", { DataType::QASYMM8_SIGNED })),
                       framework::dataset::make("QuantizationInfo", { QuantizationInfo(0.5f, 10) })))
{
    // Validate output
    validate(Accessor(_target), _reference, tolerance_u8);
}
FIXTURE_DATA_TEST_CASE(RunSmallQASYMM16, NEQuantizationLayerQASYMM16Fixture<half>, framework::DatasetMode::PRECOMMIT, combine(combine(combine(QuantizationSmallShapes,
                       framework::dataset::make("DataType", DataType::F16)),
                       framework::dataset::make("DataTypeOut", { DataType::QASYMM16 })),
                       framework::dataset::make("QuantizationInfo", { QuantizationInfo(0.5f, 10) })))
{
    // Validate output
    validate(Accessor(_target), _reference, tolerance_u16);
}
FIXTURE_DATA_TEST_CASE(RunLargeQASYMM8, NEQuantizationLayerQASYMM8Fixture<half>, framework::DatasetMode::NIGHTLY, combine(combine(combine(QuantizationLargeShapes,
                       framework::dataset::make("DataType", DataType::F16)),
                       framework::dataset::make("DataTypeOut", { DataType::QASYMM8 })),
                       framework::dataset::make("QuantizationInfo", { QuantizationInfo(0.5f, 10) })))
{
    // Validate output
    validate(Accessor(_target), _reference, tolerance_u8);
}
FIXTURE_DATA_TEST_CASE(RunLargeQASYMM16, NEQuantizationLayerQASYMM16Fixture<half>, framework::DatasetMode::NIGHTLY, combine(combine(combine(QuantizationLargeShapes,
                       framework::dataset::make("DataType", DataType::F16)),
                       framework::dataset::make("DataTypeOut", { DataType::QASYMM16 })),
                       framework::dataset::make("QuantizationInfo", { QuantizationInfo(0.5f, 10) })))
{
    // Validate output
    validate(Accessor(_target), _reference, tolerance_u16);
}
TEST_SUITE_END() // FP16
#endif           //  __ARM_FEATURE_FP16_VECTOR_ARITHMETIC
TEST_SUITE_END() // Float

TEST_SUITE_END() // QuantizationLayer
TEST_SUITE_END() // NEON
} // namespace validation
} // namespace test
} // namespace arm_compute
