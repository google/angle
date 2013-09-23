//
// Copyright (c) 2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include <math.h>
#include <stdlib.h>

#include <cerrno>
#include <limits>

#include "util.h"

#ifdef _MSC_VER
    #include <locale.h>
#else
    #include <sstream>
#endif

bool atof_clamp(const char *str, float *value)
{
    bool success = true;
#ifdef _MSC_VER
    _locale_t l = _create_locale(LC_NUMERIC, "C");
    double dvalue = _atof_l(str, l);
    _free_locale(l);
    if (errno == ERANGE || dvalue > std::numeric_limits<float>::max())
        success = false;
    else
        *value = static_cast<float>(dvalue);
#else
    std::istringstream s(str);
    std::locale l("C");
    s.imbue(l);
    s >> *value;
    if (s.fail())
        success = false;
#endif
    if (!success)
        *value = std::numeric_limits<float>::max();
    return success;
}

bool atoi_clamp(const char *str, int *value)
{
    long int lvalue = strtol(str, 0, 0);
    if (errno == ERANGE || lvalue > std::numeric_limits<int>::max())
    {
        *value = std::numeric_limits<int>::max();
        return  false;
    }
    *value = static_cast<int>(lvalue);
    return true;
}

