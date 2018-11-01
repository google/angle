/*
 * Copyright 2018 The ANGLE Project Authors. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
package com.google.android.angle;

import android.content.ContentResolver;
import android.content.Context;
import android.content.pm.PackageInfo;
import android.provider.Settings;
import android.util.Log;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

class GlobalSettings
{

    private final String TAG = this.getClass().getSimpleName();

    private Context mContext;
    private List<PackageInfo> mInstalledPkgs         = new ArrayList<>();
    private List<String> mGlobalSettingsDriverPkgs   = new ArrayList<>();
    private List<String> mGlobalSettingsDriverValues = new ArrayList<>();

    GlobalSettings(Context context, List<PackageInfo> installedPkgs)
    {
        mContext = context;

        setInstalledPkgs(installedPkgs);
    }

    Boolean getAllUseAngle()
    {
        ContentResolver contentResolver = mContext.getContentResolver();
        try
        {
            int allUseAngle = Settings.Global.getInt(
                    contentResolver, mContext.getString(R.string.global_settings_driver_all_angle));
            return (allUseAngle == 1);
        }
        catch (Settings.SettingNotFoundException e)
        {
            return false;
        }
    }

    static void updateAllUseAngle(Context context, Boolean allUseAngle)
    {
        ContentResolver contentResolver = context.getContentResolver();
        Settings.Global.putInt(contentResolver,
                context.getString(R.string.global_settings_driver_all_angle), allUseAngle ? 1 : 0);
    }

    void updatePkg(String pkgName, String driver)
    {
        int pkgIndex = getGlobalSettingsPkgIndex(pkgName);

        if (!isValidDiverValue(driver))
        {
            Log.e(TAG, "Attempting to update a PKG with an invalid driver: '" + driver + "'");
            return;
        }

        String defaultDriver = mContext.getString(R.string.default_driver);
        if (driver.equals(defaultDriver))
        {
            if (pkgIndex >= 0)
            {
                // We only store global settings for driver values other than the default
                mGlobalSettingsDriverPkgs.remove(pkgIndex);
                mGlobalSettingsDriverValues.remove(pkgIndex);
            }
        }
        else
        {
            if (pkgIndex >= 0)
            {
                mGlobalSettingsDriverValues.set(pkgIndex, driver);
            }
            else
            {
                mGlobalSettingsDriverPkgs.add(pkgName);
                mGlobalSettingsDriverValues.add(driver);
            }
        }

        writeGlobalSettings();
    }

    String getDriverForPkg(String pkgName)
    {
        int pkgIndex = getGlobalSettingsPkgIndex(pkgName);

        if (pkgIndex >= 0)
        {
            return mGlobalSettingsDriverValues.get(pkgIndex);
        }

        return null;
    }

    private void setInstalledPkgs(List<PackageInfo> installedPkgs)
    {
        mInstalledPkgs = installedPkgs;

        updateGlobalSettings();
    }

    private void updateGlobalSettings()
    {
        readGlobalSettings();

        validateGlobalSettings();

        writeGlobalSettings();
    }

    private void readGlobalSettings()
    {
        mGlobalSettingsDriverPkgs = getGlobalSettingsString(
                mContext.getString(R.string.global_settings_driver_selection_pkgs));
        mGlobalSettingsDriverValues = getGlobalSettingsString(
                mContext.getString(R.string.global_settings_driver_selection_values));
    }

    private List<String> getGlobalSettingsString(String globalSetting)
    {
        List<String> valueList;
        ContentResolver contentResolver = mContext.getContentResolver();
        String settingsValue            = Settings.Global.getString(contentResolver, globalSetting);

        if (settingsValue != null)
        {
            valueList = new ArrayList<>(Arrays.asList(settingsValue.split(",")));
        }
        else
        {
            valueList = new ArrayList<>();
        }

        return valueList;
    }

    private void writeGlobalSettings()
    {
        String driverPkgs   = String.join(",", mGlobalSettingsDriverPkgs);
        String driverValues = String.join(",", mGlobalSettingsDriverValues);

        ContentResolver contentResolver = mContext.getContentResolver();
        Settings.Global.putString(contentResolver,
                mContext.getString(R.string.global_settings_driver_selection_pkgs), driverPkgs);
        Settings.Global.putString(contentResolver,
                mContext.getString(R.string.global_settings_driver_selection_values), driverValues);
    }

    private void validateGlobalSettings()
    {
        // Verify lengths
        if (mGlobalSettingsDriverPkgs.size() != mGlobalSettingsDriverValues.size())
        {
            // The lengths don't match, so clear the values out and rebuild later.
            mGlobalSettingsDriverPkgs.clear();
            mGlobalSettingsDriverValues.clear();
            return;
        }

        String defaultDriver = mContext.getString(R.string.default_driver);
        // Use a temp list, since we're potentially modifying the original list.
        List<String> globalSettingsDriverPkgs = new ArrayList<>(mGlobalSettingsDriverPkgs);
        for (String pkgName : globalSettingsDriverPkgs)
        {
            // Remove any uninstalled packages.
            if (!isPkgInstalled(pkgName))
            {
                removePkgFromGlobalSettings(pkgName);
            }
            // Remove any packages with invalid driver values
            else if (!isValidDiverValue(getDriverForPkg(pkgName)))
            {

                removePkgFromGlobalSettings(pkgName);
            }
            // Remove any packages with the 'default' driver selected
            else if (defaultDriver.equals(getDriverForPkg(pkgName)))
            {
                removePkgFromGlobalSettings(pkgName);
            }
        }
    }

    private void removePkgFromGlobalSettings(String pkgName)
    {
        int pkgIndex = getGlobalSettingsPkgIndex(pkgName);

        mGlobalSettingsDriverPkgs.remove(pkgIndex);
        mGlobalSettingsDriverValues.remove(pkgIndex);
    }

    private int getGlobalSettingsPkgIndex(String pkgName)
    {
        for (int pkgIndex = 0; pkgIndex < mGlobalSettingsDriverPkgs.size(); pkgIndex++)
        {
            if (mGlobalSettingsDriverPkgs.get(pkgIndex).equals(pkgName))
            {
                return pkgIndex;
            }
        }

        return -1;
    }

    private Boolean isPkgInstalled(String pkgName)
    {
        for (PackageInfo pkg : mInstalledPkgs)
        {
            if (pkg.packageName.equals(pkgName))
            {
                return true;
            }
        }

        return false;
    }

    private Boolean isValidDiverValue(String driverValue)
    {
        CharSequence[] drivers = mContext.getResources().getStringArray(R.array.driver_values);

        for (CharSequence driver : drivers)
        {
            if (driverValue.equals(driver.toString()))
            {
                return true;
            }
        }

        return false;
    }
}
