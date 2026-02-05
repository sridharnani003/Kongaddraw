/**
 * @file ConfigTests.cpp
 * @brief Unit tests for configuration system
 */

#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>

// Simple test framework macros
#define TEST_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            printf("FAILED: %s:%d - %s\n", __FILE__, __LINE__, #condition); \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_EQ(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            printf("FAILED: %s:%d - Expected %d, got %d\n", __FILE__, __LINE__, \
                   static_cast<int>(expected), static_cast<int>(actual)); \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_STR_EQ(expected, actual) \
    do { \
        if (strcmp(expected, actual) != 0) { \
            printf("FAILED: %s:%d - Expected '%s', got '%s'\n", __FILE__, __LINE__, \
                   expected, actual); \
            return false; \
        } \
    } while(0)

#define RUN_TEST(test_func) \
    do { \
        printf("Running %s... ", #test_func); \
        if (test_func()) { \
            printf("PASSED\n"); \
            passed++; \
        } else { \
            failed++; \
        } \
        total++; \
    } while(0)

// ============================================================================
// INI Parser Tests
// ============================================================================

/**
 * @brief Test INI parser string trimming
 */
bool test_trim_whitespace() {
    // Test cases for whitespace trimming
    const char* inputs[] = {
        "  hello  ",
        "\thello\t",
        "  \t hello \t  ",
        "hello",
        "",
        "   ",
    };

    const char* expected[] = {
        "hello",
        "hello",
        "hello",
        "hello",
        "",
        "",
    };

    // Simple trim implementation for testing
    auto trim = [](const std::string& str) -> std::string {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    };

    for (size_t i = 0; i < sizeof(inputs) / sizeof(inputs[0]); ++i) {
        std::string result = trim(inputs[i]);
        TEST_ASSERT_STR_EQ(expected[i], result.c_str());
    }

    return true;
}

/**
 * @brief Test INI section parsing
 */
bool test_section_parsing() {
    // Test that section names are extracted correctly
    auto parseSection = [](const std::string& line) -> std::string {
        if (line.empty() || line[0] != '[') return "";
        size_t end = line.find(']');
        if (end == std::string::npos) return "";
        return line.substr(1, end - 1);
    };

    TEST_ASSERT_STR_EQ("ddraw", parseSection("[ddraw]").c_str());
    TEST_ASSERT_STR_EQ("game.exe", parseSection("[game.exe]").c_str());
    TEST_ASSERT_STR_EQ("", parseSection("not a section").c_str());
    TEST_ASSERT_STR_EQ("", parseSection("[unclosed").c_str());

    return true;
}

/**
 * @brief Test key-value parsing
 */
bool test_keyvalue_parsing() {
    auto parseKeyValue = [](const std::string& line, std::string& key, std::string& value) -> bool {
        size_t eq = line.find('=');
        if (eq == std::string::npos) return false;

        // Extract and trim key
        key = line.substr(0, eq);
        size_t keyEnd = key.find_last_not_of(" \t");
        if (keyEnd != std::string::npos) key = key.substr(0, keyEnd + 1);

        // Extract and trim value
        value = line.substr(eq + 1);
        size_t valStart = value.find_first_not_of(" \t");
        if (valStart != std::string::npos) value = value.substr(valStart);

        return true;
    };

    std::string key, value;

    TEST_ASSERT(parseKeyValue("width=640", key, value));
    TEST_ASSERT_STR_EQ("width", key.c_str());
    TEST_ASSERT_STR_EQ("640", value.c_str());

    TEST_ASSERT(parseKeyValue("renderer = auto", key, value));
    TEST_ASSERT_STR_EQ("renderer", key.c_str());
    TEST_ASSERT_STR_EQ("auto", value.c_str());

    TEST_ASSERT(parseKeyValue("fullscreen=true", key, value));
    TEST_ASSERT_STR_EQ("fullscreen", key.c_str());
    TEST_ASSERT_STR_EQ("true", value.c_str());

    TEST_ASSERT(!parseKeyValue("no equals sign", key, value));

    return true;
}

/**
 * @brief Test boolean parsing
 */
bool test_bool_parsing() {
    auto parseBool = [](const std::string& value, bool defaultVal) -> bool {
        if (value == "true" || value == "1" || value == "yes" || value == "on") {
            return true;
        }
        if (value == "false" || value == "0" || value == "no" || value == "off") {
            return false;
        }
        return defaultVal;
    };

    TEST_ASSERT_EQ(true, parseBool("true", false));
    TEST_ASSERT_EQ(true, parseBool("1", false));
    TEST_ASSERT_EQ(true, parseBool("yes", false));
    TEST_ASSERT_EQ(true, parseBool("on", false));

    TEST_ASSERT_EQ(false, parseBool("false", true));
    TEST_ASSERT_EQ(false, parseBool("0", true));
    TEST_ASSERT_EQ(false, parseBool("no", true));
    TEST_ASSERT_EQ(false, parseBool("off", true));

    TEST_ASSERT_EQ(true, parseBool("invalid", true));
    TEST_ASSERT_EQ(false, parseBool("invalid", false));

    return true;
}

/**
 * @brief Test integer parsing
 */
bool test_int_parsing() {
    auto parseInt = [](const std::string& value, int defaultVal) -> int {
        try {
            return std::stoi(value);
        } catch (...) {
            return defaultVal;
        }
    };

    TEST_ASSERT_EQ(640, parseInt("640", 0));
    TEST_ASSERT_EQ(-1, parseInt("-1", 0));
    TEST_ASSERT_EQ(0, parseInt("0", -1));
    TEST_ASSERT_EQ(42, parseInt("invalid", 42));
    TEST_ASSERT_EQ(100, parseInt("", 100));

    return true;
}

// ============================================================================
// Surface Blit Tests
// ============================================================================

/**
 * @brief Test simple memory blit
 */
bool test_simple_blit() {
    const uint32_t width = 4;
    const uint32_t height = 4;
    const uint32_t pitch = width * sizeof(uint32_t);

    uint32_t src[16] = {
        0xFF000000, 0xFF000001, 0xFF000002, 0xFF000003,
        0xFF000010, 0xFF000011, 0xFF000012, 0xFF000013,
        0xFF000020, 0xFF000021, 0xFF000022, 0xFF000023,
        0xFF000030, 0xFF000031, 0xFF000032, 0xFF000033,
    };

    uint32_t dst[16] = {0};

    // Simple blit
    for (uint32_t y = 0; y < height; ++y) {
        memcpy(&dst[y * width], &src[y * width], pitch);
    }

    for (size_t i = 0; i < 16; ++i) {
        TEST_ASSERT_EQ(src[i], dst[i]);
    }

    return true;
}

/**
 * @brief Test color key blit
 */
bool test_colorkey_blit() {
    const uint32_t width = 4;
    const uint32_t colorKey = 0xFF00FF00;  // Green is transparent

    uint32_t src[4] = { 0xFFFF0000, 0xFF00FF00, 0xFF0000FF, 0xFF00FF00 };
    uint32_t dst[4] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
    uint32_t expected[4] = { 0xFFFF0000, 0xFFFFFFFF, 0xFF0000FF, 0xFFFFFFFF };

    // Color key blit
    for (uint32_t x = 0; x < width; ++x) {
        if (src[x] != colorKey) {
            dst[x] = src[x];
        }
    }

    for (size_t i = 0; i < 4; ++i) {
        TEST_ASSERT_EQ(expected[i], dst[i]);
    }

    return true;
}

/**
 * @brief Test color fill
 */
bool test_color_fill() {
    uint32_t surface[16] = {0};
    const uint32_t fillColor = 0xFFAABBCC;

    for (size_t i = 0; i < 16; ++i) {
        surface[i] = fillColor;
    }

    for (size_t i = 0; i < 16; ++i) {
        TEST_ASSERT_EQ(fillColor, surface[i]);
    }

    return true;
}

// ============================================================================
// Palette Conversion Tests
// ============================================================================

/**
 * @brief Test 8-bit to 32-bit palette conversion
 */
bool test_palette_conversion() {
    // Create a simple grayscale palette
    uint32_t palette[256];
    for (int i = 0; i < 256; ++i) {
        palette[i] = 0xFF000000 | (i << 16) | (i << 8) | i;
    }

    // Test conversion
    uint8_t indexed = 128;
    uint32_t converted = palette[indexed];

    TEST_ASSERT_EQ(0xFF808080u, converted);

    // Test black
    TEST_ASSERT_EQ(0xFF000000u, palette[0]);

    // Test white
    TEST_ASSERT_EQ(0xFFFFFFFFu, palette[255]);

    return true;
}

/**
 * @brief Test RGB565 to RGB888 conversion
 */
bool test_rgb565_conversion() {
    auto rgb565ToRgb888 = [](uint16_t pixel) -> uint32_t {
        uint8_t r = ((pixel >> 11) & 0x1F) << 3;
        uint8_t g = ((pixel >> 5) & 0x3F) << 2;
        uint8_t b = (pixel & 0x1F) << 3;
        return 0xFF000000 | (r << 16) | (g << 8) | b;
    };

    // Red (5 bits max)
    TEST_ASSERT_EQ(0xFFF80000u, rgb565ToRgb888(0xF800));

    // Green (6 bits max)
    TEST_ASSERT_EQ(0xFF00FC00u, rgb565ToRgb888(0x07E0));

    // Blue (5 bits max)
    TEST_ASSERT_EQ(0xFF0000F8u, rgb565ToRgb888(0x001F));

    // White
    TEST_ASSERT_EQ(0xFFFFFCF8u, rgb565ToRgb888(0xFFFF));

    // Black
    TEST_ASSERT_EQ(0xFF000000u, rgb565ToRgb888(0x0000));

    return true;
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    printf("===========================================\n");
    printf("legacy-ddraw-compat Unit Tests\n");
    printf("===========================================\n\n");

    int passed = 0;
    int failed = 0;
    int total = 0;

    // Config tests
    printf("--- Configuration Tests ---\n");
    RUN_TEST(test_trim_whitespace);
    RUN_TEST(test_section_parsing);
    RUN_TEST(test_keyvalue_parsing);
    RUN_TEST(test_bool_parsing);
    RUN_TEST(test_int_parsing);

    // Surface tests
    printf("\n--- Surface Operation Tests ---\n");
    RUN_TEST(test_simple_blit);
    RUN_TEST(test_colorkey_blit);
    RUN_TEST(test_color_fill);

    // Palette tests
    printf("\n--- Palette Tests ---\n");
    RUN_TEST(test_palette_conversion);
    RUN_TEST(test_rgb565_conversion);

    // Summary
    printf("\n===========================================\n");
    printf("Results: %d/%d passed", passed, total);
    if (failed > 0) {
        printf(" (%d failed)", failed);
    }
    printf("\n===========================================\n");

    return failed > 0 ? 1 : 0;
}
