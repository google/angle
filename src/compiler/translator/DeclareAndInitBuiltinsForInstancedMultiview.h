//
// Copyright (c) 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This pass applies the following modifications to the AST:
// 1) Replaces every occurrence of gl_InstanceID with InstanceID and marks InstanceID as internal.
// 2) Adds declarations of the global variables ViewID_OVR and InstanceID.
// 3) Initializes ViewID_OVR and InstanceID depending on the number of views.
// 4) Replaces every occurrence of gl_ViewID_OVR with ViewID_OVR and marks ViewID_OVR as internal.
// The pass should be executed before any variables get collected so that usage of gl_InstanceID is
// recorded.
//

#ifndef COMPILER_TRANSLATOR_DECLAREANDINITBUILTINSFORINSTANCEDMULTIVIEW_H_
#define COMPILER_TRANSLATOR_DECLAREANDINITBUILTINSFORINSTANCEDMULTIVIEW_H_

class TIntermBlock;

namespace sh
{

void DeclareAndInitBuiltinsForInstancedMultiview(TIntermBlock *root, unsigned numberOfViews);

}  // namespace sh

#endif  // COMPILER_TRANSLATOR_DECLAREANDINITBUILTINSFORINSTANCEDMULTIVIEW_H_