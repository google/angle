//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// feature_support_util.cpp: Helps Android EGL loader to determine whether to use ANGLE or a native
// GLES driver.  Helps ANGLE know which work-arounds to use.

#include "feature_support_util.h"
#include <json/json.h>
#include <string.h>
#include <unistd.h>
#include <fstream>
#include <list>

// namespace angle_for_android
//{

#if defined(ANDROID)
#    include <android/log.h>

// Define ANGLE_FEATURE_UTIL_LOG_VERBOSE if you want ALOGV to output
// ANGLE_FEATURE_UTIL_LOG_VERBOSE is automatically defined when is_debug = true

#    define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, "ANGLE", __VA_ARGS__)
#    define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, "ANGLE", __VA_ARGS__)
#    define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, "ANGLE", __VA_ARGS__)
#    define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "ANGLE", __VA_ARGS__)
#    ifdef ANGLE_FEATURE_UTIL_LOG_VERBOSE
#        define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, "ANGLE", __VA_ARGS__)
#    else
#        define ALOGV(...) ((void)0)
#    endif
#else  // defined(ANDROID)
#    define ALOGE(...) printf(__VA_ARGS__);
#    define ALOGW(...) printf(__VA_ARGS__);
#    define ALOGI(...) printf(__VA_ARGS__);
#    define ALOGD(...) printf(__VA_ARGS__);
#    define ALOGV(...) printf(__VA_ARGS__);
#endif  // defined(ANDROID)

// JSON values are generally composed of either:
//  - Objects, which are a set of comma-separated string:value pairs (note the recursive nature)
//  - Arrays, which are a set of comma-separated values.
// We'll call the string in a string:value pair the "identifier".  These identifiers are defined
// below, as follows:

// The JSON identifier for the top-level set of rules.  This is an object, the value of which is an
// array of rules.  The rules will be processed in order.  For any given type of answer, if a rule
// matches, the rule's version of the answer (true or false) becomes the new answer.  After all
// rules are processed, the most-recent answer is the final answer.
constexpr char kJsonRules[] = "Rules";
// The JSON identifier for a given rule.  A rule is an object, the first string:value pair is this
// identifier (i.e. "Rule") as the string and a user-firendly description of the rule:
constexpr char kJsonRule[] = "Rule";
// Within a rule, the JSON identifier for one type of answer--whether to allow an application to
// specify whether to use ANGLE.  The value is a boolean (i.e. true or false), with true allowing
// the application to specify whether or not to use ANGLE.
constexpr char kJsonAppChoice[] = "AppChoice";
// Within a rule, the JSON identifier for one type of answer--whether or not to use ANGLE when an
// application doesn't specify (or isn't allowed to specify) whether or not to use ANGLE.  The
// value is a boolean (i.e. true or false).
constexpr char kJsonNonChoice[] = "NonChoice";

// Within a rule, the JSON identifier for describing one or more applications.  The value is an
// array of objects, each object of which can specify attributes of an application.
constexpr char kJsonApplications[] = "Applications";
// Within an object that describes the attributes of an application, the JSON identifier for the
// name of the application (e.g. "com.google.maps").  The value is a string.  If any other
// attributes will be specified, this must be the first attribute specified in the object.
constexpr char kJsonAppName[] = "AppName";
// Within an object that describes the attributes of an application, the JSON identifier for the
// intent of the application to run.  The value is a string.
constexpr char kJsonIntent[] = "Intent";

// Within a rule, the JSON identifier for describing one or more devices.  The value is an
// array of objects, each object of which can specify attributes of a device.
constexpr char kJsonDevices[] = "Devices";
// Within an object that describes the attributes of a device, the JSON identifier for the
// manufacturer of the device.  The value is a string.  If any other attributes will be specified,
// this must be the first attribute specified in the object.
constexpr char kJsonManufacturer[] = "Manufacturer";
// Within an object that describes the attributes of a device, the JSON identifier for the
// model of the device.  The value is a string.
constexpr char kJsonModel[] = "Model";

// Within an object that describes the attributes of a device, the JSON identifier for describing
// one or more GPUs/drivers used in the device.  The value is an
// array of objects, each object of which can specify attributes of a GPU and its driver.
constexpr char kJsonGPUs[] = "GPUs";
// Within an object that describes the attributes of a GPU and driver, the JSON identifier for the
// vendor of the device/driver.  The value is a string.  If any other attributes will be specified,
// this must be the first attribute specified in the object.
constexpr char kJsonvendor[] = "vendor";
// Within an object that describes the attributes of a GPU and driver, the JSON identifier for the
// deviceId of the device.  The value is an unsigned integer.  If the driver version will be
// specified, this must preceded the version attributes specified in the object.
constexpr char kJsondeviceId[] = "deviceId";

