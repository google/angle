//
// Copyright (c) 2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_PREPROCESSOR_TOKEN_H_
#define COMPILER_PREPROCESSOR_TOKEN_H_

#include <string>
#include <vector>

namespace pp
{

class Token
{
  public:
    typedef int Location;

    Token();
    Token(Location l, int t);
    Token(Location l, int t, const std::string& s);

    static Location encodeLocation(int line, int file);
    static void decodeLocation(Location loc, int* line, int* file);

    Location location() const { return mLocation; }
    int type() const { return mType; }
    const std::string& value() const { return mValue; }

  private:
    Location mLocation;
    int mType;
    std::string mValue;
};

typedef std::vector<Token*> TokenVector;
typedef TokenVector::const_iterator TokenIterator;

}  // namepsace pp
#endif  // COMPILER_PREPROCESSOR_TOKEN_H_

