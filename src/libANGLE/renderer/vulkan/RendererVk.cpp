//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// RendererVk.cpp:
//    Implements the class methods for RendererVk.
//

#include "libANGLE/renderer/vulkan/RendererVk.h"

// Placing this first seems to solve an intellisense bug.
#include "libANGLE/renderer/vulkan/vk_utils.h"

#include <EGL/eglext.h>

#include "common/debug.h"
#include "common/system_utils.h"
#include "libANGLE/renderer/driver_utils.h"
#include "libANGLE/renderer/vulkan/CommandGraph.h"
#include "libANGLE/renderer/vulkan/CompilerVk.h"
#include "libANGLE/renderer/vulkan/FramebufferVk.h"
#include "libANGLE/renderer/vulkan/GlslangWrapper.h"
#include "libANGLE/renderer/vulkan/ProgramVk.h"
#include "libANGLE/renderer/vulkan/VertexArrayVk.h"
#include "libANGLE/renderer/vulkan/vk_caps_utils.h"
#include "libANGLE/renderer/vulkan/vk_format_utils.h"
#include "platform/Platform.h"

// Consts
namespace
{
const uint32_t kMockVendorID     = 0xba5eba11;
const uint32_t kMockDeviceID     = 0xf005ba11;
constexpr char kMockDeviceName[] = "Vulkan Mock Device";
}  // anonymous namespace

