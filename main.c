
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


//heler function for validation layers
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* userData)
{
    fprintf(stderr, "validation: %s\n", data->pMessage);
    return VK_FALSE;
}

//validation layers for vulkan
const char* validationLayers[] = {
    "VK_LAYER_KHRONOS_validation"
};
const uint32_t validationLayerCount = 1;

//extensions for swapchain
const char* deviceExtensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};


//for the physical device / logical device steps
typedef struct {
    uint32_t graphicsFamily;
    uint32_t presentFamily;
    int foundGraphics;
    int foundPresent;
} QueueFamilyIndices;




int main(){
//STEP 1; BUILD FOUNDATION
    //set up glfw (window)
    if (!glfwInit()){
	fprintf(stderr, "glfwInit() failed :<\n");
	return 1;
    }//if no glfw, terminate and exit

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan", NULL, NULL);

    if (!window){
	fprintf(stderr, "window creation failed :<\n");
	glfwTerminate();
	return 1;
    }//if no window, terminate glfw and exit

    //.01 fill in application info (mostly optional, but conventionally provided)
    VkApplicationInfo appInfo = {0};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;


    //.02 gather GLFW required extensions
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    //.03 build combined extension array: GLFW's + debug utils
    const char* myExtensions[16];
    uint32_t myExtensionCount = 0;
    for (uint32_t i = 0; i < glfwExtensionCount; i++) {
        myExtensions[myExtensionCount++] = glfwExtensions[i];
    }
    myExtensions[myExtensionCount++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

    //.04 set up debug messenger create info (chained via pNext)
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {0};
    debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCreateInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugCreateInfo.pfnUserCallback = debugCallback;

    //.05 fill in instance create info
    VkInstanceCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pNext = &debugCreateInfo;              // chain the debug messenger
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledLayerCount = validationLayerCount;
    createInfo.ppEnabledLayerNames = validationLayers;
    createInfo.enabledExtensionCount = myExtensionCount;
    createInfo.ppEnabledExtensionNames = myExtensions;

    //.1 create instance:
	//instance is your projects access point to vulkan
    VkInstance instance;

    VkResult result1 = vkCreateInstance(&createInfo, NULL, &instance);

    if (result1 != VK_SUCCESS) {
	fprintf(stderr, "vkCreateInstance() failed: %d\n", result1);
	glfwDestroyWindow(window);
	glfwTerminate();
	return 1;
    }
    fprintf(stderr, "*\n");

    //.2 create surface
        //is independent of os, a wrapper that picks the right 
        //platform extension for your instance 
    VkSurfaceKHR surface;

    VkResult result2 = glfwCreateWindowSurface(instance, window, NULL, &surface);
    if (result2 != VK_SUCCESS) {
	fprintf(stderr, "vkCreateWindowSurface() failed: %d\n", result2);
	glfwDestroyWindow(window);
	glfwTerminate();
	return 1;
    }

    fprintf(stderr, "**\n");

    //.3 create physical device:
        //picking physical suitable for your specific window surface
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, NULL);
    if (device_count == 0) {
        fprintf(stderr, "no vulkan capable GPUs found\n");
        return 1;
    }//.31 check if GPUs are vulkan compatible

    VkPhysicalDevice* devices = malloc(sizeof(VkPhysicalDevice) * device_count);
    vkEnumeratePhysicalDevices(instance, &device_count, devices);

    VkPhysicalDevice physical = VK_NULL_HANDLE;

    // these two track the winning GPU's queue families
    uint32_t graphicsFamily = 0;
    uint32_t presentFamily  = 0;
    int foundGraphics = 0;
    int foundPresent  = 0;

    //.32 get compatible GPU(s)
    for (uint32_t i = 0; i < device_count; i++) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(devices[i], &props);

        uint32_t familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &familyCount, NULL);

        VkQueueFamilyProperties* families = malloc(sizeof(VkQueueFamilyProperties) * familyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &familyCount, families);

        // local search results for *this* GPU
        uint32_t gFamily = 0, pFamily = 0;
        int gFound = 0, pFound = 0;

        for (uint32_t j = 0; j < familyCount; j++) {
            // graphics?
            if ((families[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) && !gFound) {
                gFamily = j;
                gFound = 1;
            }
            // present?
            VkBool32 presentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(devices[i], j, surface, &presentSupport);
            if (presentSupport && !pFound) {
                pFamily = j;
                pFound = 1;
            }
            if (gFound && pFound) break;
        }

        free(families);

        // this GPU works only if it found *both* families
        if (gFound && pFound) {
            fprintf(stderr, "picked GPU: %s (graphics=%u present=%u)\n",
                    props.deviceName, gFamily, pFamily);
            physical        = devices[i];
            graphicsFamily  = gFamily;
            presentFamily   = pFamily;
            foundGraphics   = gFound;
            foundPresent    = pFound;
            break;
        }
    }

    free(devices);

    //.33 checking again
    if (physical == VK_NULL_HANDLE) {
        fprintf(stderr, "pickPhysicalDevice() failed\n");
        vkDestroySurfaceKHR(instance, surface, NULL);
        vkDestroyInstance(instance, NULL);
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    fprintf(stderr, "***\n");

    //.4 create logical device:
        //interface to the physical device, buffers, pipelines, 
	//and queues go through this
    // build a de-duplicated list of queue families we want queues from
    uint32_t uniqueFamilies[2];
    uint32_t uniqueCount = 0;

    uniqueFamilies[uniqueCount++] = graphicsFamily;
    if (presentFamily != graphicsFamily) {
        uniqueFamilies[uniqueCount++] = presentFamily;
    }

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueInfos[2] = {0};
    for (uint32_t i = 0; i < uniqueCount; i++) {
        queueInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfos[i].queueFamilyIndex = uniqueFamilies[i];
        queueInfos[i].queueCount = 1;
        queueInfos[i].pQueuePriorities = &queuePriority;
    }

    VkPhysicalDeviceFeatures deviceFeatures = {0};   // none for now

    VkDeviceCreateInfo deviceInfo = {0};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.queueCreateInfoCount = uniqueCount;
    deviceInfo.pQueueCreateInfos = queueInfos;
    deviceInfo.pEnabledFeatures = &deviceFeatures;
    deviceInfo.enabledExtensionCount = 1;
    deviceInfo.ppEnabledExtensionNames = deviceExtensions; // necessary for swapchain
    deviceInfo.enabledLayerCount = 0;
    deviceInfo.ppEnabledLayerNames = NULL;

    VkDevice device;
    VkResult deviceResult = vkCreateDevice(physical, &deviceInfo, NULL, &device);
    if (deviceResult != VK_SUCCESS) {
        fprintf(stderr, "vkCreateDevice() failed: %d\n", deviceResult);
        vkDestroySurfaceKHR(instance, surface, NULL);
        vkDestroyInstance(instance, NULL);
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    fprintf(stderr, "****\n");

    //.5 create queues
        //graphics queue: channel for draw/rendering commands
        //present  queue: channel to submit "show present image on screen"
    VkQueue graphicsQueue, presentQueue;
    vkGetDeviceQueue(device, graphicsFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(device, presentFamily,  0, &presentQueue);

    fprintf(stderr, "*****\n");

    //here you can double check that everything works
//VERIFICATION HERE
    //and now we know that everything works


    //.6 create swapchain:
        //swapchain is a queue of images that the gpu renders into

    //.61 first query the surface's capabilities (min/max image count, extent, transform, etc)
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical, surface, &caps);

   //.62 pick a (format, colorSpace) pair the surface supports
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &formatCount, NULL);
    if (formatCount == 0) {
        fprintf(stderr, "no surface formats\n");
        return 1;
    }

    VkSurfaceFormatKHR* formats = malloc(formatCount * sizeof(*formats));
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &formatCount, formats);

       VkSurfaceFormatKHR chosenFormat = formats[0];
    for (uint32_t i = 0; i < formatCount; i++) {
        if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
            formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            chosenFormat = formats[i];
            break;
        }
    }
    free(formats);

    //.63 pick a present mode
    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical, surface, &presentModeCount, NULL);
    if (presentModeCount == 0) {
	fprintf(stderr, "no present modes\n");
	return 1;
    }

    VkPresentModeKHR* presentModes = malloc(presentModeCount * sizeof(*presentModes));
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical, surface, &presentModeCount, presentModes);

    // MAILBOX = triple buffering (no tearing, no waiting). FIFO = hard vsync.
    // FIFO is guaranteed to be supported, so it's a safe default.
    VkPresentModeKHR chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (uint32_t i = 0; i < presentModeCount; i++) {
	if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
	    chosenPresentMode = presentModes[i];
	    break;
	}
    }
    free(presentModes);
    
    //.64 pick extent
    VkExtent2D chosenExtent;
    if (caps.currentExtent.width != UINT32_MAX) {
	// surface has a fixed size, you MUST use it
	chosenExtent = caps.currentExtent;
    } else {
	// surface is flexible; clamp our window size to what's allowed
	int w, h;
	glfwGetFramebufferSize(window, &w, &h);

	chosenExtent.width  = (uint32_t)w;
	chosenExtent.height = (uint32_t)h;

	if (chosenExtent.width  < caps.minImageExtent.width)  chosenExtent.width  = caps.minImageExtent.width;
	if (chosenExtent.width  > caps.maxImageExtent.width)  chosenExtent.width  = caps.maxImageExtent.width;
	if (chosenExtent.height < caps.minImageExtent.height) chosenExtent.height = caps.minImageExtent.height;
	if (chosenExtent.height > caps.maxImageExtent.height) chosenExtent.height = caps.maxImageExtent.height;
    }

    //.65 image count
    uint32_t imageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount) {
	imageCount = caps.maxImageCount;
    }
    //.66 queue family sharing

    uint32_t queueFamilyIndices[2] = { graphicsFamily, presentFamily };   // ← delete from here down
    VkSharingMode sharingMode;
    uint32_t queueFamilyIndexCount;
    const uint32_t* pQueueFamilyIndices;

    if (graphicsFamily != presentFamily) {
	sharingMode = VK_SHARING_MODE_CONCURRENT;
	queueFamilyIndexCount = 2;
	pQueueFamilyIndices = queueFamilyIndices;
    } else {
	sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	queueFamilyIndexCount = 0;
	pQueueFamilyIndices = NULL;
    }

    //.67 create the swapchain
    VkSwapchainCreateInfoKHR swapInfo = {0};
    swapInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapInfo.surface          = surface;
    swapInfo.minImageCount    = imageCount;
    swapInfo.imageFormat      = chosenFormat.format;
    swapInfo.imageColorSpace  = chosenFormat.colorSpace;
    swapInfo.imageExtent      = chosenExtent;
    swapInfo.imageArrayLayers = 1;                            // 1 unless doing VR/stereo
    swapInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapInfo.imageSharingMode = sharingMode;
    swapInfo.queueFamilyIndexCount = queueFamilyIndexCount;
    swapInfo.pQueueFamilyIndices   = pQueueFamilyIndices;
    swapInfo.preTransform     = caps.currentTransform;
    swapInfo.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapInfo.presentMode      = chosenPresentMode;
    swapInfo.clipped          = VK_TRUE;                      // don't render pixels hidden by other windows
    swapInfo.oldSwapchain     = VK_NULL_HANDLE;               // NULL on first creation

    VkSwapchainKHR swapchain;
    VkResult swapResult = vkCreateSwapchainKHR(device, &swapInfo, NULL, &swapchain);
    if (swapResult != VK_SUCCESS) {
	fprintf(stderr, "vkCreateSwapchainKHR() failed: %d\n", swapResult);
	// cleanup...
	return 1;
    }
    fprintf(stderr, "swapchain created!\n");


    //.68 fetch swapchain images
    uint32_t swapImageCount = 0;
    vkGetSwapchainImagesKHR(device, swapchain, &swapImageCount, NULL);

    VkImage* swapImages = malloc(swapImageCount * sizeof(*swapImages));
    vkGetSwapchainImagesKHR(device, swapchain, &swapImageCount, swapImages);

    fprintf(stderr, "got %u swapchain images\n", swapImageCount);