// Within an object that describes the attributes of either an application or a GPU, the JSON
// identifier for the major version of that application or GPU driver.  The value is a positive
// integer number.  Not specifying a major version implies a wildcard for all values of a version.
constexpr char kJsonVerMajor[] = "VerMajor";
// Within an object that describes the attributes of either an application or a GPU, the JSON
// identifier for the minor version of that application or GPU driver.  The value is a positive
// integer number.  In order to specify a minor version, it must be specified immediately after the
// major number associated with it.  Not specifying a minor version implies a wildcard for the
// minor, subminor, and patch values of a version.
constexpr char kJsonVerMinor[] = "VerMinor";
// Within an object that describes the attributes of either an application or a GPU, the JSON
// identifier for the subminor version of that application or GPU driver.  The value is a positive
// integer number.  In order to specify a subminor version, it must be specified immediately after
// the minor number associated with it.  Not specifying a subminor version implies a wildcard for
// the subminor and patch values of a version.
constexpr char kJsonVerSubMinor[] = "VerSubMinor";
// Within an object that describes the attributes of either an application or a GPU, the JSON
// identifier for the patch version of that application or GPU driver.  The value is a positive
// integer number.  In order to specify a patch version, it must be specified immediately after the
// subminor number associated with it.  Not specifying a patch version implies a wildcard for the
// patch value of a version.
constexpr char kJsonVerPatch[] = "VerPatch";

// This encapsulates a std::string.  The default constructor (not given a string) assumes that this
// is a wildcard (i.e. will match all other StringPart objects).
class StringPart
{
  public:
    StringPart() : mPart(""), mWildcard(true) {}
    StringPart(std::string part) : mPart(part), mWildcard(false) {}
    ~StringPart() {}
    bool match(StringPart &toCheck)
    {
        return (mWildcard || toCheck.mWildcard || (toCheck.mPart == mPart));
    }

  public:
    std::string mPart;
    bool mWildcard;
};

// This encapsulates a 32-bit unsigned integer.  The default constructor (not given a number)
// assumes that this is a wildcard (i.e. will match all other IntegerPart objects).
class IntegerPart
{
  public:
    IntegerPart() : mPart(0), mWildcard(true) {}
    IntegerPart(uint32_t part) : mPart(part), mWildcard(false) {}
    ~IntegerPart() {}
    bool match(IntegerPart &toCheck)
    {
        return (mWildcard || toCheck.mWildcard || (toCheck.mPart == mPart));
    }

  public:
    uint32_t mPart;
    bool mWildcard;
};

// This encapsulates a list of other classes, which of which will have a match() method.  The
// common constructor (given a type, but not any list items) assumes that this is a wildcard
// (i.e. will match all other ListOf<t> objects).
template <class T>
class ListOf
{
  public:
    ListOf(std::string listType) : mListType(listType), mWildcard(true) {}
    ~ListOf() {}
    void addItem(T &toAdd)
    {
        mList.push_back(toAdd);
        mWildcard = false;
    }
    bool match(T &toCheck)
    {
        ALOGV("\t\t Within ListOf<%s> match: wildcards are %s and %s,\n", mListType.c_str(),
              mWildcard ? "true" : "false", toCheck.mWildcard ? "true" : "false");
        if (mWildcard || toCheck.mWildcard)
        {
            return true;
        }
        for (auto &it : mList)
        {
            ALOGV("\t\t   Within ListOf<%s> match: calling match on sub-item is %s,\n",
                  mListType.c_str(), it.match(toCheck) ? "true" : "false");
            if (it.match(toCheck))
            {
                return true;
            }
        }
        return false;
    }
    T &front() { return (mList.front()); }
    void logListOf(std::string prefix, std::string name)
    {
        if (mWildcard)
        {
            ALOGV("%sListOf%s is wildcarded to always match", prefix.c_str(), name.c_str());
        }
        else
        {
            ALOGV("%sListOf%s is has %d item(s):", prefix.c_str(), name.c_str(),
                  static_cast<int>(mList.size()));
            for (auto &it : mList)
            {
                it.logItem();
            }
        }
    }

  private:
    std::string mListType;
    std::list<T> mList;