namespace rx
{

namespace
{
// We currently only allocate 2 uniform buffer per descriptor set, one for the fragment shader and
// one for the vertex shader.
constexpr size_t kUniformBufferDescriptorsPerDescriptorSet = 2;

bool ShouldEnableMockICD(const egl::AttributeMap &attribs)
{
#if !defined(ANGLE_PLATFORM_ANDROID)
    // Mock ICD does not currently run on Android
    return (attribs.get(EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE,
                        EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE) ==
            EGL_PLATFORM_ANGLE_DEVICE_TYPE_NULL_ANGLE);
#else
    return false;
#endif  // !defined(ANGLE_PLATFORM_ANDROID)
}

VkResult VerifyExtensionsPresent(const std::vector<VkExtensionProperties> &extensionProps,
                                 const std::vector<const char *> &enabledExtensionNames)
{
    // Compile the extensions names into a set.
    std::set<std::string> extensionNames;
    for (const auto &extensionProp : extensionProps)
    {
        extensionNames.insert(extensionProp.extensionName);
    }

    for (const char *extensionName : enabledExtensionNames)
    {
        if (extensionNames.count(extensionName) == 0)
        {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    return VK_SUCCESS;
}

VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback(VkDebugReportFlagsEXT flags,
                                                   VkDebugReportObjectTypeEXT objectType,
                                                   uint64_t object,
                                                   size_t location,
                                                   int32_t messageCode,
                                                   const char *layerPrefix,
                                                   const char *message,
                                                   void *userData)
{
    if ((flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) != 0)
    {
        ERR() << message;
#if !defined(NDEBUG)
        // Abort the call in Debug builds.
        return VK_TRUE;
#endif
    }
    else if ((flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) != 0)
    {
        WARN() << message;
    }
    else
    {
        // Uncomment this if you want Vulkan spam.
        // WARN() << message;
    }

    return VK_FALSE;
}

// If we're loading the validation layers, we could be running from any random directory.
// Change to the executable directory so we can find the layers, then change back to the
// previous directory to be safe we don't disrupt the application.
class ScopedVkLoaderEnvironment : angle::NonCopyable
{
  public:
    ScopedVkLoaderEnvironment(bool enableValidationLayers, bool enableMockICD)
        : mEnableValidationLayers(enableValidationLayers),
          mEnableMockICD(enableMockICD),
          mChangedCWD(false),
          mChangedICDPath(false)
    {
// Changing CWD and setting environment variables makes no sense on Android,
// since this code is a part of Java application there.
// Android Vulkan loader doesn't need this either.
#if !defined(ANGLE_PLATFORM_ANDROID)
        if (enableMockICD)
        {
            // Override environment variable to use built Mock ICD
            // ANGLE_VK_ICD_JSON gets set to the built mock ICD in BUILD.gn
            mPreviousICDPath = angle::GetEnvironmentVar(g_VkICDPathEnv);
            mChangedICDPath  = angle::SetEnvironmentVar(g_VkICDPathEnv, ANGLE_VK_ICD_JSON);
            if (!mChangedICDPath)
            {
                ERR() << "Error setting Path for Mock/Null Driver.";
                mEnableMockICD = false;
            }
        }
        if (mEnableValidationLayers)
        {
            const auto &cwd = angle::GetCWD();
            if (!cwd.valid())
            {
                ERR() << "Error getting CWD for Vulkan layers init.";
                mEnableValidationLayers = false;
            }
            else
            {
                mPreviousCWD       = cwd.value();
                const char *exeDir = angle::GetExecutableDirectory();
                mChangedCWD        = angle::SetCWD(exeDir);
                if (!mChangedCWD)
                {
                    ERR() << "Error setting CWD for Vulkan layers init.";
                    mEnableValidationLayers = false;
                }
            }
        }

        // Override environment variable to use the ANGLE layers.
        if (mEnableValidationLayers)
        {
            if (!angle::PrependPathToEnvironmentVar(g_VkLoaderLayersPathEnv, ANGLE_VK_DATA_DIR))
            {
                ERR() << "Error setting environment for Vulkan layers init.";
                mEnableValidationLayers = false;
            }
        }
#endif  // !defined(ANGLE_PLATFORM_ANDROID)
    }

    ~ScopedVkLoaderEnvironment()
    {
        if (mChangedCWD)
        {
#if !defined(ANGLE_PLATFORM_ANDROID)
            ASSERT(mPreviousCWD.valid());
            angle::SetCWD(mPreviousCWD.value().c_str());
#endif  // !defined(ANGLE_PLATFORM_ANDROID)
        }
        if (mChangedICDPath)
        {
            angle::SetEnvironmentVar(g_VkICDPathEnv, mPreviousICDPath.value().c_str());
        }
    }

    bool canEnableValidationLayers() const { return mEnableValidationLayers; }

    bool canEnableMockICD() const { return mEnableMockICD; }

  private:
    bool mEnableValidationLayers;
    bool mEnableMockICD;
    bool mChangedCWD;
    Optional<std::string> mPreviousCWD;
    bool mChangedICDPath;
    Optional<std::string> mPreviousICDPath;
};

}  // anonymous namespace

// CommandBatch implementation.
RendererVk::CommandBatch::CommandBatch() = default;

RendererVk::CommandBatch::~CommandBatch() = default;

RendererVk::CommandBatch::CommandBatch(CommandBatch &&other)
    : commandPool(std::move(other.commandPool)), fence(std::move(other.fence)), serial(other.serial)
{
}

RendererVk::CommandBatch &RendererVk::CommandBatch::operator=(CommandBatch &&other)
{
    std::swap(commandPool, other.commandPool);
    std::swap(fence, other.fence);
    std::swap(serial, other.serial);
    return *this;
}

void RendererVk::CommandBatch::destroy(VkDevice device)
{
    commandPool.destroy(device);
    fence.destroy(device);
}

// RendererVk implementation.
RendererVk::RendererVk()
    : mCapsInitialized(false),
      mInstance(VK_NULL_HANDLE),
      mEnableValidationLayers(false),
      mDebugReportCallback(VK_NULL_HANDLE),
      mPhysicalDevice(VK_NULL_HANDLE),
      mQueue(VK_NULL_HANDLE),
      mCurrentQueueFamilyIndex(std::numeric_limits<uint32_t>::max()),
      mDevice(VK_NULL_HANDLE),
      mGlslangWrapper(nullptr),
      mLastCompletedQueueSerial(mQueueSerialFactory.generate()),
      mCurrentQueueSerial(mQueueSerialFactory.generate())
{
}

RendererVk::~RendererVk()
{
    if (!mInFlightCommands.empty() || !mGarbage.empty())
    {
        // TODO(jmadill): Not nice to pass nullptr here, but shouldn't be a problem.
        vk::Error error = finish(nullptr);
        if (error.isError())
        {
            ERR() << "Error during VK shutdown: " << error;
        }
    }

    mPipelineLayoutCache.destroy(mDevice);
    mDescriptorSetLayoutCache.destroy(mDevice);

    mRenderPassCache.destroy(mDevice);
    mPipelineCache.destroy(mDevice);
    mShaderLibrary.destroy(mDevice);

    if (mGlslangWrapper)
    {
        GlslangWrapper::ReleaseReference();
        mGlslangWrapper = nullptr;
    }

    if (mCommandPool.valid())
    {
        mCommandPool.destroy(mDevice);
    }

    if (mDevice)
    {
        vkDestroyDevice(mDevice, nullptr);
        mDevice = VK_NULL_HANDLE;
    }

    if (mDebugReportCallback)
    {
        ASSERT(mInstance);
        auto destroyDebugReportCallback = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(
            vkGetInstanceProcAddr(mInstance, "vkDestroyDebugReportCallbackEXT"));
        ASSERT(destroyDebugReportCallback);
        destroyDebugReportCallback(mInstance, mDebugReportCallback, nullptr);
    }

    if (mInstance)
    {
        vkDestroyInstance(mInstance, nullptr);
        mInstance = VK_NULL_HANDLE;
    }

    mPhysicalDevice = VK_NULL_HANDLE;
}

void ChoosePhysicalDevice(const std::vector<VkPhysicalDevice> &physicalDevices,
                          bool preferMockICD,
                          VkPhysicalDevice *physicalDeviceOut,
                          VkPhysicalDeviceProperties *physicalDevicePropertiesOut)
{
    ASSERT(!physicalDevices.empty());
    if (preferMockICD)
    {
        for (const VkPhysicalDevice &physicalDevice : physicalDevices)
        {
            vkGetPhysicalDeviceProperties(physicalDevice, physicalDevicePropertiesOut);
            if ((kMockVendorID == physicalDevicePropertiesOut->vendorID) &&
                (kMockDeviceID == physicalDevicePropertiesOut->deviceID) &&
                (strcmp(kMockDeviceName, physicalDevicePropertiesOut->deviceName) == 0))
            {
                *physicalDeviceOut = physicalDevice;
                return;
            }
        }
        WARN() << "Vulkan Mock Driver was requested but Mock Device was not found. Using default "
                  "physicalDevice instead.";
    }

    // Fall back to first device.
    *physicalDeviceOut = physicalDevices[0];
    vkGetPhysicalDeviceProperties(*physicalDeviceOut, physicalDevicePropertiesOut);
}

vk::Error RendererVk::initialize(const egl::AttributeMap &attribs, const char *wsiName)
{
    ScopedVkLoaderEnvironment scopedEnvironment(ShouldUseDebugLayers(attribs),
                                                ShouldEnableMockICD(attribs));
    mEnableValidationLayers = scopedEnvironment.canEnableValidationLayers();
    bool enableMockICD      = scopedEnvironment.canEnableMockICD();

    // Gather global layer properties.
    uint32_t instanceLayerCount = 0;
    ANGLE_VK_TRY(vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr));

    std::vector<VkLayerProperties> instanceLayerProps(instanceLayerCount);
    if (instanceLayerCount > 0)
    {
        ANGLE_VK_TRY(
            vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayerProps.data()));
    }

