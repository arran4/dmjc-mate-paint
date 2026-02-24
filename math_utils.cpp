#include "math_utils.hpp"
#include <algorithm>

double clamp_double(double value, double min_value, double max_value) {
    return std::max(min_value, std::min(value, max_value));
}

double clamp_color_channel(double channel) {
    return clamp_double(channel, 0.0, 1.0);
}
