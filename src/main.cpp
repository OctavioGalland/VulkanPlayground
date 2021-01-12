#include <iostream>
#include <vector>
#include <cstring>
#include <cinttypes>

#include <vulkan/vulkan.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include "vulkanUtils.h"

std::vector<const char*> requiredDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
std::vector<const char*> requiredInstanceExtensions = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
std::vector<const char*> requiredLayers = { "VK_LAYER_KHRONOS_validation" };

VkBool32 debugCallback (VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT messageTypes,
        const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData) {
    std::cerr << callbackData->pMessage << std::endl;
    return VK_FALSE;
}

VkResult createVulkanInstance (SDL_Window* window, VkInstance* res) {
    std::vector<VkLayerProperties> layerProperties;
    ENUMERATE_OBJECTS(layerProperties, vkEnumerateInstanceLayerProperties(&size, nullptr),
            vkEnumerateInstanceLayerProperties(&size, layerProperties.data()), "Failed to enumerate instance layers");

    for (const char *layerName : requiredLayers) {
        bool found = false;
        for (VkLayerProperties layerProps : layerProperties) {
            if (strcmp(layerName, layerProps.layerName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            panic(("Required layer \"" + std::string(layerName) + "\" not availbale").c_str());
        }
    }

    std::vector<VkExtensionProperties> extProps;
    ENUMERATE_OBJECTS(extProps, vkEnumerateInstanceExtensionProperties(nullptr, &size, NULL),
            vkEnumerateInstanceExtensionProperties(nullptr, &size, extProps.data()), "Failed to enumerate instance extensions");

    std::vector<const char*> sdlRequiredInstanceExtensions;
    uint32_t size = 0;
    ASSERT_RESULT(SDL_Vulkan_GetInstanceExtensions(window, &size, nullptr), SDL_TRUE, "Failed to get number of instance extensions required by SDL2");
    sdlRequiredInstanceExtensions.resize(size);
    ASSERT_RESULT(SDL_Vulkan_GetInstanceExtensions(window, &size, sdlRequiredInstanceExtensions.data()), SDL_TRUE,
            "Failed to enumerate instance extensions required by SDL2");

    for (const char * extName : sdlRequiredInstanceExtensions) {
        requiredInstanceExtensions.push_back(extName);
    }

    for (const char *extensionName : requiredInstanceExtensions) {
        bool found = false;
        for (VkExtensionProperties extProp : extProps) {
            if (strcmp(extProp.extensionName, extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            panic(("Required instance extension \"" + std::string(extensionName) + "\" not availbale").c_str());
        }
    }

    VkApplicationInfo applicationInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = "Vulkan Test",
        .applicationVersion = 1,
        .pEngineName = "Vulkan Test",
        .engineVersion = 1,
        .apiVersion = VK_API_VERSION_1_0
    };

    VkInstanceCreateInfo instanceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pApplicationInfo = &applicationInfo,
        .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
        .ppEnabledLayerNames = requiredLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(requiredInstanceExtensions.size()),
        .ppEnabledExtensionNames = requiredInstanceExtensions.data()
    };

    return vkCreateInstance(&instanceCreateInfo, nullptr, res);
}

VkResult createDebugMessenger (const VkInstance& instance, PFN_vkDebugUtilsMessengerCallbackEXT callback, VkDebugUtilsMessengerEXT* debugMessenger) {
    VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext = nullptr,
        .flags = 0,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = callback,
        .pUserData = nullptr
    };

    VkResult (*vkCreateDebugUtilsMessengerEXT)(VkInstance, const VkDebugUtilsMessengerCreateInfoEXT*, const VkAllocationCallbacks*, VkDebugUtilsMessengerEXT*) = 
        (VkResult (*)(VkInstance, const VkDebugUtilsMessengerCreateInfoEXT*, const VkAllocationCallbacks*, VkDebugUtilsMessengerEXT*))
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

    if (vkCreateDebugUtilsMessengerEXT == nullptr) {
        panic("Failed to get function pointer for vkCreateDebugUtilsMessengerEXT");
    }

    return vkCreateDebugUtilsMessengerEXT(instance, &debugMessengerCreateInfo, nullptr, debugMessenger);
}

void destroyDebugMessenger (const VkInstance& instance, const VkDebugUtilsMessengerEXT& messenger) {
    void (*vkDestroyDebugUtilsMessengerEXT)(VkInstance, VkDebugUtilsMessengerEXT, const VkAllocationCallbacks*) = 
        (void (*)(VkInstance, VkDebugUtilsMessengerEXT, const VkAllocationCallbacks*)) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

    if (vkDestroyDebugUtilsMessengerEXT == nullptr) {
        panic("Failed to get function pointer for vkCreateDebugUtilsMessengerEXT");
    }

    vkDestroyDebugUtilsMessengerEXT(instance, messenger, nullptr);
}

void pickPhysicalDevice (const VkInstance& instance, const VkSurfaceKHR& surface, VkPhysicalDevice *physicalDevice,
        uint32_t& graphicsQueueIndex, uint32_t& presentQueueIndex) {
    std::vector<VkPhysicalDevice> devices;
    ENUMERATE_OBJECTS(devices, vkEnumeratePhysicalDevices(instance, &size, NULL), vkEnumeratePhysicalDevices(instance, &size, devices.data()),
            "Failed to enumerate physical devices");

    uint32_t maxScore = 0;
    for (VkPhysicalDevice device : devices) {
        bool suitable = true;
        uint32_t graphicsFamilyIndex = 0, presentFamilyIndex = 0;
        bool supportsGraphics = false, supportsPresent = false;
        uint32_t queueFaimilyCount = 0;
        std::vector<VkQueueFamilyProperties> queueFamilyProperties;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFaimilyCount, NULL);
        queueFamilyProperties.resize(queueFaimilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFaimilyCount, queueFamilyProperties.data());

        for (uint32_t i = 0; i < queueFamilyProperties.size(); i++) {
            VkQueueFamilyProperties familyProperties = queueFamilyProperties[i];
            if (!supportsGraphics && (familyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
                graphicsFamilyIndex = i;
                supportsGraphics = true;
            }
            VkBool32 wsiSupported = VK_FALSE;
            ASSERT_RESULT(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &wsiSupported), VK_SUCCESS, "Failed to query queue family for WSI support");
            if (!supportsPresent && wsiSupported == VK_TRUE) {
                presentFamilyIndex = i;
                supportsPresent = true;
            }
        }

        if (!supportsGraphics || !supportsPresent) {
            suitable = false;
        }

        std::vector<VkLayerProperties> deviceLayers;
        ENUMERATE_OBJECTS(deviceLayers, vkEnumerateDeviceLayerProperties(device, &size, nullptr),
                vkEnumerateDeviceLayerProperties(device, &size, deviceLayers.data()), "Failed to enumerate device layers");
        for (const char *layer : requiredLayers) {
            bool found = false;
            for (VkLayerProperties layerProps : deviceLayers) {
                if (strcmp(layer, layerProps.layerName) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                suitable = false;
            }
        }

        std::vector<VkExtensionProperties> deviceExtensions;
        ENUMERATE_OBJECTS(deviceExtensions, vkEnumerateDeviceExtensionProperties(device, nullptr, &size, nullptr),
                vkEnumerateDeviceExtensionProperties(device, nullptr, &size, deviceExtensions.data()), "Failed to enumerate device extensions");
        for (const char* extension : requiredDeviceExtensions) {
            bool found = false;
            for (VkExtensionProperties extProps : deviceExtensions) {
                if (strcmp(extProps.extensionName, extension) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                suitable = false;
            }
        }

        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &capabilities);
        if ((capabilities.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) == 0) {
            suitable = false;
        }

        if (suitable) {
            uint32_t score = 1;
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(device, &properties);
            if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                score += 100;
            }
            if (score > maxScore) {
                maxScore = score;
                graphicsQueueIndex = graphicsFamilyIndex;
                presentQueueIndex = presentFamilyIndex;
                (*physicalDevice) = device;
            }
        }
    }
    if (maxScore == 0) {
        panic("No suitable GPU found");
    }
}