//STEP 2; RENDER LOOP

//.7 create image views for each swapchain image
VkImageView* swapImageViews = malloc(swapImageCount * sizeof(*swapImageViews));

for (uint32_t i = 0; i < swapImageCount; i++) {
    VkImageViewCreateInfo viewInfo = {0};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = swapImages[i];
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = chosenFormat.format;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &viewInfo, NULL, &swapImageViews[i]) != VK_SUCCESS) {
        fprintf(stderr, "vkCreateImageView failed\n");
        return 1;
    }
}
fprintf(stderr, "image views created\n");

for (uint32_t i = 0; i < swapImageCount; i++)
    vkDestroyImageView(device, swapImageViews[i], NULL);
free(swapImageViews);

//TEMP: keep window alive until user closes it
while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
}
fprintf(stderr, "window closed, exiting\n");


//STEP 3: GET RID OF EVIDENCE    
    vkDestroySwapchainKHR(device, swapchain, NULL);   // MUST come before vkDestroyDevice
    free(swapImages);
    vkDeviceWaitIdle(device);
    vkDestroyDevice(device, NULL);
    vkDestroySurfaceKHR(instance, surface, NULL);
    vkDestroyInstance(instance, NULL);
    glfwDestroyWindow(window);
    glfwTerminate();
}
