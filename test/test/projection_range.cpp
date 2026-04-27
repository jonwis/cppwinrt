#include "pch.h"
#include <filesystem>

#ifndef CPPWINRT_TEST_BASE_PROJECTION_DIR
#define CPPWINRT_TEST_BASE_PROJECTION_DIR ""
#endif

#ifndef CPPWINRT_TEST_RANGE_PROJECTION_DIR
#define CPPWINRT_TEST_RANGE_PROJECTION_DIR ""
#endif

TEST_CASE("projection_range_max_platform_filters_headers [projection-range]")
{
    std::filesystem::path base_projection{ CPPWINRT_TEST_BASE_PROJECTION_DIR };
    std::filesystem::path range_projection{ CPPWINRT_TEST_RANGE_PROJECTION_DIR };

    if (base_projection.empty() || range_projection.empty())
    {
        SUCCEED("Projection paths are not configured in this test mode.");
        return;
    }

    auto base_root = base_projection / "winrt";
    auto range_root = range_projection / "winrt";

    auto foundation_header = "Windows.Foundation.h";
    auto ai_ml_header = "Windows.AI.MachineLearning.h";

    REQUIRE(std::filesystem::exists(base_root / foundation_header));
    REQUIRE(std::filesystem::exists(range_root / foundation_header));

    if (!std::filesystem::exists(base_root / ai_ml_header))
    {
        SUCCEED("Current SDK projection does not include Windows.AI.MachineLearning.h; skipping contract omission assertion.");
        return;
    }

    REQUIRE_FALSE(std::filesystem::exists(range_root / ai_ml_header));
}