VkResult createLogicalDevice (const VkPhysicalDevice& physicalDevice, VkDevice* device, uint32_t graphicsQueueIndex, uint32_t presentQueueIndex) {
    float one =  1.0f;
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos {
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueFamilyIndex = graphicsQueueIndex,
            .queueCount = 1,
            .pQueuePriorities = &one
        }
    };
    if (graphicsQueueIndex != presentQueueIndex) {
        queueCreateInfos.push_back({
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueFamilyIndex = presentQueueIndex,
            .queueCount = 1,
            .pQueuePriorities = &one
        });
    }
    VkDeviceCreateInfo deviceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
        .ppEnabledLayerNames = requiredLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size()),
        .ppEnabledExtensionNames = requiredDeviceExtensions.data(),
        .pEnabledFeatures = NULL
    };

    return vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, device);
}

void retrieveQueues(const VkDevice& device, uint32_t graphicsQueueIndex, uint32_t presentQueueIndex, VkQueue *graphicsQueue, VkQueue *presentQueue) {
    vkGetDeviceQueue(device, graphicsQueueIndex, 0, graphicsQueue);
    if (graphicsQueueIndex == presentQueueIndex) {
        (*presentQueue) = (*graphicsQueue);
    } else {
        vkGetDeviceQueue(device, presentQueueIndex, 0, presentQueue);
    }
}

