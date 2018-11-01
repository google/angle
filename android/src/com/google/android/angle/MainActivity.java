/*
 * Copyright 2018 The ANGLE Project Authors. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
package com.google.android.angle;

import android.app.Activity;
import android.os.Bundle;
import android.support.v7.preference.PreferenceManager;

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
