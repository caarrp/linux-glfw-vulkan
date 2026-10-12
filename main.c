
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


//
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

    //.02 fill in instance create info
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    VkInstanceCreateInfo createInfo ={0};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledLayerCount = 0;        // validation layers go here later
    createInfo.ppEnabledLayerNames = NULL;
    createInfo.enabledExtensionCount = glfwExtensionCount;    // GLFW extensions go here
    createInfo.ppEnabledExtensionNames = glfwExtensions;


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
    deviceInfo.enabledExtensionCount = 0;            // swapchain ext later
    deviceInfo.ppEnabledExtensionNames = NULL;
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

    //.5 get the queues
        //graphics queue: channel for draw/rendering commands
        //present  queue: channel to submit "show present image on screen"
    VkQueue graphicsQueue, presentQueue;
    vkGetDeviceQueue(device, graphicsFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(device, presentFamily,  0, &presentQueue);

    fprintf(stderr, "**** device created\n");
    //create queue:
        //graphics queue: channel for draw/rendering commands
	//present  queue: channel to submit "show present image on screen"
    //VkQueue graphicsQueue, presentQueue;
    //vkGetDeviceQueue(device, graphicsFamily, 0, &graphicsQueue);
    //vkGetDeviceQueue(device, presentFamily, 0, &presentQueue);

    //create swapchain:
        //
    //VkSwapchainKHR swapchain = createSwapchain(device, physical, surface);

//STEP 2; RENDER LOOP

    vkDeviceWaitIdle(device);
    vkDestroyDevice(device, NULL);
    vkDestroySurfaceKHR(instance, surface, NULL);
    vkDestroyInstance(instance, NULL);
    glfwDestroyWindow(window);
    glfwTerminate();
}