  public:
    bool mWildcard = true;
};

// This encapsulates up-to four 32-bit unsigned integers, that represent a potentially-complex
// version number.  The default constructor (not given any numbers) assumes that this is a wildcard
// (i.e. will match all other Version objects).  Each part of a Version is stored in an IntegerPart
// class, and so may be wildcarded as well.
class Version
{
  public:
    Version(uint32_t major, uint32_t minor, uint32_t subminor, uint32_t patch)
        : mMajor(major), mMinor(minor), mSubminor(subminor), mPatch(patch), mWildcard(false)
    {}
    Version(uint32_t major, uint32_t minor, uint32_t subminor)
        : mMajor(major), mMinor(minor), mSubminor(subminor), mWildcard(false)
    {}
    Version(uint32_t major, uint32_t minor) : mMajor(major), mMinor(minor), mWildcard(false) {}
    Version(uint32_t major) : mMajor(major), mWildcard(false) {}
    Version() : mWildcard(true) {}
    Version(const Version &toCopy)
        : mMajor(toCopy.mMajor),
          mMinor(toCopy.mMinor),
          mSubminor(toCopy.mSubminor),
          mPatch(toCopy.mPatch),
          mWildcard(toCopy.mWildcard)
    {}
    ~Version() {}
    bool match(Version &toCheck)
    {
        ALOGV("\t\t\t Within Version %d,%d,%d,%d match(%d,%d,%d,%d): wildcards are %s and %s,\n",
              mMajor.mPart, mMinor.mPart, mSubminor.mPart, mPatch.mPart, toCheck.mMajor.mPart,
              toCheck.mMinor.mPart, toCheck.mSubminor.mPart, toCheck.mPatch.mPart,
              mWildcard ? "true" : "false", toCheck.mWildcard ? "true" : "false");
        if (!(mWildcard || toCheck.mWildcard))
        {
            ALOGV("\t\t\t   mMajor match is %s, mMinor is %s, mSubminor is %s, mPatch is %s\n",
                  mMajor.match(toCheck.mMajor) ? "true" : "false",
                  mMinor.match(toCheck.mMinor) ? "true" : "false",
                  mSubminor.match(toCheck.mSubminor) ? "true" : "false",
                  mPatch.match(toCheck.mPatch) ? "true" : "false");
        }
        return (mWildcard || toCheck.mWildcard ||
                (mMajor.match(toCheck.mMajor) && mMinor.match(toCheck.mMinor) &&
                 mSubminor.match(toCheck.mSubminor) && mPatch.match(toCheck.mPatch)));
    }
    static Version *CreateVersionFromJson(Json::Value &jObject)
    {
        Version *version = nullptr;
        // A major version must be provided before a minor, and so on:
        if (jObject.isMember(kJsonVerMajor) && jObject[kJsonVerMajor].isInt())
        {
            int major = jObject[kJsonVerMajor].asInt();
            if (jObject.isMember(kJsonVerMinor) && jObject[kJsonVerMinor].isInt())
            {
                int minor = jObject[kJsonVerMinor].asInt();
                if (jObject.isMember(kJsonVerSubMinor) && jObject[kJsonVerSubMinor].isInt())
                {
                    int subMinor = jObject[kJsonVerSubMinor].asInt();
                    if (jObject.isMember(kJsonVerPatch) && jObject[kJsonVerPatch].isInt())
                    {
                        int patch = jObject[kJsonVerPatch].asInt();
                        version   = new Version(major, minor, subMinor, patch);
                    }
                    else
                    {
                        version = new Version(major, minor, subMinor);
                    }
                }
                else
                {
                    version = new Version(major, minor);
                }
            }
            else
            {
                version = new Version(major);
            }
        }
        // TODO (ianelliott@) (b/113346561) appropriately destruct lists and
        // other items that get created from json parsing
        return version;
    }
    std::string getString()
    {
        if (mWildcard)
        {
            return "*";
        }
        else
        {
            char ret[100];
            // Must at least have a major version:
            if (!mMinor.mWildcard)
            {
                if (!mSubminor.mWildcard)
                {
                    if (!mPatch.mWildcard)
                    {
                        snprintf(ret, 100, "%d.%d.%d.%d", mMajor.mPart, mMinor.mPart,
                                 mSubminor.mPart, mPatch.mPart);
                    }
                    else
                    {
                        snprintf(ret, 100, "%d.%d.%d.*", mMajor.mPart, mMinor.mPart,
                                 mSubminor.mPart);
                    }
                }
                else
                {
                    snprintf(ret, 100, "%d.%d.*", mMajor.mPart, mMinor.mPart);
                }
            }
            else
            {
                snprintf(ret, 100, "%d.*", mMajor.mPart);
            }
            std::string retString = ret;
            return retString;
        }
    }

