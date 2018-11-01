/*
 * Copyright 2018 The ANGLE Project Authors. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
package com.android.angle;

import android.app.Activity;
import android.os.Bundle;

import com.android.angle.common.*;

public class MainActivity extends Activity
{
    private final String TAG = this.getClass().getSimpleName();

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.fragment);
    }
}
