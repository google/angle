/*
 * Copyright 2018 The ANGLE Project Authors. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
package com.android.angle.common;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.util.Log;

public class Receiver extends BroadcastReceiver
{

    private final static String TAG = Receiver.class.getSimpleName();

    @Override
    public void onReceive(Context context, Intent intent)
    {
        // Nothing to do yet...
    }

    static void updateAllUseAngle(Context context)
    {
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(context);
        String allUseAngleKey   = context.getString(R.string.pref_key_all_angle);
        boolean allUseAngle     = prefs.getBoolean(allUseAngleKey, false);

        GlobalSettings.updateAllUseAngle(context, allUseAngle);

        Log.v(TAG, "All PKGs use ANGLE set to: " + allUseAngle);
    }

    static void updateShowAngleInUseDialogBox(Context context)
    {
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(context);
        String showAngleInUseDialogBoxKey =
                context.getString(R.string.pref_key_angle_in_use_dialog);
        boolean showAngleInUseDialogBox = prefs.getBoolean(showAngleInUseDialogBoxKey, false);

        GlobalSettings.updateShowAngleInUseDialog(context, showAngleInUseDialogBox);

        Log.v(TAG, "Show 'ANGLE In Use' dialog box set to: " + showAngleInUseDialogBox);
    }
}