    uint32_t instanceExtensionCount = 0;
    ANGLE_VK_TRY(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr));

    std::vector<VkExtensionProperties> instanceExtensionProps(instanceExtensionCount);
    if (instanceExtensionCount > 0)
    {
        ANGLE_VK_TRY(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount,
                                                            instanceExtensionProps.data()));
    }

    const char *const *enabledLayerNames = nullptr;
    uint32_t enabledLayerCount           = 0;
    if (mEnableValidationLayers)
    {
        bool layersRequested =
            (attribs.get(EGL_PLATFORM_ANGLE_DEBUG_LAYERS_ENABLED_ANGLE, EGL_DONT_CARE) == EGL_TRUE);
        mEnableValidationLayers = GetAvailableValidationLayers(
            instanceLayerProps, layersRequested, &enabledLayerNames, &enabledLayerCount);
    }

    std::vector<const char *> enabledInstanceExtensions;
    enabledInstanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    enabledInstanceExtensions.push_back(wsiName);

    // TODO(jmadill): Should be able to continue initialization if debug report ext missing.
    if (mEnableValidationLayers)
    {
        enabledInstanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }

    // Verify the required extensions are in the extension names set. Fail if not.
    ANGLE_VK_TRY(VerifyExtensionsPresent(instanceExtensionProps, enabledInstanceExtensions));

    VkApplicationInfo applicationInfo;
    applicationInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pNext              = nullptr;
    applicationInfo.pApplicationName   = "ANGLE";
    applicationInfo.applicationVersion = 1;
    applicationInfo.pEngineName        = "ANGLE";
    applicationInfo.engineVersion      = 1;
    applicationInfo.apiVersion         = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instanceInfo;
    instanceInfo.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pNext            = nullptr;
    instanceInfo.flags            = 0;
    instanceInfo.pApplicationInfo = &applicationInfo;

    // Enable requested layers and extensions.
    instanceInfo.enabledExtensionCount = static_cast<uint32_t>(enabledInstanceExtensions.size());
    instanceInfo.ppEnabledExtensionNames =
        enabledInstanceExtensions.empty() ? nullptr : enabledInstanceExtensions.data();
    instanceInfo.enabledLayerCount   = enabledLayerCount;
    instanceInfo.ppEnabledLayerNames = enabledLayerNames;

    ANGLE_VK_TRY(vkCreateInstance(&instanceInfo, nullptr, &mInstance));

    if (mEnableValidationLayers)
    {
        VkDebugReportCallbackCreateInfoEXT debugReportInfo;

        debugReportInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        debugReportInfo.pNext = nullptr;
        debugReportInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT |
                                VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
                                VK_DEBUG_REPORT_INFORMATION_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT;
        debugReportInfo.pfnCallback = &DebugReportCallback;
        debugReportInfo.pUserData   = this;

        auto createDebugReportCallback = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(
            vkGetInstanceProcAddr(mInstance, "vkCreateDebugReportCallbackEXT"));
        ASSERT(createDebugReportCallback);
        ANGLE_VK_TRY(
            createDebugReportCallback(mInstance, &debugReportInfo, nullptr, &mDebugReportCallback));
    }

    uint32_t physicalDeviceCount = 0;
    ANGLE_VK_TRY(vkEnumeratePhysicalDevices(mInstance, &physicalDeviceCount, nullptr));
    ANGLE_VK_CHECK(physicalDeviceCount > 0, VK_ERROR_INITIALIZATION_FAILED);

    // TODO(jmadill): Handle multiple physical devices. For now, use the first device.
    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    ANGLE_VK_TRY(
        vkEnumeratePhysicalDevices(mInstance, &physicalDeviceCount, physicalDevices.data()));
    ChoosePhysicalDevice(physicalDevices, enableMockICD, &mPhysicalDevice,
                         &mPhysicalDeviceProperties);

    // Ensure we can find a graphics queue family.
    uint32_t queueCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &queueCount, nullptr);

    ANGLE_VK_CHECK(queueCount > 0, VK_ERROR_INITIALIZATION_FAILED);

    mQueueFamilyProperties.resize(queueCount);
    vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &queueCount,
                                             mQueueFamilyProperties.data());

    size_t graphicsQueueFamilyCount   = false;
    uint32_t firstGraphicsQueueFamily = 0;
    for (uint32_t familyIndex = 0; familyIndex < queueCount; ++familyIndex)
    {
        const auto &queueInfo = mQueueFamilyProperties[familyIndex];
        if ((queueInfo.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
        {
            ASSERT(queueInfo.queueCount > 0);
            graphicsQueueFamilyCount++;
            if (firstGraphicsQueueFamily == 0)
            {
                firstGraphicsQueueFamily = familyIndex;
            }
            break;
        }
    }

    ANGLE_VK_CHECK(graphicsQueueFamilyCount > 0, VK_ERROR_INITIALIZATION_FAILED);

    initFeatures();

    // If only one queue family, go ahead and initialize the device. If there is more than one
    // queue, we'll have to wait until we see a WindowSurface to know which supports present.
    if (graphicsQueueFamilyCount == 1)
    {
        ANGLE_TRY(initializeDevice(firstGraphicsQueueFamily));
    }

    // Store the physical device memory properties so we can find the right memory pools.
    mMemoryProperties.init(mPhysicalDevice);

    mGlslangWrapper = GlslangWrapper::GetReference();

    // Initialize the format table.
    mFormatTable.initialize(mPhysicalDevice, &mNativeTextureCaps,
                            &mNativeCaps.compressedTextureFormats);

    return vk::NoError();
}

vk::Error RendererVk::initializeDevice(uint32_t queueFamilyIndex)
{
    uint32_t deviceLayerCount = 0;
    ANGLE_VK_TRY(vkEnumerateDeviceLayerProperties(mPhysicalDevice, &deviceLayerCount, nullptr));

    std::vector<VkLayerProperties> deviceLayerProps(deviceLayerCount);
    if (deviceLayerCount > 0)
    {
        ANGLE_VK_TRY(vkEnumerateDeviceLayerProperties(mPhysicalDevice, &deviceLayerCount,
                                                      deviceLayerProps.data()));
    }

    uint32_t deviceExtensionCount = 0;
    ANGLE_VK_TRY(vkEnumerateDeviceExtensionProperties(mPhysicalDevice, nullptr,
                                                      &deviceExtensionCount, nullptr));

    std::vector<VkExtensionProperties> deviceExtensionProps(deviceExtensionCount);
    if (deviceExtensionCount > 0)
    {
        ANGLE_VK_TRY(vkEnumerateDeviceExtensionProperties(
            mPhysicalDevice, nullptr, &deviceExtensionCount, deviceExtensionProps.data()));
    }

    const char *const *enabledLayerNames = nullptr;
    uint32_t enabledLayerCount           = 0;
    if (mEnableValidationLayers)
    {
        mEnableValidationLayers = GetAvailableValidationLayers(
            deviceLayerProps, false, &enabledLayerNames, &enabledLayerCount);
    }

    std::vector<const char *> enabledDeviceExtensions;
    enabledDeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    // Selectively enable KHR_MAINTENANCE1 to support viewport flipping.
    if (getFeatures().flipViewportY)
    {
        enabledDeviceExtensions.push_back(VK_KHR_MAINTENANCE1_EXTENSION_NAME);
    }

    ANGLE_VK_TRY(VerifyExtensionsPresent(deviceExtensionProps, enabledDeviceExtensions));

    VkDeviceQueueCreateInfo queueCreateInfo;

    float zeroPriority = 0.0f;

    queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.pNext            = nullptr;
    queueCreateInfo.flags            = 0;
    queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
    queueCreateInfo.queueCount       = 1;
    queueCreateInfo.pQueuePriorities = &zeroPriority;

    // Initialize the device
    VkDeviceCreateInfo createInfo;

    createInfo.sType                 = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext                 = nullptr;
    createInfo.flags                 = 0;
    createInfo.queueCreateInfoCount  = 1;
    createInfo.pQueueCreateInfos     = &queueCreateInfo;
    createInfo.enabledLayerCount     = enabledLayerCount;
    createInfo.ppEnabledLayerNames   = enabledLayerNames;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledDeviceExtensions.size());
    createInfo.ppEnabledExtensionNames =
        enabledDeviceExtensions.empty() ? nullptr : enabledDeviceExtensions.data();
    createInfo.pEnabledFeatures = nullptr;  // TODO(jmadill): features

    ANGLE_VK_TRY(vkCreateDevice(mPhysicalDevice, &createInfo, nullptr, &mDevice));

    mCurrentQueueFamilyIndex = queueFamilyIndex;

    vkGetDeviceQueue(mDevice, mCurrentQueueFamilyIndex, 0, &mQueue);

    // Initialize the command pool now that we know the queue family index.
    VkCommandPoolCreateInfo commandPoolInfo;
    commandPoolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolInfo.pNext            = nullptr;
    commandPoolInfo.flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    commandPoolInfo.queueFamilyIndex = mCurrentQueueFamilyIndex;

    ANGLE_TRY(mCommandPool.init(mDevice, commandPoolInfo));

    return vk::NoError();
}

