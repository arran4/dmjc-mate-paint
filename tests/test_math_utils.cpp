#include <iostream>
#include <cmath>
#include <cassert>
#include <cstdlib>
#include "../math_utils.hpp"

#define ASSERT_NEAR(a, b, epsilon) \
    do { \
        if (std::abs((a) - (b)) > (epsilon)) { \
            std::cerr << "Assertion failed: " << #a << " (" << (a) << ") near " << #b << " (" << (b) << ") failed at " << __FILE__ << ":" << __LINE__ << std::endl; \
            exit(1); \
        } \
    } while (0)

void test_clamp_double() {
    std::cout << "Testing clamp_double..." << std::endl;

    // Within range
    ASSERT_NEAR(clamp_double(5.0, 0.0, 10.0), 5.0, 1e-9);
    ASSERT_NEAR(clamp_double(0.0, -1.0, 1.0), 0.0, 1e-9);

    // Below range
    ASSERT_NEAR(clamp_double(-5.0, 0.0, 10.0), 0.0, 1e-9);
    ASSERT_NEAR(clamp_double(-2.0, -1.0, 1.0), -1.0, 1e-9);

    // Above range
    ASSERT_NEAR(clamp_double(15.0, 0.0, 10.0), 10.0, 1e-9);
    ASSERT_NEAR(clamp_double(2.0, -1.0, 1.0), 1.0, 1e-9);

    // Edge cases
    ASSERT_NEAR(clamp_double(0.0, 0.0, 10.0), 0.0, 1e-9);
    ASSERT_NEAR(clamp_double(10.0, 0.0, 10.0), 10.0, 1e-9);

    // Min == Max
    ASSERT_NEAR(clamp_double(5.0, 5.0, 5.0), 5.0, 1e-9);
    ASSERT_NEAR(clamp_double(10.0, 5.0, 5.0), 5.0, 1e-9);
    ASSERT_NEAR(clamp_double(0.0, 5.0, 5.0), 5.0, 1e-9);

    std::cout << "clamp_double tests passed!" << std::endl;
}

void test_clamp_color_channel() {
    std::cout << "Testing clamp_color_channel..." << std::endl;

    ASSERT_NEAR(clamp_color_channel(0.5), 0.5, 1e-9);
    ASSERT_NEAR(clamp_color_channel(-0.1), 0.0, 1e-9);
    ASSERT_NEAR(clamp_color_channel(1.1), 1.0, 1e-9);
    ASSERT_NEAR(clamp_color_channel(0.0), 0.0, 1e-9);
    ASSERT_NEAR(clamp_color_channel(1.0), 1.0, 1e-9);

    std::cout << "clamp_color_channel tests passed!" << std::endl;
}

int main() {
    test_clamp_double();
    test_clamp_color_channel();
    std::cout << "All tests passed!" << std::endl;
    return 0;
}
