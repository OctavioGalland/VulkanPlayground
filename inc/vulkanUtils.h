#ifndef _VULKAN_UTILS_H_
#define _VULKAN_UTILS_H_

#include <vulkan/vulkan.h>

#define CLAMP(x, lo, hi)    ((x) < (lo) ? (lo) : (x) > (hi) ? (hi) : (x))

#define ASSERT_RESULT(funCall, res, errorMsg) if (funCall != res) { panic(errorMsg); }

#define ENUMERATE_OBJECTS(container, funCall1, funCall2, errorMsg) \
    { \
        uint32_t size; \
        VkResult res = VK_INCOMPLETE; \
        while (res == VK_INCOMPLETE) { \
            res = funCall1; \
            if (res == VK_SUCCESS) { \
                container.resize(size); \
                res = funCall2; \
            } \
        } \
        if (res != VK_SUCCESS) { \
            panic(errorMsg); \
        } \
    }

void panic (const char *msg);
void log (const char *msg);

#endif // _VULKAN_UTILS_H_