  public:
    IntegerPart mMajor;
    IntegerPart mMinor;
    IntegerPart mSubminor;
    IntegerPart mPatch;
    bool mWildcard;
};

// This encapsulates an application, and potentially the application's Version and/or the intent
// that it is launched with.  The default constructor (not given any values) assumes that this is a
// wildcard (i.e. will match all other Application objects).  Each part of an Application is stored
// in a class that may also be wildcarded.
class Application
{
  public:
    Application(std::string name, Version &version, std::string intent)
        : mName(name), mVersion(version), mIntent(intent), mWildcard(false)
    {}
    Application(std::string name, std::string intent)
        : mName(name), mVersion(), mIntent(intent), mWildcard(false)
    {}
    Application(std::string name, Version &version)
        : mName(name), mVersion(version), mIntent(), mWildcard(false)
    {}
    Application(std::string name) : mName(name), mVersion(), mIntent(), mWildcard(false) {}
    Application() : mName(), mVersion(), mIntent(), mWildcard(true) {}
    ~Application() {}
    bool match(Application &toCheck)
    {
        return (mWildcard || toCheck.mWildcard ||
                (toCheck.mName.match(mName) && toCheck.mVersion.match(mVersion) &&
                 toCheck.mIntent.match(mIntent)));
    }
    static Application *CreateApplicationFromJson(Json::Value &jObject)
    {
        Application *application = nullptr;

        // If an application is listed, the application's name is required:
        std::string appName = jObject[kJsonAppName].asString();

        // The application's version and intent are optional:
        Version *version = Version::CreateVersionFromJson(jObject);
        if (version)
        {
            if (jObject.isMember(kJsonIntent) && jObject[kJsonIntent].isString())
            {
                application = new Application(appName, *version, jObject[kJsonIntent].asString());
            }
            else
            {
                application = new Application(appName, *version);
            }
        }
        else
        {
            if (jObject.isMember(kJsonIntent) && jObject[kJsonIntent].isString())
            {
                application = new Application(appName, jObject[kJsonIntent].asString());
            }
            else
            {
                application = new Application(appName);
            }
        }
        // TODO (ianelliott@) (b/113346561) appropriately destruct lists and
        // other items that get created from json parsing
        return application;
    }
    void logItem()
    {
        if (mWildcard)
        {
            ALOGV("      Wildcard (i.e. will match all applications)");
        }
        else if (!mVersion.mWildcard)
        {
            if (!mIntent.mWildcard)
            {
                ALOGV("      Application \"%s\" (version: %s; intent: \"%s\")", mName.mPart.c_str(),
                      mVersion.getString().c_str(), mIntent.mPart.c_str());
            }
            else
            {
                ALOGV("      Application \"%s\" (version: %s)", mName.mPart.c_str(),
                      mVersion.getString().c_str());
            }
        }
        else if (!mIntent.mWildcard)
        {
            ALOGV("      Application \"%s\" (intent: \"%s\")", mName.mPart.c_str(),
                  mIntent.mPart.c_str());
        }
        else
        {
            ALOGV("      Application \"%s\"", mName.mPart.c_str());
        }
    }

  public:
    StringPart mName;
    Version mVersion;
    StringPart mIntent;
    bool mWildcard;
};

