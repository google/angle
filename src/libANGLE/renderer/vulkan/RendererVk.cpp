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
#include "libANGLE/renderer/vulkan/renderervk_utils.h"

#include <EGL/eglext.h>

#include "common/debug.h"
#include "common/system_utils.h"
#include "libANGLE/renderer/driver_utils.h"
#include "libANGLE/renderer/vulkan/CommandBufferNode.h"
#include "libANGLE/renderer/vulkan/CompilerVk.h"
#include "libANGLE/renderer/vulkan/FramebufferVk.h"
#include "libANGLE/renderer/vulkan/GlslangWrapper.h"
#include "libANGLE/renderer/vulkan/TextureVk.h"
#include "libANGLE/renderer/vulkan/VertexArrayVk.h"
#include "libANGLE/renderer/vulkan/formatutilsvk.h"
#include "platform/Platform.h"

namespace rx
{

namespace
{

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

VkBool32 VKAPI_CALL DebugReportCallback(VkDebugReportFlagsEXT flags,
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

}  // anonymous namespace

// RenderPassCache implementation.
RenderPassCache::RenderPassCache()
{
}

RenderPassCache::~RenderPassCache()
{
    ASSERT(mPayload.empty());
}

void RenderPassCache::destroy(VkDevice device)
{
    for (auto &outerIt : mPayload)
    {
        for (auto &innerIt : outerIt.second)
        {
            innerIt.second.get().destroy(device);
        }
    }
    mPayload.clear();
}

vk::Error RenderPassCache::getCompatibleRenderPass(VkDevice device,
                                                   Serial serial,
                                                   const vk::RenderPassDesc &desc,
                                                   vk::RenderPass **renderPassOut)
{
    auto outerIt = mPayload.find(desc);
    if (outerIt != mPayload.end())
    {
        InnerCache &innerCache = outerIt->second;
        ASSERT(!innerCache.empty());

        // Find the first element and return it.
        *renderPassOut = &innerCache.begin()->second.get();
        return vk::NoError();
    }

    // Insert some dummy attachment ops.
    // TODO(jmadill): Pre-populate the cache in the Renderer so we rarely miss here.
    vk::AttachmentOpsArray ops;
    for (uint32_t colorIndex = 0; colorIndex < desc.colorAttachmentCount(); ++colorIndex)
    {
        ops.initDummyOp(colorIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }

    if (desc.depthStencilAttachmentCount() > 0)
    {
        ops.initDummyOp(desc.colorAttachmentCount(),
                        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    }

    return getRenderPassWithOps(device, serial, desc, ops, renderPassOut);
}

vk::Error RenderPassCache::getRenderPassWithOps(VkDevice device,
                                                Serial serial,
                                                const vk::RenderPassDesc &desc,
                                                const vk::AttachmentOpsArray &attachmentOps,
                                                vk::RenderPass **renderPassOut)
{
    auto outerIt = mPayload.find(desc);
    if (outerIt != mPayload.end())
    {
        InnerCache &innerCache = outerIt->second;

        auto innerIt = innerCache.find(attachmentOps);
        if (innerIt != innerCache.end())
        {
            // Update the serial before we return.
            // TODO(jmadill): Could possibly use an MRU cache here.
            innerIt->second.updateSerial(serial);
            *renderPassOut = &innerIt->second.get();
            return vk::NoError();
        }
    }
    else
    {
        auto emplaceResult = mPayload.emplace(desc, InnerCache());
        outerIt            = emplaceResult.first;
    }

    vk::RenderPass newRenderPass;
    ANGLE_TRY(vk::InitializeRenderPassFromDesc(device, desc, attachmentOps, &newRenderPass));

    vk::RenderPassAndSerial withSerial(std::move(newRenderPass), serial);

    InnerCache &innerCache = outerIt->second;
    auto insertPos         = innerCache.emplace(attachmentOps, std::move(withSerial));
    *renderPassOut = &insertPos.first->second.get();

    // TODO(jmadill): Trim cache, and pre-populate with the most common RPs on startup.
    return vk::NoError();
}

// CommandBatch implementation.
RendererVk::CommandBatch::CommandBatch()
{
}

RendererVk::CommandBatch::~CommandBatch()
{
}

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
      mCurrentQueueSerial(mQueueSerialFactory.generate()),
      mInFlightCommands()
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

    for (auto &descriptorSetLayout : mGraphicsDescriptorSetLayouts)
    {
        descriptorSetLayout.destroy(mDevice);
    }

    mGraphicsPipelineLayout.destroy(mDevice);

    mRenderPassCache.destroy(mDevice);

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

vk::Error RendererVk::initialize(const egl::AttributeMap &attribs, const char *wsiName)
{
    mEnableValidationLayers = ShouldUseDebugLayers(attribs);

    // If we're loading the validation layers, we could be running from any random directory.
    // Change to the executable directory so we can find the layers, then change back to the
    // previous directory to be safe we don't disrupt the application.
    std::string previousCWD;

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
            previousCWD = cwd.value();
            const char *exeDir = angle::GetExecutableDirectory();
            if (!angle::SetCWD(exeDir))
            {
                ERR() << "Error setting CWD for Vulkan layers init.";
                mEnableValidationLayers = false;
            }
        }
    }

    // Override environment variable to use the ANGLE layers.
    if (mEnableValidationLayers)
    {
        if (!angle::SetEnvironmentVar(g_VkLoaderLayersPathEnv, ANGLE_VK_LAYERS_DIR))
        {
            ERR() << "Error setting environment for Vulkan layers init.";
            mEnableValidationLayers = false;
        }
    }

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

    if (mEnableValidationLayers)
    {
        // Verify the standard validation layers are available.
        if (!HasStandardValidationLayer(instanceLayerProps))
        {
            // Generate an error if the attribute was requested, warning otherwise.
            if (attribs.get(EGL_PLATFORM_ANGLE_DEBUG_LAYERS_ENABLED_ANGLE, EGL_DONT_CARE) ==
                EGL_TRUE)
            {
                ERR() << "Vulkan standard validation layers are missing.";
            }
            else
            {
                WARN() << "Vulkan standard validation layers are missing.";
            }
            mEnableValidationLayers = false;
        }
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
    instanceInfo.enabledLayerCount = mEnableValidationLayers ? 1u : 0u;
    instanceInfo.ppEnabledLayerNames =
        mEnableValidationLayers ? &g_VkStdValidationLayerName : nullptr;

    ANGLE_VK_TRY(vkCreateInstance(&instanceInfo, nullptr, &mInstance));

    if (mEnableValidationLayers)
    {
        // Change back to the previous working directory now that we've loaded the instance -
        // the validation layers should be loaded at this point.
        angle::SetCWD(previousCWD.c_str());

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
    physicalDeviceCount = 1;
    ANGLE_VK_TRY(vkEnumeratePhysicalDevices(mInstance, &physicalDeviceCount, &mPhysicalDevice));

    vkGetPhysicalDeviceProperties(mPhysicalDevice, &mPhysicalDeviceProperties);

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
    mFormatTable.initialize(mPhysicalDevice, &mNativeTextureCaps);

    // Initialize the pipeline layout for GL programs.
    ANGLE_TRY(initGraphicsPipelineLayout());

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

    if (mEnableValidationLayers)
    {
        if (!HasStandardValidationLayer(deviceLayerProps))
        {
            WARN() << "Vulkan standard validation layer is missing.";
            mEnableValidationLayers = false;
        }
    }

    std::vector<const char *> enabledDeviceExtensions;
    enabledDeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

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

    createInfo.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext                = nullptr;
    createInfo.flags                = 0;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos    = &queueCreateInfo;
    createInfo.enabledLayerCount    = mEnableValidationLayers ? 1u : 0u;
    createInfo.ppEnabledLayerNames =
        mEnableValidationLayers ? &g_VkStdValidationLayerName : nullptr;
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

vk::ErrorOrResult<uint32_t> RendererVk::selectPresentQueueForSurface(VkSurfaceKHR surface)
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

        return (supportsPresent == VK_TRUE);
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

    return newPresentQueue.value();
}

std::string RendererVk::getVendorString() const
{
    switch (mPhysicalDeviceProperties.vendorID)
    {
        case VENDOR_ID_AMD:
            return "Advanced Micro Devices";
        case VENDOR_ID_NVIDIA:
            return "NVIDIA";
        case VENDOR_ID_INTEL:
            return "Intel";
        default:
        {
            // TODO(jmadill): More vendor IDs.
            std::stringstream strstr;
            strstr << "Vendor ID: " << mPhysicalDeviceProperties.vendorID;
            return strstr.str();
        }
    }
}

std::string RendererVk::getRendererDescription() const
{
    std::stringstream strstr;

    uint32_t apiVersion = mPhysicalDeviceProperties.apiVersion;

    strstr << "Vulkan ";
    strstr << VK_VERSION_MAJOR(apiVersion) << ".";
    strstr << VK_VERSION_MINOR(apiVersion) << ".";
    strstr << VK_VERSION_PATCH(apiVersion);

    strstr << "(" << mPhysicalDeviceProperties.deviceName << ")";

    return strstr.str();
}

void RendererVk::ensureCapsInitialized() const
{
    if (!mCapsInitialized)
    {
        generateCaps(&mNativeCaps, &mNativeTextureCaps, &mNativeExtensions, &mNativeLimitations);
        mCapsInitialized = true;
    }
}

void RendererVk::generateCaps(gl::Caps *outCaps,
                              gl::TextureCapsMap * /*outTextureCaps*/,
                              gl::Extensions *outExtensions,
                              gl::Limitations * /* outLimitations */) const
{
    // TODO(jmadill): Caps.
    outCaps->maxDrawBuffers      = 1;
    outCaps->maxVertexAttributes     = gl::MAX_VERTEX_ATTRIBS;
    outCaps->maxVertexAttribBindings = gl::MAX_VERTEX_ATTRIB_BINDINGS;
    outCaps->maxVaryingVectors            = 16;
    outCaps->maxTextureImageUnits         = 1;
    outCaps->maxCombinedTextureImageUnits = 1;
    outCaps->max2DTextureSize             = 1024;
    outCaps->maxElementIndex              = std::numeric_limits<GLuint>::max() - 1;
    outCaps->maxFragmentUniformVectors    = 8;
    outCaps->maxVertexUniformVectors      = 8;
    outCaps->maxColorAttachments          = 1;

    // Enable this for simple buffer readback testing, but some functionality is missing.
    // TODO(jmadill): Support full mapBufferRange extension.
    outExtensions->mapBuffer      = true;
    outExtensions->mapBufferRange = true;
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

const vk::CommandPool &RendererVk::getCommandPool() const
{
    return mCommandPool;
}

vk::Error RendererVk::finish(const gl::Context *context)
{
    if (!mOpenCommandGraph.empty())
    {
        vk::CommandBuffer commandBatch;
        ANGLE_TRY(flushCommandGraph(context, &commandBatch));

        VkSubmitInfo submitInfo;
        submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext                = nullptr;
        submitInfo.waitSemaphoreCount   = 0;
        submitInfo.pWaitSemaphores      = nullptr;
        submitInfo.pWaitDstStageMask    = nullptr;
        submitInfo.commandBufferCount   = 1;
        submitInfo.pCommandBuffers      = commandBatch.ptr();
        submitInfo.signalSemaphoreCount = 0;
        submitInfo.pSignalSemaphores    = nullptr;

        ANGLE_TRY(submitFrame(submitInfo, std::move(commandBatch)));
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

    CommandBatch batch;
    ANGLE_TRY(batch.fence.init(mDevice, fenceInfo));

    ANGLE_VK_TRY(vkQueueSubmit(mQueue, 1, &submitInfo, batch.fence.getHandle()));

    // Store this command buffer in the in-flight list.
    batch.commandPool = std::move(mCommandPool);
    batch.serial      = mCurrentQueueSerial;

    mInFlightCommands.emplace_back(std::move(batch));

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

    mCommandPool.init(mDevice, poolInfo);

    return vk::NoError();
}

vk::Error RendererVk::createStagingImage(TextureDimension dimension,
                                         const vk::Format &format,
                                         const gl::Extents &extent,
                                         vk::StagingUsage usage,
                                         vk::StagingImage *imageOut)
{
    ANGLE_TRY(imageOut->init(mDevice, mCurrentQueueFamilyIndex, mMemoryProperties, dimension,
                             format.vkTextureFormat, extent, usage));
    return vk::NoError();
}

GlslangWrapper *RendererVk::getGlslangWrapper()
{
    return mGlslangWrapper;
}

Serial RendererVk::getCurrentQueueSerial() const
{
    return mCurrentQueueSerial;
}

bool RendererVk::isResourceInUse(const ResourceVk &resource)
{
    return isSerialInUse(resource.getQueueSerial());
}

bool RendererVk::isSerialInUse(Serial serial)
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

vk::CommandBufferNode *RendererVk::allocateCommandNode()
{
    // TODO(jmadill): Use a pool allocator for the CPU node allocations.
    vk::CommandBufferNode *newCommands = new vk::CommandBufferNode();
    mOpenCommandGraph.emplace_back(newCommands);
    return newCommands;
}

vk::Error RendererVk::flushCommandGraph(const gl::Context *context, vk::CommandBuffer *commandBatch)
{
    VkCommandBufferAllocateInfo primaryInfo;
    primaryInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    primaryInfo.pNext              = nullptr;
    primaryInfo.commandPool        = mCommandPool.getHandle();
    primaryInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    primaryInfo.commandBufferCount = 1;

    ANGLE_TRY(commandBatch->init(mDevice, primaryInfo));

    if (mOpenCommandGraph.empty())
    {
        return vk::NoError();
    }

    std::vector<vk::CommandBufferNode *> nodeStack;

    VkCommandBufferBeginInfo beginInfo;
    beginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.pNext            = nullptr;
    beginInfo.flags            = 0;
    beginInfo.pInheritanceInfo = nullptr;

    ANGLE_TRY(commandBatch->begin(beginInfo));

    for (vk::CommandBufferNode *topLevelNode : mOpenCommandGraph)
    {
        // Only process commands that don't have child commands. The others will be pulled in
        // automatically. Also skip commands that have already been visited.
        if (topLevelNode->isDependency() ||
            topLevelNode->visitedState() != vk::VisitedState::Unvisited)
            continue;

        nodeStack.push_back(topLevelNode);

        while (!nodeStack.empty())
        {
            vk::CommandBufferNode *node = nodeStack.back();

            switch (node->visitedState())
            {
                case vk::VisitedState::Unvisited:
                    node->visitDependencies(&nodeStack);
                    break;
                case vk::VisitedState::Ready:
                    ANGLE_TRY(node->visitAndExecute(this, commandBatch));
                    nodeStack.pop_back();
                    break;
                case vk::VisitedState::Visited:
                    nodeStack.pop_back();
                    break;
                default:
                    UNREACHABLE();
                    break;
            }
        }
    }

    ANGLE_TRY(commandBatch->end());
    resetCommandGraph();
    return vk::NoError();
}

void RendererVk::resetCommandGraph()
{
    // TODO(jmadill): Use pool allocation so we don't need to deallocate command graph.
    for (vk::CommandBufferNode *node : mOpenCommandGraph)
    {
        delete node;
    }
    mOpenCommandGraph.clear();
}

vk::Error RendererVk::flush(const gl::Context *context,
                            const vk::Semaphore &waitSemaphore,
                            const vk::Semaphore &signalSemaphore)
{
    vk::CommandBuffer commandBatch;
    ANGLE_TRY(flushCommandGraph(context, &commandBatch));

    VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

    VkSubmitInfo submitInfo;
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext                = nullptr;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = waitSemaphore.ptr();
    submitInfo.pWaitDstStageMask    = &waitStageMask;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = commandBatch.ptr();
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = signalSemaphore.ptr();

    ANGLE_TRY(submitFrame(submitInfo, std::move(commandBatch)));
    return vk::NoError();
}

const vk::PipelineLayout &RendererVk::getGraphicsPipelineLayout() const
{
    return mGraphicsPipelineLayout;
}

const std::vector<vk::DescriptorSetLayout> &RendererVk::getGraphicsDescriptorSetLayouts() const
{
    return mGraphicsDescriptorSetLayouts;
}

vk::Error RendererVk::initGraphicsPipelineLayout()
{
    ASSERT(!mGraphicsPipelineLayout.valid());

    // Create two descriptor set layouts: one for default uniform info, and one for textures.
    // Skip one or both if there are no uniforms.
    VkDescriptorSetLayoutBinding uniformBindings[2];
    uint32_t blockCount = 0;

    {
        auto &layoutBinding = uniformBindings[blockCount];

        layoutBinding.binding            = blockCount;
        layoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        layoutBinding.descriptorCount    = 1;
        layoutBinding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
        layoutBinding.pImmutableSamplers = nullptr;

        blockCount++;
    }

    {
        auto &layoutBinding = uniformBindings[blockCount];

        layoutBinding.binding            = blockCount;
        layoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        layoutBinding.descriptorCount    = 1;
        layoutBinding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
        layoutBinding.pImmutableSamplers = nullptr;

        blockCount++;
    }

    {
        VkDescriptorSetLayoutCreateInfo uniformInfo;
        uniformInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        uniformInfo.pNext        = nullptr;
        uniformInfo.flags        = 0;
        uniformInfo.bindingCount = blockCount;
        uniformInfo.pBindings    = uniformBindings;

        vk::DescriptorSetLayout uniformLayout;
        ANGLE_TRY(uniformLayout.init(mDevice, uniformInfo));
        mGraphicsDescriptorSetLayouts.push_back(std::move(uniformLayout));
    }

    std::array<VkDescriptorSetLayoutBinding, gl::IMPLEMENTATION_MAX_ACTIVE_TEXTURES>
        textureBindings;

    // TODO(jmadill): This approach might not work well for texture arrays.
    for (uint32_t textureIndex = 0; textureIndex < gl::IMPLEMENTATION_MAX_ACTIVE_TEXTURES;
         ++textureIndex)
    {
        VkDescriptorSetLayoutBinding &layoutBinding = textureBindings[textureIndex];

        layoutBinding.binding         = textureIndex;
        layoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags      = (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
        layoutBinding.pImmutableSamplers = nullptr;
    }

    {
        VkDescriptorSetLayoutCreateInfo textureInfo;
        textureInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        textureInfo.pNext        = nullptr;
        textureInfo.flags        = 0;
        textureInfo.bindingCount = static_cast<uint32_t>(textureBindings.size());
        textureInfo.pBindings    = textureBindings.data();

        vk::DescriptorSetLayout textureLayout;
        ANGLE_TRY(textureLayout.init(mDevice, textureInfo));
        mGraphicsDescriptorSetLayouts.push_back(std::move(textureLayout));
    }

    VkPipelineLayoutCreateInfo createInfo;
    createInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    createInfo.pNext                  = nullptr;
    createInfo.flags                  = 0;
    createInfo.setLayoutCount         = static_cast<uint32_t>(mGraphicsDescriptorSetLayouts.size());
    createInfo.pSetLayouts            = mGraphicsDescriptorSetLayouts[0].ptr();
    createInfo.pushConstantRangeCount = 0;
    createInfo.pPushConstantRanges    = nullptr;

    ANGLE_TRY(mGraphicsPipelineLayout.init(mDevice, createInfo));

    return vk::NoError();
}
}  // namespace rx
