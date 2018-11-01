/*
 * Copyright 2018 The ANGLE Project Authors. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
package com.google.android.angle;

import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.os.Bundle;
import android.os.Process;
import android.support.v14.preference.PreferenceFragment;
import android.support.v14.preference.SwitchPreference;
import android.support.v7.preference.ListPreference;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceCategory;
import android.support.v7.preference.PreferenceManager;
import android.util.Log;

import java.text.Collator;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Map;

public class MainFragment extends PreferenceFragment implements OnSharedPreferenceChangeListener
{

    private final String TAG = this.getClass().getSimpleName();

    private static final String ACTION_REFRESH_PKGS = "com.google.android.angle.REFRESH_PKGS";
    // Global.Settings value to indicate if ANGLE should be used for all PKGs.

    private SharedPreferences mPrefs;
    private GlobalSettings mGlobalSettings;
    private Receiver mRefreshReceiver;
    private SwitchPreference mAllAngleSwitchPref;
    private List<PackageInfo> mInstalledPkgs      = new ArrayList<>();
    private List<ListPreference> mDriverListPrefs = new ArrayList<>();

    SharedPreferences.OnSharedPreferenceChangeListener listener =
            new SharedPreferences.OnSharedPreferenceChangeListener() {
                public void onSharedPreferenceChanged(SharedPreferences prefs, String key)
                {
                    // Nothing to do yet
                }
            };

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        getInstalledPkgsList();

        mPrefs = PreferenceManager.getDefaultSharedPreferences(
                getActivity().getApplicationContext());
        validatePreferences();

        mGlobalSettings = new GlobalSettings(getContext(), mInstalledPkgs);
        mergeGlobalSettings();

        mRefreshReceiver = new Receiver() {
            @Override
            public void onReceive(Context context, Intent intent)
            {
                getInstalledPkgsList();
            }
        };

        String allUseAngleKey = getContext().getString(R.string.pref_key_all_angle);
        Boolean allUseAngle   = mPrefs.getBoolean(allUseAngleKey, false);
        mAllAngleSwitchPref   = (SwitchPreference) findPreference(allUseAngleKey);
        mAllAngleSwitchPref.setChecked(mGlobalSettings.getAllUseAngle());
        mAllAngleSwitchPref.setOnPreferenceClickListener(
                new Preference.OnPreferenceClickListener() {
                    @Override
                    public boolean onPreferenceClick(Preference preference)
                    {
                        Receiver.updateAllUseAngle(getContext());
                        return true;
                    }
                });

        String selectDriverCatKey =
                getContext().getString(R.string.pref_key_select_opengl_driver_category);
        PreferenceCategory installedPkgsCat =
                (PreferenceCategory) findPreference(selectDriverCatKey);
        getInstalledPkgsList();
        mDriverListPrefs.clear();
        for (PackageInfo packageInfo : mInstalledPkgs)
        {
            ListPreference listPreference = new ListPreference(getActivity());

            initListPreference(packageInfo, listPreference);

            installedPkgsCat.addPreference(listPreference);
        }
    }

    @Override
    public void onResume()
    {
        super.onResume();

        getActivity().registerReceiver(mRefreshReceiver, new IntentFilter(ACTION_REFRESH_PKGS));
        getPreferenceScreen().getSharedPreferences().registerOnSharedPreferenceChangeListener(
                listener);
    }

    @Override
    public void onPause()
    {
        getActivity().unregisterReceiver(mRefreshReceiver);
        getPreferenceScreen().getSharedPreferences().unregisterOnSharedPreferenceChangeListener(
                listener);

        super.onPause();
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey)
    {
        addPreferencesFromResource(R.xml.main);
    }

    @Override
    public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key)
    {
        Log.v(TAG, "Shared preference changed: key = '" + key + "'");
    }

    private void validatePreferences()
    {
        Map<String, ?> allPrefs = mPrefs.getAll();

        // Remove Preference values for any uninstalled PKGs
        for (String key : allPrefs.keySet())
        {
            // Remove any uninstalled PKGs
            PackageInfo packageInfo = getPackageInfoForPackageName(key);

            if (packageInfo != null)
            {
                removePkgPreference(key);
            }
        }
    }

    private void getInstalledPkgsList()
    {
        List<PackageInfo> pkgs = getActivity().getPackageManager().getInstalledPackages(0);

        mInstalledPkgs.clear();

        for (PackageInfo packageInfo : pkgs)
        {
            if (packageInfo.applicationInfo.uid == Process.SYSTEM_UID)
            {
                continue;
            }

            // Filter out apps that are system apps
            if ((packageInfo.applicationInfo.flags & ApplicationInfo.FLAG_SYSTEM) != 0)
            {
                continue;
            }

            mInstalledPkgs.add(packageInfo);
        }

        Collections.sort(mInstalledPkgs, displayNameComparator);
    }

    private final Comparator<PackageInfo> displayNameComparator = new Comparator<PackageInfo>() {
        public final int compare(PackageInfo a, PackageInfo b)
        {
            return collator.compare(getAppName(a), getAppName(b));
        }

        private final Collator collator = Collator.getInstance();
    };

    private String getAppName(PackageInfo packageInfo)
    {
        return packageInfo.applicationInfo.loadLabel(getActivity().getPackageManager()).toString();
    }

    private void initListPreference(PackageInfo packageInfo, ListPreference listPreference)
    {
        CharSequence[] drivers = getResources().getStringArray(R.array.driver_values);
        listPreference.setEntryValues(drivers);
        listPreference.setEntries(drivers);

        String defaultDriver = getContext().getString(R.string.default_driver);
        listPreference.setDefaultValue(defaultDriver);

        String dialogTitleKey = getContext().getString(R.string.select_opengl_driver_title);
        listPreference.setDialogTitle(dialogTitleKey);
        listPreference.setKey(packageInfo.packageName);

        listPreference.setTitle(getAppName(packageInfo));
        listPreference.setSummary(mPrefs.getString(packageInfo.packageName, defaultDriver));

        listPreference.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener() {
            @Override
            public boolean onPreferenceChange(Preference preference, Object newValue)
            {
                ListPreference listPreference = (ListPreference) preference;

                listPreference.setSummary(newValue.toString());
                mGlobalSettings.updatePkg(preference.getKey(), newValue.toString());

                return true;
            }
        });

        mDriverListPrefs.add(listPreference);
    }

    private void removePkgPreference(String key)
    {
        SharedPreferences.Editor editor = mPrefs.edit();

        editor.remove(key);
        editor.apply();

        for (ListPreference listPreference : mDriverListPrefs)
        {
            if (listPreference.getKey().equals(key))
            {
                mDriverListPrefs.remove(listPreference);
            }
        }
    }

    private PackageInfo getPackageInfoForPackageName(String pkgName)
    {
        PackageInfo foundPackageInfo = null;

        for (PackageInfo packageInfo : mInstalledPkgs)
        {
            if (pkgName.equals(getAppName(packageInfo)))
            {
                foundPackageInfo = packageInfo;
                break;
            }
        }

        return foundPackageInfo;
    }

    private void mergeGlobalSettings()
    {
        SharedPreferences.Editor editor = mPrefs.edit();

        for (PackageInfo packageInfo : mInstalledPkgs)
        {
            String driver = mGlobalSettings.getDriverForPkg(packageInfo.packageName);

            if (driver != null)
            {
                editor.putString(packageInfo.packageName, driver);
            }
        }

        editor.apply();
    }
}