// This encapsulates a GPU and its driver.  The default constructor (not given any values) assumes
// that this is a wildcard (i.e. will match all other GPU objects).  Each part of a GPU is stored
// in a class that may also be wildcarded.
class GPU
{
  public:
    GPU(std::string vendor, uint32_t deviceId, Version &version)
        : mVendor(vendor), mDeviceId(IntegerPart(deviceId)), mVersion(version), mWildcard(false)
    {}
    GPU(uint32_t deviceId, Version &version)
        : mVendor(), mDeviceId(IntegerPart(deviceId)), mVersion(version), mWildcard(false)
    {}
    GPU(std::string vendor, uint32_t deviceId)
        : mVendor(vendor), mDeviceId(IntegerPart(deviceId)), mVersion(), mWildcard(false)
    {}
    GPU(std::string vendor) : mVendor(vendor), mDeviceId(), mVersion(), mWildcard(false) {}
    GPU(uint32_t deviceId)
        : mVendor(), mDeviceId(IntegerPart(deviceId)), mVersion(), mWildcard(false)
    {}
    GPU() : mVendor(), mDeviceId(), mVersion(), mWildcard(true) {}
    bool match(GPU &toCheck)
    {
        ALOGV("\t\t Within GPU match: wildcards are %s and %s,\n", mWildcard ? "true" : "false",
              toCheck.mWildcard ? "true" : "false");
        ALOGV("\t\t   mVendor = \"%s\" and toCheck.mVendor = \"%s\"\n", mVendor.mPart.c_str(),
              toCheck.mVendor.mPart.c_str());
        ALOGV("\t\t   mDeviceId = %d and toCheck.mDeviceId = %d\n", mDeviceId.mPart,
              toCheck.mDeviceId.mPart);
        ALOGV("\t\t   mVendor match is %s, mDeviceId is %s, mVersion is %s\n",
              toCheck.mVendor.match(mVendor) ? "true" : "false",
              toCheck.mDeviceId.match(mDeviceId) ? "true" : "false",
              toCheck.mVersion.match(mVersion) ? "true" : "false");
        return (mWildcard || toCheck.mWildcard ||
                (toCheck.mVendor.match(mVendor) && toCheck.mDeviceId.match(mDeviceId) &&
                 toCheck.mVersion.match(mVersion)));
    }
    ~GPU() {}
    static GPU *CreateGpuFromJson(Json::Value &jObject)
    {
        GPU *gpu = nullptr;

        // If a GPU is listed, the vendor name is required:
        if (jObject.isMember(kJsonvendor) && jObject[kJsonvendor].isString())
        {
            std::string vendor = jObject[kJsonvendor].asString();
            // If a version is given, the deviceId is required:
            if (jObject.isMember(kJsondeviceId) && jObject[kJsondeviceId].isUInt())
            {
                uint32_t deviceId = jObject[kJsondeviceId].asUInt();
                Version *version  = Version::CreateVersionFromJson(jObject);
                if (version)
                {
                    gpu = new GPU(vendor, deviceId, *version);
                }
                else
                {
                    gpu = new GPU(vendor, deviceId);
                }
            }
            else
            {
                gpu = new GPU(vendor);
            }
        }
        else
        {
            ALOGW("Asked to parse a GPU, but no GPU found");
        }

        // TODO (ianelliott@) (b/113346561) appropriately destruct lists and
        // other items that get created from json parsing
        return gpu;
    }
    void logItem()
    {
        if (mWildcard)
        {
            ALOGV("          Wildcard (i.e. will match all GPUs)");
        }
        else
        {
            if (!mDeviceId.mWildcard)
            {
                if (!mVersion.mWildcard)
                {
                    ALOGV("\t     GPU vendor: %s, deviceId: 0x%x, version: %s",
                          mVendor.mPart.c_str(), mDeviceId.mPart, mVersion.getString().c_str());
                }
                else
                {
                    ALOGV("\t     GPU vendor: %s, deviceId: 0x%x", mVendor.mPart.c_str(),
                          mDeviceId.mPart);
                }
            }
            else
            {
                ALOGV("\t     GPU vendor: %s", mVendor.mPart.c_str());
            }
        }
    }

  public:
    StringPart mVendor;
    IntegerPart mDeviceId;
    Version mVersion;
    bool mWildcard;
};

