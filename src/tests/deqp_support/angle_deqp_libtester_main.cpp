//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// deqp_libtester_main.cpp: Entry point for tester DLL.

// Located in tcuMain.cc in dEQP's soruces.
int main(int argc, const char* argv[]);

// Exported to the tester app.
__declspec(dllexport) int deqp_libtester_main(int argc, const char *argv[])
{
    return main(argc, argv);
}
