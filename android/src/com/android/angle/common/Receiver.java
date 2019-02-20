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

import java.io.InputStream;
import java.io.IOException;

import org.json.JSONObject;
import org.json.JSONArray;
import org.json.JSONException;

public class Receiver extends BroadcastReceiver
{

    private final static String TAG = "AngleReceiver";
    private final static String ANGLE_RULES_FILE = "a4a_rules.json";

    @Override
    public void onReceive(Context context, Intent intent)
    {
        Log.v(TAG, "Received intent, updating ANGLE whitelist...");

        String jsonStr = loadRules(context);

        String packageNames = parsePackageNames(jsonStr);

        // Update the ANGLE whitelist
        if (packageNames != null) {
            GlobalSettings.updateAngleWhitelist(context, packageNames);
        }
    }

    /*
     * Open the rules file and pull all the JSON into a string
     */
    private String loadRules(Context context)
    {
        String jsonStr = null;

        try {
            InputStream rulesStream = context.getAssets().open(ANGLE_RULES_FILE);
            int size = rulesStream.available();
            byte[] buffer = new byte[size];
            rulesStream.read(buffer);
            rulesStream.close();
            jsonStr = new String(buffer, "UTF-8");

        } catch (IOException ioe) {
            Log.e(TAG, "Failed to open " + ANGLE_RULES_FILE + ": ", ioe);
        }

        return jsonStr;
    }

    /*
     * Extract all app package names from the json file and return them comma separated
     */
    private String parsePackageNames(String rulesJSON)
    {
        String packageNames = "";

        try {
            JSONObject jsonObj = new JSONObject(rulesJSON);
            JSONArray rules = jsonObj.getJSONArray("Rules");
            if (rules == null) {
                Log.e(TAG, "No Rules in " + ANGLE_RULES_FILE);
                return null;
            }
            for (int i = 0; i < rules.length(); i++) {
                JSONObject rule = rules.getJSONObject(i);
                JSONArray apps = rule.optJSONArray("Applications");
                if (apps == null) {
                    Log.v(TAG, "Skipping Rules entry with no Applications");
                    continue;
                }
                for (int j = 0; j < apps.length(); j++) {
                    JSONObject app = apps.optJSONObject(j);
                    String appName = app.optString("AppName");
                    if ((appName == null) || appName.isEmpty()) {
                        Log.e(TAG, "Invalid AppName: '" + appName + "'");
                    }
                    if (packageNames.isEmpty()) {
                        packageNames += appName;
                    } else {
                        packageNames += "," + appName;
                    }
                }
            }
            Log.v(TAG, "Parsed the following package names from " +
                ANGLE_RULES_FILE + ": " + packageNames);
        } catch (JSONException je) {
            Log.e(TAG, "Error when parsing angle JSON: ", je);
            return null;
        }

        return packageNames;
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