// This encapsulates a device, and potentially the device's model and/or a list of GPUs/drivers
// associated with the Device.  The default constructor (not given any values) assumes that this is
// a wildcard (i.e. will match all other Device objects).  Each part of a Device is stored in a
// class that may also be wildcarded.
class Device
{
  public:
    Device(std::string manufacturer, std::string model)
        : mManufacturer(manufacturer), mModel(model), mGpuList("GPU"), mWildcard(false)
    {}
    Device(std::string manufacturer)
        : mManufacturer(manufacturer), mModel(), mGpuList("GPU"), mWildcard(false)
    {}
    Device() : mManufacturer(), mModel(), mGpuList("GPU"), mWildcard(true) {}
    ~Device() {}
    void addGPU(GPU &gpu) { mGpuList.addItem(gpu); }
    bool match(Device &toCheck)
    {
        ALOGV("\t Within Device match: wildcards are %s and %s,\n", mWildcard ? "true" : "false",
              toCheck.mWildcard ? "true" : "false");
        if (!(mWildcard || toCheck.mWildcard))
        {
            ALOGV("\t   Manufacturer match is %s, model is %s\n",
                  toCheck.mManufacturer.match(mManufacturer) ? "true" : "false",
                  toCheck.mModel.match(mModel) ? "true" : "false");
        }
        ALOGV("\t   Need to check ListOf<GPU>\n");
        return ((mWildcard || toCheck.mWildcard ||
                 // The wildcards can override the Manufacturer/Model check, but not the GPU check
                 (toCheck.mManufacturer.match(mManufacturer) && toCheck.mModel.match(mModel))) &&
                // Note: toCheck.mGpuList is for the device and must contain exactly one item,
                // where mGpuList may contain zero or more items:
                mGpuList.match(toCheck.mGpuList.front()));
    }
    static Device *CreateDeviceFromJson(Json::Value &jObject)
    {
        Device *device = nullptr;
        if (jObject.isMember(kJsonManufacturer) && jObject[kJsonManufacturer].isString())
        {
            std::string manufacturerName = jObject[kJsonManufacturer].asString();
            // We don't let a model be specified without also specifying an Manufacturer:
            if (jObject.isMember(kJsonModel) && jObject[kJsonModel].isString())
            {
                std::string model = jObject[kJsonModel].asString();
                device            = new Device(manufacturerName, model);
            }
            else
            {
                device = new Device(manufacturerName);
            }
        }
        else
        {
            // This case is not treated as an error because a rule may wish to only call out one or
            // more GPUs, and not any specific Manufacturer devices:
            device = new Device();
        }
        // TODO (ianelliott@) (b/113346561) appropriately destruct lists and
        // other items that get created from json parsing
        return device;
    }
    void logItem()
    {
        if (mWildcard)
        {
            if (mGpuList.mWildcard)
            {
                ALOGV("      Wildcard (i.e. will match all devices)");
                return;
            }
            else
            {
                ALOGV(
                    "      Device with any manufacturer and model"
                    ", and with the following GPUs:");
            }
        }
        else
        {
            if (!mModel.mWildcard)
            {
                ALOGV(
                    "      Device manufacturer: \"%s\" and model \"%s\""
                    ", and with the following GPUs:",
                    mManufacturer.mPart.c_str(), mModel.mPart.c_str());
            }
            else
            {
                ALOGV(
                    "      Device manufacturer: \"%s\""
                    ", and with the following GPUs:",
                    mManufacturer.mPart.c_str());
            }
        }
        mGpuList.logListOf("        ", "GPUs");
    }

  public:
    StringPart mManufacturer;
    StringPart mModel;
    ListOf<GPU> mGpuList;
    bool mWildcard;
};

// This encapsulates a particular scenario to check against the rules.  A Scenario is similar to a
// Rule, except that a Rule has answers and potentially many wildcards, and a Scenario is the
// fully-specified combination of an Application and a Device that is proposed to be run with
// ANGLE.  It is compared with the list of Rules.
class Scenario
{
  public:
    Scenario(const char *appName, const char *deviceMfr, const char *deviceModel)
        : mApplication(Application(appName)), mDevice(Device(deviceMfr, deviceModel))
    {}
    ~Scenario() {}
    void logScenario()
    {
        ALOGV("  Scenario to compare against the rules");
        ALOGV("    Application:");
        mApplication.logItem();
        ALOGV("    Device:");
        mDevice.logItem();
    }

  public:
    Application mApplication;
    Device mDevice;

  private:
    Scenario(Application &app, Device &dev) : mApplication(app), mDevice(dev) {}
    Scenario() : mApplication(), mDevice() {}
};

// This encapsulates a Rule that provides answers based on whether a particular Scenario matches
// the Rule.  A Rule always has answers, but can potentially wildcard every item in it (i.e. match
// every scenario).
class Rule
{
  public:
    Rule(std::string description, bool appChoice, bool answer)
        : mDescription(description),
          mAppList("Application"),
          mDevList("Device"),
          mAppChoice(appChoice),
          mAnswer(answer)
    {}
    ~Rule() {}
    void addApp(Application &app) { mAppList.addItem(app); }
    void addDev(Device &dev) { mDevList.addItem(dev); }
    bool match(Scenario &toCheck)
    {
        ALOGV("    Within \"%s\" Rule: application match is %s and device match is %s\n",
              mDescription.c_str(), mAppList.match(toCheck.mApplication) ? "true" : "false",
              mDevList.match(toCheck.mDevice) ? "true" : "false");
        return (mAppList.match(toCheck.mApplication) && mDevList.match(toCheck.mDevice));
    }
    bool getAppChoice() { return mAppChoice; }
    bool getAnswer() { return mAnswer; }
    void logRule()
    {
        ALOGV("  Rule: \"%s\" %s ANGLE, and %s the app a choice if matched", mDescription.c_str(),
              mAnswer ? "enables" : "disables", mAppChoice ? "does give" : "does NOT give");
        mAppList.logListOf("    ", "Applications");
        mDevList.logListOf("    ", "Devices");
    }

