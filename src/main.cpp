#include <iostream>
#include <vector>
#include <cstring>

#include <vulkan/vulkan.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include "vulkanUtils.h"

std::vector<const char*> requiredDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
std::vector<const char*> requiredInstanceExtensions = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
std::vector<const char*> requiredLayers = { "VK_LAYER_KHRONOS_validation" };

VkResult createVulkanInstance(SDL_Window* window, VkInstance* res) {
    std::vector<VkLayerProperties> layerProperties;
    ENUMERATE_OBJECTS(layerProperties, vkEnumerateInstanceLayerProperties(&size, nullptr),
            vkEnumerateInstanceLayerProperties(&size, layerProperties.data()), "Failed to enumerate instance layers");

    for (const char *layerName : requiredLayers) {
        bool found = false;
        for (VkLayerProperties layerProps : layerProperties) {
            if (strcmp(layerName, layerProps.layerName) == 0) {
                found = true;
            }
        }
        if (!found) {
            panic(("Required layer \"" + std::string(layerName) + "\" not availbale!").c_str());
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
            }
        }
        if (!found) {
            panic(("Required instance extension \"" + std::string(extensionName) + "\" not availbale!").c_str());
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

int main (int argc, char *argv[]) {
    VkInstance vulkanInstance;
    SDL_Window *window = nullptr;

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        panic("Failed to initialize SDL2");
    }
    window = SDL_CreateWindow("Vulkan", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1366, 768, SDL_WINDOW_VULKAN);
    if (window == nullptr) {
        panic("Failed to create SDL2 window");
    }
    createVulkanInstance(window, &vulkanInstance);

    vkDestroyInstance(vulkanInstance, nullptr);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