VkResult createSwapchain (const VkPhysicalDevice& physicalDevice, const VkDevice& device, const VkSurfaceKHR& surface, SDL_Window* window,
        uint32_t graphicsQueueIndex, uint32_t presentQueueIndex, VkSwapchainKHR *swapchain) {
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    VkExtent2D extent;
    VkFormat format;
    VkColorSpaceKHR colorSpace;
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);

    if ((surfaceCapabilities.supportedCompositeAlpha & compositeAlpha) == 0) {
        if ((surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR) != 0)
            compositeAlpha = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
        if ((surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR) != 0)
            compositeAlpha = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
        if ((surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) != 0)
            compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    }

    std::vector<VkSurfaceFormatKHR> supportedSurfaceFormats;
    ENUMERATE_OBJECTS(supportedSurfaceFormats, vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &size, nullptr),
            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &size, supportedSurfaceFormats.data()), "Failed to enumerate supported surface formats");
    bool preferedSurfaceFormatFound = false;
    for (VkSurfaceFormatKHR surfaceFormat : supportedSurfaceFormats) {
        if (surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB) {
            format = surfaceFormat.format;
            colorSpace = surfaceFormat.colorSpace;
            preferedSurfaceFormatFound = true;
            break;
        }
    }
    if (!preferedSurfaceFormatFound) {
        log("Prefered surface format could not be found, defaulting to first available surface format (colors might be inaccurate)");
        format = supportedSurfaceFormats[0].format;
        colorSpace = supportedSurfaceFormats[0].colorSpace;
    }

    std::vector<VkPresentModeKHR> supportedPresentModes;
    ENUMERATE_OBJECTS(supportedPresentModes, vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &size, nullptr),
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &size, supportedPresentModes.data()), "Failed to enumerate supported present modes");
    for (VkPresentModeKHR mode : supportedPresentModes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = mode;
            break;
        }
    }

    int winWidth = 0, winHeight = 0;
    SDL_Vulkan_GetDrawableSize(window, &winWidth, &winHeight);
    extent.width = CLAMP(static_cast<uint32_t>(winWidth), surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
    extent.height = CLAMP(static_cast<uint32_t>(winWidth), surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);

    uint32_t queues[] = { graphicsQueueIndex, presentQueueIndex };

    VkSwapchainCreateInfoKHR swapchainCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .surface = surface,
        .minImageCount = surfaceCapabilities.minImageCount,
        .imageFormat = format,
        .imageColorSpace = colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = (graphicsQueueIndex == presentQueueIndex ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT),
        .queueFamilyIndexCount = 2,
        .pQueueFamilyIndices = queues,
        .preTransform = surfaceCapabilities.currentTransform,
        .compositeAlpha = compositeAlpha,
        .presentMode = presentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    };

    return vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, swapchain);
}