  public:
    std::string mDescription;
    ListOf<Application> mAppList;
    ListOf<Device> mDevList;
    bool mAppChoice;
    bool mAnswer;

  private:
    Rule()
        : mDescription(),
          mAppList("Application"),
          mDevList("Device"),
          mAppChoice(false),
          mAnswer(false)
    {}
};

// This encapsulates a list of Rules that Scenarios are matched against.  A Scenario is compared
// with each Rule, in order.  Any time a Scenario matches a Rule, the current answer is overridden
// with the answer of the matched Rule.
class RuleList
{
  public:
    RuleList() {}
    ~RuleList() {}
    void addRule(Rule &rule) { mRuleList.push_back(rule); }
    bool getAppChoice(Scenario &toCheck)
    {
        // Initialize the choice to the system-wide default (that should be set in the default
        // rule, but just in case, set it here too):
        bool appChoice = true;
        ALOGV("Checking scenario against %d ANGLE-for-Android rules:",
              static_cast<int>(mRuleList.size()));

        for (auto &it : mRuleList)
        {
            ALOGV("  Checking Rule: \"%s\" (to see whether there's a match)",
                  it.mDescription.c_str());
            if (it.match(toCheck))
            {
                ALOGV("  -> Rule matches.  Setting the app choice to %s",
                      it.getAppChoice() ? "true" : "false");
                appChoice = it.getAppChoice();
            }
            else
            {
                ALOGV("  -> Rule doesn't match.");
            }
        }

        return appChoice;
    }
    bool getAnswer(Scenario &toCheck)
    {
        // Initialize the answer to the system-wide default (that should be set in the default
        // rule, but just in case, set it here too):
        bool answer = false;
        ALOGV("Checking scenario against %d ANGLE-for-Android rules:",
              static_cast<int>(mRuleList.size()));

        for (auto &it : mRuleList)
        {
            ALOGV("  Checking Rule: \"%s\" (to see whether there's a match)",
                  it.mDescription.c_str());
            if (it.match(toCheck))
            {
                ALOGV("  -> Rule matches.  Setting the answer to %s",
                      it.getAnswer() ? "true" : "false");
                answer = it.getAnswer();
            }
            else
            {
                ALOGV("  -> Rule doesn't match.");
            }
        }

        return answer;
    }
    static RuleList *ReadRulesFromJsonString(std::string jsonFileContents)
    {
        RuleList *rules = new RuleList;

        // Open the file and start parsing it:
        using namespace std;
        Json::Reader jReader;
        Json::Value jTopLevelObject;
        jReader.parse(jsonFileContents, jTopLevelObject);
        Json::Value jRules = jTopLevelObject[kJsonRules];
        for (unsigned int i = 0; i < jRules.size(); i++)
        {
            Json::Value jRule           = jRules[i];
            std::string ruleDescription = jRule[kJsonRule].asString();
            bool ruleAppChoice          = jRule[kJsonAppChoice].asBool();
            bool ruleAnswer             = jRule[kJsonNonChoice].asBool();
            // TODO (ianelliott@) (b/113346561) appropriately destruct lists and
            // other items that get created from json parsing
            Rule *newRule = new Rule(ruleDescription, ruleAppChoice, ruleAnswer);

            Json::Value jApps = jRule[kJsonApplications];
            for (unsigned int j = 0; j < jApps.size(); j++)
            {
                Json::Value jApp    = jApps[j];
                Application *newApp = Application::CreateApplicationFromJson(jApp);
                // TODO (ianelliott@) (b/113346561) appropriately destruct lists and
                // other items that get created from json parsing
                newRule->addApp(*newApp);
            }

            Json::Value jDevs = jRule[kJsonDevices];
            for (unsigned int j = 0; j < jDevs.size(); j++)
            {
                Json::Value jDev = jDevs[j];
                Device *newDev   = Device::CreateDeviceFromJson(jDev);

                Json::Value jGPUs = jDev[kJsonGPUs];
                for (unsigned int k = 0; k < jGPUs.size(); k++)
                {
                    Json::Value jGPU = jGPUs[k];
                    GPU *newGPU      = GPU::CreateGpuFromJson(jGPU);
                    if (newGPU)
                    {
                        newDev->addGPU(*newGPU);
                    }
                }
                newRule->addDev(*newDev);
            }

            // TODO: Need to manage memory
            rules->addRule(*newRule);
        }

        // Make sure there is at least one, default rule.  If not, add it here:
        int nRules = rules->mRuleList.size();
        if (nRules == 0)
        {
            Rule defaultRule("Default Rule", true, false);
            rules->addRule(defaultRule);
        }
        return rules;
    }
    void logRules()
    {
        ALOGV("Showing %d ANGLE-for-Android rules:", static_cast<int>(mRuleList.size()));
        for (auto &it : mRuleList)
        {
            it.logRule();
        }
    }

