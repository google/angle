//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Version.h: Encapsulation of a GL version.

#ifndef LIBANGLE_VERSION_H_
#define LIBANGLE_VERSION_H_

#include <cstdint>

namespace gl
{

struct alignas(uint16_t) Version
{
    // Avoid conflicts with linux system defines
#undef major
#undef minor

    constexpr Version() : minor(0), major(0) {}
    constexpr Version(uint8_t major_, uint8_t minor_) : minor(minor_), major(major_) {}

    constexpr bool operator==(const Version &other) const { return value() == other.value(); }
    constexpr bool operator!=(const Version &other) const { return value() != other.value(); }
    constexpr bool operator>=(const Version &other) const { return value() >= other.value(); }
    constexpr bool operator<=(const Version &other) const { return value() <= other.value(); }
    constexpr bool operator<(const Version &other) const { return value() < other.value(); }
    constexpr bool operator>(const Version &other) const { return value() > other.value(); }

    // These functions should not be used for compare operations.
    constexpr uint32_t getMajor() const { return major; }
    constexpr uint32_t getMinor() const { return minor; }

    // Declaring minor before major makes the value() function
    // return the struct's bytes as-is, as a single value.
    uint8_t minor;
    uint8_t major;

  private:
    constexpr uint16_t value() const { return (major << 8) | minor; }
};
static_assert(sizeof(Version) == 2);

static_assert(Version().getMajor() == 0 && Version().getMinor() == 0);
static_assert(Version(4, 6) == Version(4, 6));
static_assert(Version(1, 0) != Version(1, 1));
static_assert(Version(1, 0) != Version(2, 0));
static_assert(Version(2, 0) > Version(1, 0));
static_assert(Version(3, 1) > Version(3, 0));
static_assert(Version(3, 0) > Version(1, 1));
static_assert(Version(2, 0) < Version(3, 0));
static_assert(Version(3, 1) < Version(3, 2));
static_assert(Version(1, 1) < Version(2, 0));

}  // namespace gl

#endif  // LIBANGLE_VERSION_H_