int main (int argc, char *argv[]) {
    SDL_Window *window = nullptr;
    VkInstance vulkanInstance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalDevice;
    VkSurfaceKHR vulkanSurface;
    uint32_t graphicsQueueIndex = 0, presentQueueIndex = 0;
    VkDevice vulkanDevice;
    VkQueue graphicsQueue, presentQueue;
    VkSwapchainKHR swapchain;
    std::vector<VkImage> swapchainImages;
    VkFence swapchainImageFence;
    uint32_t swapchainImageIndex;

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        panic("Failed to initialize SDL2");
    }
    window = SDL_CreateWindow("Vulkan", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1366, 768, SDL_WINDOW_VULKAN);
    if (window == nullptr) {
        panic("Failed to create SDL2 window");
    }
    ASSERT_RESULT(createVulkanInstance(window, &vulkanInstance), VK_SUCCESS, "Failed to create vulkan instance");
    if (SDL_Vulkan_CreateSurface(window, vulkanInstance, &vulkanSurface) != SDL_TRUE) {
        panic("Failed to create Vulkan surface");
    }
    ASSERT_RESULT(createDebugMessenger(vulkanInstance, debugCallback, &debugMessenger), VK_SUCCESS, "Falied to create debug messenger");
    pickPhysicalDevice(vulkanInstance, vulkanSurface, &physicalDevice, graphicsQueueIndex, presentQueueIndex);
    ASSERT_RESULT(createLogicalDevice(physicalDevice, &vulkanDevice, graphicsQueueIndex, presentQueueIndex), VK_SUCCESS, "Failed to create Logical device");
    retrieveQueues(vulkanDevice, graphicsQueueIndex, presentQueueIndex, &graphicsQueue, &presentQueue);
    ASSERT_RESULT(createSwapchain(physicalDevice, vulkanDevice, vulkanSurface, window, graphicsQueueIndex, presentQueueIndex, &swapchain), VK_SUCCESS,
            "Failed to create swapchain");

    ENUMERATE_OBJECTS(swapchainImages, vkGetSwapchainImagesKHR(vulkanDevice, swapchain, &size, nullptr), vkGetSwapchainImagesKHR(vulkanDevice,swapchain, &size, swapchainImages.data()), "Failed to get swap chain images");

    VkFenceCreateInfo fenceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0
    };
    ASSERT_RESULT(vkCreateFence(vulkanDevice, &fenceCreateInfo, nullptr, &swapchainImageFence), VK_SUCCESS, "Error creating Vulkan fence");

    bool running = true;
    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev) != 0) {
            switch (ev.type) {
                case SDL_QUIT:
                    running = 0;
                    break;
                default:
                    break;
            }
        }

    }

    vkDestroyFence(vulkanDevice, swapchainImageFence, nullptr);
    vkDestroySwapchainKHR(vulkanDevice, swapchain, nullptr);
    vkDestroyDevice(vulkanDevice, nullptr);
    destroyDebugMessenger(vulkanInstance, debugMessenger);
    vkDestroySurfaceKHR(vulkanInstance, vulkanSurface, nullptr);
    vkDestroyInstance(vulkanInstance, nullptr);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