  public:
    std::list<Rule> mRuleList;
};

//}  // namespace angle_for_android

extern "C" {

// using namespace angle_for_android;

ANGLE_EXPORT bool ANGLEUseForApplication(const char *appName,
                                         const char *deviceMfr,
                                         const char *deviceModel,
                                         ANGLEPreference developerOption,
                                         ANGLEPreference appPreference)
{
    Scenario scenario(appName, deviceMfr, deviceModel);
    bool rtn = false;
    scenario.logScenario();

    // #include the contents of the file into a string and then parse it:
    using namespace std;
    // Embed the rules file contents into a string:
    const char *s =
#include "a4a_rules.json"
        ;
    std::string jsonFileContents = s;
    RuleList *rules              = RuleList::ReadRulesFromJsonString(jsonFileContents);
    rules->logRules();

    if (developerOption != ANGLE_NO_PREFERENCE)
    {
        rtn = (developerOption == ANGLE_PREFER_ANGLE);
    }
    else if ((appPreference != ANGLE_NO_PREFERENCE) && rules->getAppChoice(scenario))
    {
        rtn = (appPreference == ANGLE_PREFER_ANGLE);
    }
    else
    {
        rtn = rules->getAnswer(scenario);
    }
    ALOGV("Application \"%s\" should %s ANGLE", appName, rtn ? "use" : "NOT use");
    delete rules;
    return rtn;
}

ANGLE_EXPORT bool ANGLEGetUtilityAPI(unsigned int *versionToUse)
{
    if (*versionToUse >= kFeatureVersion_LowestSupported)
    {
        if (*versionToUse <= kFeatureVersion_HighestSupported)
        {
            // This versionToUse is valid, and doesn't need to be changed.
            return true;
        }
        else
        {
            // The versionToUse is greater than the highest version supported; change it to the
            // highest version supported (caller will decide if it can use that version).
            *versionToUse = kFeatureVersion_HighestSupported;
            return true;
        }
    }
    else
    {
        // The versionToUse is less than the lowest version supported, which is an error.
        return false;
    }
}

ANGLE_EXPORT bool AndroidUseANGLEForApplication(int rules_fd,
                                                long rules_offset,
                                                long rules_length,
                                                const char *appName,
                                                const char *deviceMfr,
                                                const char *deviceModel)
{
    Scenario scenario(appName, deviceMfr, deviceModel);
    bool rtn = false;
    scenario.logScenario();

    // Read the contents of the file into a string and then parse it:
    if (rules_fd < 0)
    {
        ALOGW("Asked to read a non-open JSON file");
        return rtn;
    }
    off_t fileSize       = rules_length;
    off_t startOfContent = rules_offset;
    // This is temporary magic--while there's extra stuff at the start of the file
    // (so that it can be #include'd into the source code):
    startOfContent += 8;
    fileSize -= (8 + 7 + 2);
    lseek(rules_fd, startOfContent, SEEK_SET);
    char *buffer                 = new char[fileSize + 1];
    ssize_t numBytesRead         = read(rules_fd, buffer, fileSize);
    buffer[numBytesRead]         = '\0';
    std::string jsonFileContents = std::string(buffer);
    delete[] buffer;
    RuleList *rules = RuleList::ReadRulesFromJsonString(jsonFileContents);
    rules->logRules();

    rtn = rules->getAnswer(scenario);
    ALOGV("Application \"%s\" should %s ANGLE", appName, rtn ? "use" : "NOT use");

    delete rules;
    return rtn;
}

}  // extern "C"
