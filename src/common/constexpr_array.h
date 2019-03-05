//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// constexpr_array:
//   C++14 relaxes constexpr requirements enough that we can support sorting
//   a constexpr std::array at compile-time. This is useful for defining fast
//   lookup tables and other data at compile-time.
//   The code is adapted from https://stackoverflow.com/a/40030044.

#ifndef COMMON_CONSTEXPR_ARRAY_H_
#define COMMON_CONSTEXPR_ARRAY_H_

#include <algorithm>

namespace angle
{
template <class T>
constexpr void constexpr_swap(T &l, T &r)
{
    T tmp = std::move(l);
    l     = std::move(r);
    r     = std::move(tmp);
}

template <typename T, size_t N>
struct constexpr_array
{
    constexpr T &operator[](size_t i) { return arr[i]; }

    constexpr const T &operator[](size_t i) const { return arr[i]; }

    constexpr const T *begin() const { return arr; }
    constexpr const T *end() const { return arr + N; }

    T arr[N];
};

namespace priv
{
template <typename T, size_t N>
size_t hoare_partition(constexpr_array<T, N> &arr, size_t left, size_t right)
{
    const T pivot = arr[(left + right) >> 1];
    size_t i      = left - 1;
    size_t j      = right + 1;

    for (;;)
    {
        do
        {
            i++;
        } while (arr[i] < pivot);

        do
        {
            j--;
        } while (arr[j] > pivot);

        if (i >= j)
        {
            return j;
        }

        constexpr_swap(arr[i], arr[j]);
    }

    return 0;
}

template <typename T, size_t N>
void constexpr_sort_impl(constexpr_array<T, N> &arr, size_t left, size_t right)
{
    if (left < right)
    {
        size_t p = priv::hoare_partition(arr, left, right);
        constexpr_sort_impl(arr, left, p);
        constexpr_sort_impl(arr, p + 1, right);
    }
}
}  // namespace priv

// Note that std::sort in constexpr in c++20. So this implementation can be
// removed given sufficient STL support.
template <typename T, size_t N>
constexpr_array<T, N> constexpr_sort(constexpr_array<T, N> arr)
{
    auto sorted = arr;
    priv::constexpr_sort_impl(sorted, 0, N - 1);
    return sorted;
}

template <typename T, size_t N>
bool constexpr_array_contains(const constexpr_array<T, N> &haystack, const T &needle)
{
    const T *found = std::lower_bound(haystack.begin(), haystack.end(), needle);
    return (found && *found == needle);
}
}  // namespace angle

#endif  // COMMON_CONSTEXPR_ARRAY_H_