vk::Error RendererVk::selectPresentQueueForSurface(VkSurfaceKHR surface, uint32_t *presentQueueOut)
{
    // We've already initialized a device, and can't re-create it unless it's never been used.
    // TODO(jmadill): Handle the re-creation case if necessary.
    if (mDevice != VK_NULL_HANDLE)
    {
        ASSERT(mCurrentQueueFamilyIndex != std::numeric_limits<uint32_t>::max());

        // Check if the current device supports present on this surface.
        VkBool32 supportsPresent = VK_FALSE;
        ANGLE_VK_TRY(vkGetPhysicalDeviceSurfaceSupportKHR(mPhysicalDevice, mCurrentQueueFamilyIndex,
                                                          surface, &supportsPresent));

        if (supportsPresent == VK_TRUE)
        {
            *presentQueueOut = mCurrentQueueFamilyIndex;
            return vk::NoError();
        }
    }

    // Find a graphics and present queue.
    Optional<uint32_t> newPresentQueue;
    uint32_t queueCount = static_cast<uint32_t>(mQueueFamilyProperties.size());
    for (uint32_t queueIndex = 0; queueIndex < queueCount; ++queueIndex)
    {
        const auto &queueInfo = mQueueFamilyProperties[queueIndex];
        if ((queueInfo.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
        {
            VkBool32 supportsPresent = VK_FALSE;
            ANGLE_VK_TRY(vkGetPhysicalDeviceSurfaceSupportKHR(mPhysicalDevice, queueIndex, surface,
                                                              &supportsPresent));

            if (supportsPresent == VK_TRUE)
            {
                newPresentQueue = queueIndex;
                break;
            }
        }
    }

    ANGLE_VK_CHECK(newPresentQueue.valid(), VK_ERROR_INITIALIZATION_FAILED);
    ANGLE_TRY(initializeDevice(newPresentQueue.value()));

    *presentQueueOut = newPresentQueue.value();
    return vk::NoError();
}

std::string RendererVk::getVendorString() const
{
    return GetVendorString(mPhysicalDeviceProperties.vendorID);
}

std::string RendererVk::getRendererDescription() const
{
    std::stringstream strstr;

    uint32_t apiVersion = mPhysicalDeviceProperties.apiVersion;

    strstr << "Vulkan ";
    strstr << VK_VERSION_MAJOR(apiVersion) << ".";
    strstr << VK_VERSION_MINOR(apiVersion) << ".";
    strstr << VK_VERSION_PATCH(apiVersion);

    strstr << "(";

    // In the case of NVIDIA, deviceName does not necessarily contain "NVIDIA". Add "NVIDIA" so that
    // Vulkan end2end tests can be selectively disabled on NVIDIA. TODO(jmadill): should not be
    // needed after http://anglebug.com/1874 is fixed and end2end_tests use more sophisticated
    // driver detection.
    if (mPhysicalDeviceProperties.vendorID == VENDOR_ID_NVIDIA)
    {
        strstr << GetVendorString(mPhysicalDeviceProperties.vendorID) << " ";
    }

    strstr << mPhysicalDeviceProperties.deviceName << ")";

    return strstr.str();
}

void RendererVk::initFeatures()
{
    // Use OpenGL line rasterization rules by default.
    mFeatures.basicGLLineRasterization = true;

    // For now, set this manually to true to enable viewport flipping. A couple of features are not
    // working well like copyTexImage, copySubTexImage, blit, and probably some more. Until
    // everything is fixed, we will keep the viewport flipping feature disabled.
    mFeatures.flipViewportY = false;
}

void RendererVk::ensureCapsInitialized() const
{
    if (!mCapsInitialized)
    {
        vk::GenerateCaps(mPhysicalDeviceProperties, mNativeTextureCaps, &mNativeCaps,
                         &mNativeExtensions, &mNativeLimitations);
        mCapsInitialized = true;
    }
}

const gl::Caps &RendererVk::getNativeCaps() const
{
    ensureCapsInitialized();
    return mNativeCaps;
}

const gl::TextureCapsMap &RendererVk::getNativeTextureCaps() const
{
    ensureCapsInitialized();
    return mNativeTextureCaps;
}

const gl::Extensions &RendererVk::getNativeExtensions() const
{
    ensureCapsInitialized();
    return mNativeExtensions;
}

const gl::Limitations &RendererVk::getNativeLimitations() const
{
    ensureCapsInitialized();
    return mNativeLimitations;
}

uint32_t RendererVk::getMaxActiveTextures()
{
    // TODO(lucferron): expose this limitation to GL in Context Caps
    return std::min<uint32_t>(mPhysicalDeviceProperties.limits.maxPerStageDescriptorSamplers,
                              gl::IMPLEMENTATION_MAX_ACTIVE_TEXTURES);
}

const vk::CommandPool &RendererVk::getCommandPool() const
{
    return mCommandPool;
}

vk::Error RendererVk::finish(const gl::Context *context)
{
    if (!mCommandGraph.empty())
    {
        vk::Scoped<vk::CommandBuffer> commandBatch(mDevice);
        ANGLE_TRY(flushCommandGraph(context, &commandBatch.get()));

        VkSubmitInfo submitInfo;
        submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext                = nullptr;
        submitInfo.waitSemaphoreCount   = 0;
        submitInfo.pWaitSemaphores      = nullptr;
        submitInfo.pWaitDstStageMask    = nullptr;
        submitInfo.commandBufferCount   = 1;
        submitInfo.pCommandBuffers      = commandBatch.get().ptr();
        submitInfo.signalSemaphoreCount = 0;
        submitInfo.pSignalSemaphores    = nullptr;

        ANGLE_TRY(submitFrame(submitInfo, std::move(commandBatch.get())));
    }

    ASSERT(mQueue != VK_NULL_HANDLE);
    ANGLE_VK_TRY(vkQueueWaitIdle(mQueue));
    freeAllInFlightResources();
    return vk::NoError();
}

void RendererVk::freeAllInFlightResources()
{
    for (CommandBatch &batch : mInFlightCommands)
    {
        batch.fence.destroy(mDevice);
        batch.commandPool.destroy(mDevice);
    }
    mInFlightCommands.clear();

    for (auto &garbage : mGarbage)
    {
        garbage.destroy(mDevice);
    }
    mGarbage.clear();
}

vk::Error RendererVk::checkInFlightCommands()
{
    int finishedCount = 0;

    for (CommandBatch &batch : mInFlightCommands)
    {
        VkResult result = batch.fence.getStatus(mDevice);
        if (result == VK_NOT_READY)
            break;

        ANGLE_VK_TRY(result);
        ASSERT(batch.serial > mLastCompletedQueueSerial);
        mLastCompletedQueueSerial = batch.serial;

        batch.fence.destroy(mDevice);
        batch.commandPool.destroy(mDevice);
        ++finishedCount;
    }

    mInFlightCommands.erase(mInFlightCommands.begin(), mInFlightCommands.begin() + finishedCount);

    size_t freeIndex = 0;
    for (; freeIndex < mGarbage.size(); ++freeIndex)
    {
        if (!mGarbage[freeIndex].destroyIfComplete(mDevice, mLastCompletedQueueSerial))
            break;
    }

    // Remove the entries from the garbage list - they should be ready to go.
    if (freeIndex > 0)
    {
        mGarbage.erase(mGarbage.begin(), mGarbage.begin() + freeIndex);
    }

    return vk::NoError();
}

vk::Error RendererVk::submitFrame(const VkSubmitInfo &submitInfo, vk::CommandBuffer &&commandBuffer)
{
    VkFenceCreateInfo fenceInfo;
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.pNext = nullptr;
    fenceInfo.flags = 0;

    vk::Scoped<CommandBatch> scopedBatch(mDevice);
    CommandBatch &batch = scopedBatch.get();
    ANGLE_TRY(batch.fence.init(mDevice, fenceInfo));

    ANGLE_VK_TRY(vkQueueSubmit(mQueue, 1, &submitInfo, batch.fence.getHandle()));

    // Store this command buffer in the in-flight list.
    batch.commandPool = std::move(mCommandPool);
    batch.serial      = mCurrentQueueSerial;

    mInFlightCommands.emplace_back(scopedBatch.release());

    // Sanity check.
    ASSERT(mInFlightCommands.size() < 1000u);

    // Increment the queue serial. If this fails, we should restart ANGLE.
    // TODO(jmadill): Overflow check.
    mCurrentQueueSerial = mQueueSerialFactory.generate();

    ANGLE_TRY(checkInFlightCommands());

    // Simply null out the command buffer here - it was allocated using the command pool.
    commandBuffer.releaseHandle();

    // Reallocate the command pool for next frame.
    // TODO(jmadill): Consider reusing command pools.
    VkCommandPoolCreateInfo poolInfo;
    poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.pNext            = nullptr;
    poolInfo.flags            = 0;
    poolInfo.queueFamilyIndex = mCurrentQueueFamilyIndex;

    ANGLE_TRY(mCommandPool.init(mDevice, poolInfo));

    return vk::NoError();
}

GlslangWrapper *RendererVk::getGlslangWrapper() const
{
    return mGlslangWrapper;
}

Serial RendererVk::getCurrentQueueSerial() const
{
    return mCurrentQueueSerial;
}

bool RendererVk::isSerialInUse(Serial serial) const
{
    return serial > mLastCompletedQueueSerial;
}

vk::Error RendererVk::getCompatibleRenderPass(const vk::RenderPassDesc &desc,
                                              vk::RenderPass **renderPassOut)
{
    return mRenderPassCache.getCompatibleRenderPass(mDevice, mCurrentQueueSerial, desc,
                                                    renderPassOut);
}

vk::Error RendererVk::getRenderPassWithOps(const vk::RenderPassDesc &desc,
                                           const vk::AttachmentOpsArray &ops,
                                           vk::RenderPass **renderPassOut)
{
    return mRenderPassCache.getRenderPassWithOps(mDevice, mCurrentQueueSerial, desc, ops,
                                                 renderPassOut);
}

vk::CommandGraph *RendererVk::getCommandGraph()
{
    return &mCommandGraph;
}

vk::Error RendererVk::flushCommandGraph(const gl::Context *context, vk::CommandBuffer *commandBatch)
{
    return mCommandGraph.submitCommands(mDevice, mCurrentQueueSerial, &mRenderPassCache,
                                        &mCommandPool, commandBatch);
}

vk::Error RendererVk::flush(const gl::Context *context,
                            const vk::Semaphore &waitSemaphore,
                            const vk::Semaphore &signalSemaphore)
{
    vk::Scoped<vk::CommandBuffer> commandBatch(mDevice);
    ANGLE_TRY(flushCommandGraph(context, &commandBatch.get()));

    VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

    VkSubmitInfo submitInfo;
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext                = nullptr;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = waitSemaphore.ptr();
    submitInfo.pWaitDstStageMask    = &waitStageMask;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = commandBatch.get().ptr();
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = signalSemaphore.ptr();

    ANGLE_TRY(submitFrame(submitInfo, commandBatch.release()));
    return vk::NoError();
}

Serial RendererVk::issueShaderSerial()
{
    return mShaderSerialFactory.generate();
}

vk::Error RendererVk::getAppPipeline(const ProgramVk *programVk,
                                     const vk::PipelineDesc &desc,
                                     const gl::AttributesMask &activeAttribLocationsMask,
                                     vk::PipelineAndSerial **pipelineOut)
{
    ASSERT(programVk->getVertexModuleSerial() ==
           desc.getShaderStageInfo()[vk::ShaderType::VertexShader].moduleSerial);
    ASSERT(programVk->getFragmentModuleSerial() ==
           desc.getShaderStageInfo()[vk::ShaderType::FragmentShader].moduleSerial);

    // Pull in a compatible RenderPass.
    vk::RenderPass *compatibleRenderPass = nullptr;
    ANGLE_TRY(getCompatibleRenderPass(desc.getRenderPassDesc(), &compatibleRenderPass));

    const vk::PipelineLayout &pipelineLayout = programVk->getPipelineLayout();

    return mPipelineCache.getPipeline(mDevice, *compatibleRenderPass, pipelineLayout,
                                      activeAttribLocationsMask, programVk->getLinkedVertexModule(),
                                      programVk->getLinkedFragmentModule(), desc, pipelineOut);
}

vk::Error RendererVk::getInternalPipeline(const vk::ShaderAndSerial &vertexShader,
                                          const vk::ShaderAndSerial &fragmentShader,
                                          const vk::PipelineLayout &pipelineLayout,
                                          const vk::PipelineDesc &pipelineDesc,
                                          const gl::AttributesMask &activeAttribLocationsMask,
                                          vk::PipelineAndSerial **pipelineOut)
{
    ASSERT(vertexShader.queueSerial() ==
           pipelineDesc.getShaderStageInfo()[vk::ShaderType::VertexShader].moduleSerial);
    ASSERT(fragmentShader.queueSerial() ==
           pipelineDesc.getShaderStageInfo()[vk::ShaderType::FragmentShader].moduleSerial);

    // Pull in a compatible RenderPass.
    vk::RenderPass *compatibleRenderPass = nullptr;
    ANGLE_TRY(getCompatibleRenderPass(pipelineDesc.getRenderPassDesc(), &compatibleRenderPass));

    return mPipelineCache.getPipeline(mDevice, *compatibleRenderPass, pipelineLayout,
                                      activeAttribLocationsMask, vertexShader.get(),
                                      fragmentShader.get(), pipelineDesc, pipelineOut);
}

vk::Error RendererVk::getDescriptorSetLayout(
    const vk::DescriptorSetLayoutDesc &desc,
    vk::BindingPointer<vk::DescriptorSetLayout> *descriptorSetLayoutOut)
{
    return mDescriptorSetLayoutCache.getDescriptorSetLayout(mDevice, desc, descriptorSetLayoutOut);
}

vk::Error RendererVk::getPipelineLayout(
    const vk::PipelineLayoutDesc &desc,
    const vk::DescriptorSetLayoutPointerArray &descriptorSetLayouts,
    vk::BindingPointer<vk::PipelineLayout> *pipelineLayoutOut)
{
    return mPipelineLayoutCache.getPipelineLayout(mDevice, desc, descriptorSetLayouts,
                                                  pipelineLayoutOut);
}

vk::ShaderLibrary *RendererVk::getShaderLibrary()
{
    return &mShaderLibrary;
}

uint32_t GetUniformBufferDescriptorCount()
{
    return kUniformBufferDescriptorsPerDescriptorSet;
}

}  // namespace rx
