#include <stdio.h>
#include <assert.h>
#include "gp_point_array.h"

#define ASSERT_RECT(r, ex, ey, ew, eh) do { \
    if ((r).x != (ex) || (r).y != (ey) || (r).width != (ew) || (r).height != (eh)) { \
        fprintf(stderr, "Assertion failed: expected {%d, %d, %d, %d}, got {%d, %d, %d, %d}\n", \
                (ex), (ey), (ew), (eh), (r).x, (r).y, (r).width, (r).height); \
        assert(0); \
    } \
} while(0)

void test_empty_array() {
    gp_point_array *pa = gp_point_array_new();
    GdkRectangle rect;
    GdkRectangle rect_max = {0, 0, 100, 100};

    gp_point_array_get_clipbox(pa, &rect, 0, &rect_max);
    ASSERT_RECT(rect, 0, 0, 0, 0);

    gp_point_array_get_clipbox(pa, &rect, 10, NULL);
    ASSERT_RECT(rect, 0, 0, 0, 0);

    gp_point_array_free(pa);
    printf("test_empty_array passed\n");
}

void test_single_point() {
    gp_point_array *pa = gp_point_array_new();
    gp_point_array_append(pa, 50, 50);
    GdkRectangle rect;

    // pixel_width = 0
    gp_point_array_get_clipbox(pa, &rect, 0, NULL);
    ASSERT_RECT(rect, 50, 50, 1, 1);

    // pixel_width = 1
    gp_point_array_get_clipbox(pa, &rect, 1, NULL);
    ASSERT_RECT(rect, 50, 50, 1, 1);

    // pixel_width = 2
    gp_point_array_get_clipbox(pa, &rect, 2, NULL);
    ASSERT_RECT(rect, 49, 49, 3, 3);

    // pixel_width = 3
    gp_point_array_get_clipbox(pa, &rect, 3, NULL);
    ASSERT_RECT(rect, 49, 49, 3, 3);

    gp_point_array_free(pa);
    printf("test_single_point passed\n");
}

void test_multiple_points() {
    gp_point_array *pa = gp_point_array_new();
    gp_point_array_append(pa, 10, 10);
    gp_point_array_append(pa, 20, 30);
    gp_point_array_append(pa, 5, 15);
    GdkRectangle rect;

    gp_point_array_get_clipbox(pa, &rect, 0, NULL);
    ASSERT_RECT(rect, 5, 10, 16, 21);

    gp_point_array_free(pa);
    printf("test_multiple_points passed\n");
}

void test_negative_coordinates() {
    gp_point_array *pa = gp_point_array_new();
    gp_point_array_append(pa, -10, -10);
    gp_point_array_append(pa, 10, 10);
    GdkRectangle rect;

    gp_point_array_get_clipbox(pa, &rect, 0, NULL);
    ASSERT_RECT(rect, -10, -10, 21, 21);

    gp_point_array_free(pa);
    printf("test_negative_coordinates passed\n");
}

void test_rect_max_clipping_partial() {
    gp_point_array *pa = gp_point_array_new();
    gp_point_array_append(pa, 10, 10);
    gp_point_array_append(pa, 100, 100);
    GdkRectangle rect;
    GdkRectangle rect_max = {20, 20, 50, 50}; // 20,20 to 69,69

    gp_point_array_get_clipbox(pa, &rect, 0, &rect_max);
    ASSERT_RECT(rect, 20, 20, 50, 50);

    gp_point_array_free(pa);
    printf("test_rect_max_clipping_partial passed\n");
}

void test_rect_max_clipping_entirely_outside() {
    gp_point_array *pa = gp_point_array_new();
    gp_point_array_append(pa, 100, 100);
    gp_point_array_append(pa, 110, 110);
    GdkRectangle rect;
    GdkRectangle rect_max = {0, 0, 50, 50}; // 0,0 to 49,49

    gp_point_array_get_clipbox(pa, &rect, 0, &rect_max);

    // We expect it to be empty
    assert(rect.width == 0);
    assert(rect.height == 0);

    gp_point_array_free(pa);
    printf("test_rect_max_clipping_entirely_outside passed\n");
}

void test_large_pixel_width() {
    gp_point_array *pa = gp_point_array_new();
    gp_point_array_append(pa, 50, 50);
    GdkRectangle rect;

    gp_point_array_get_clipbox(pa, &rect, 100, NULL);
    ASSERT_RECT(rect, 0, 0, 101, 101); // 50-50 to 50+50

    gp_point_array_free(pa);
    printf("test_large_pixel_width passed\n");
}

int main() {
    test_empty_array();
    test_single_point();
    test_multiple_points();
    test_negative_coordinates();
    test_rect_max_clipping_partial();
    test_rect_max_clipping_entirely_outside();
    test_large_pixel_width();
    printf("All tests passed!\n");
    return 0;
}
