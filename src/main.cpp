#define GLFW_INCLUDE_VULKAN
#include "../include/glfw3.h"
#include <vulkan/vulkan_beta.h> 
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <optional>
#include <set>       // used in struct QueueFamilyIndices
 
const uint32_t WIDTH = 800;  // screen width
const uint32_t HEIGHT = 600; // screen height
 
// enable Vulkan SDK standard diagnostics (validation) layers
const std::vector<const char*> validationLayers =
{
  "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
 
// if compiled in debug mode: enable validation layers
#ifdef NDEBUG
  const bool enableValidationLayers = false;
#else
  const bool enableValidationLayers = true;
#endif
 
 
// look-up proxy function required for setupDebugMessenger()
VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger)
    {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) 
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    else
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
 
 
// proxy function required for cleanup()
void DestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator)
{
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) 
    func(instance, debugMessenger, pAllocator);
  
}
 
 
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};
 
 
class HelloTriangleApplication {
    public:
        void run() {
            initWindow();
            initVulkan();
            mainLoop();
            cleanup();
        }
 
    private:
        GLFWwindow* window; // window variable
        VkInstance instance; // vulkan instance variable
        VkDebugUtilsMessengerEXT debugMessenger;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE; // graphics card
        VkDevice device; // logical device
        VkQueue graphicsQueue;
        VkSurfaceKHR surface;
        VkQueue presentQueue;
        VkSwapchainKHR swapChain;
        std::vector<VkImage> swapChainImages;
        VkFormat swapChainImageFormat;
        VkExtent2D swapChainExtent;
 
 
  void initWindow()
  {
    glfwInit(); // initialize GLFW
 
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // do not create OpenGL context
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);   // disable resizing windows (for now)
 
    // window initizialize. param4: optionally specify monitor. param5: only relevant to OpenGL
    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
  }
 
 
  void initVulkan()
  {
    createInstance(); // initialize Vulkan library
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
  }
 
  void mainLoop()
  {
    while (!glfwWindowShouldClose(window)) // keep application running until window closed or error 
    {
      glfwPollEvents();
    }
  }
 
 
  void cleanup()
  {
    vkDestroySwapchainKHR(device, swapChain, nullptr);
    vkDestroyDevice(device, nullptr);
 
    if (enableValidationLayers)
    {
      DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
  }
 
 
  void createInstance()
  {
    // enable validation layers 
    if (enableValidationLayers && !checkValidationLayerSupport()) 
    {
      throw std::runtime_error("validation layers requested, but not available!");
    }
 
    // (optional) create struct with info about application
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;
 
    // tell Vulkan driver which global extensions and validation layers to use
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
 
    // get required extensions for validation layers message callback
    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR; // macOS specific fix
 
    // include validation layer names (if enabled) in VkInstanceCreateInfo instantiation
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers)
    {
      createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
      createInfo.ppEnabledLayerNames = validationLayers.data();
 
      populateDebugMessengerCreateInfo(debugCreateInfo);
      createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    }
    else
    {
      createInfo.enabledLayerCount = 0;
      createInfo.pNext = nullptr;
    }
 
    // create vulkan instance and check if created successfully
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) 
    {
      throw std::runtime_error("failed to create instance!");
    }
  }
 
 
  // needed to debug any issues in the vkCreateInstance and vkDestroyInstance calls
  void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) 
  {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
  }
 
 
  void setupDebugMessenger()
  {
    if (!enableValidationLayers) return;
 
    // details about the messenger and its callback
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    populateDebugMessengerCreateInfo(createInfo);
 
    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) 
    {
      throw std::runtime_error("failed to set up debug messenger!");
    }
  }
 
 
  // select graphics card
  // NOTE: possible to select "best" graphics card (see code in obsidian notes)
  void pickPhysicalDevice()
  {
    // list available graphics cards
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
 
    if (deviceCount == 0)
    {
      throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }
 
    // allocate array to store available devices
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
 
    // find suitable devices
    for (const auto& device : devices) 
    {
      if (isDeviceSuitable(device)) 
      {
        physicalDevice = device;
        break;
      }
    }
 
    if (physicalDevice == VK_NULL_HANDLE) 
    {
      throw std::runtime_error("failed to find a suitable GPU!");
    }
  }
 
 
  void createLogicalDevice() {
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    std::vector<const char*> device_extensions;
    if(enableValidationLayers)
        device_extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
        device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());;
    createInfo.ppEnabledExtensionNames = device_extensions.data();

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
  }


  void createSwapChain() {
      SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

      VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
      VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
      VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

      uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
      if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
          imageCount = swapChainSupport.capabilities.maxImageCount;
      }

      VkSwapchainCreateInfoKHR createInfo{};
      createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
      createInfo.surface = surface;

      createInfo.minImageCount = imageCount;
      createInfo.imageFormat = surfaceFormat.format;
      createInfo.imageColorSpace = surfaceFormat.colorSpace;
      createInfo.imageExtent = extent;
      createInfo.imageArrayLayers = 1;
      createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

      QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
      uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

      if (indices.graphicsFamily != indices.presentFamily) {
          createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
          createInfo.queueFamilyIndexCount = 2;
          createInfo.pQueueFamilyIndices = queueFamilyIndices;
      } else {
          createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
      }

      createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
      createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
      createInfo.presentMode = presentMode;
      createInfo.clipped = VK_TRUE;

      createInfo.oldSwapchain = VK_NULL_HANDLE;

      if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
          throw std::runtime_error("failed to create swap chain!");
      }

      vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
      swapChainImages.resize(imageCount);
      vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

      swapChainImageFormat = surfaceFormat.format;
      swapChainExtent = extent;
  }

  VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
      for (const auto& availableFormat : availableFormats) {
          if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
              return availableFormat;
          }
      }

      return availableFormats[0];
  }

  VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
      for (const auto& availablePresentMode : availablePresentModes) {
          if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
              return availablePresentMode;
          }
      }

      return VK_PRESENT_MODE_FIFO_KHR;
  }

  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
      if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
          return capabilities.currentExtent;
      } else {
          int width, height;
          glfwGetFramebufferSize(window, &width, &height);

          VkExtent2D actualExtent = {
              static_cast<uint32_t>(width),
              static_cast<uint32_t>(height)
          };

          actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
          actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

          return actualExtent;
      }
  }

  SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
  }

  // find suitable device
  bool isDeviceSuitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = findQueueFamilies(device);
    const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };  

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return indices.isComplete() && extensionsSupported && swapChainAdequate;
  }

  bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
        uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
  }
 
 
  // queue family lookup function. check which queue families are supported by the device
  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;
 
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
 
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

 
    // find at least one queue family that supports VK_QUEUE_GRAPHICS_BIT
    int i = 0;
    for (const auto& queueFamily : queueFamilies) 
    {
      if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        indices.graphicsFamily = i;
      }

      VkBool32 presentSupport = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

      if (presentSupport) {
        indices.presentFamily = i;
      }
 
      if (indices.isComplete()) {
        break;
      }

      i++;
    }
 
    return indices;
  }
 
  // validation layer message callback
  // return the required list of extensions based on whether validation layers are enabled or not
  // NOTE: `extensions` are for VkInstance
  std::vector<const char*> getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
 
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
 
    if (enableValidationLayers) 
    {
      extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
 
      // macOS specific fix
      extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
      extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME); // required for VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
    }

    for(int i = 0; i < extensions.size(); i++)
      printf("\n\nExtensions: %s \n\n", extensions[i]);

    return extensions;
  }
 
 
  // check if all of the requested validation layers are available
  bool checkValidationLayerSupport()
  {
    // list all available layers
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
 
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
 
    //return false;
 
    // check if all of the layers in validationLayers exist in the availableLayers list
    for (const char* layerName : validationLayers) 
    {
      bool layerFound = false;
 
      for (const auto& layerProperties : availableLayers) 
      {
        if (strcmp(layerName, layerProperties.layerName) == 0) 
        {
          layerFound = true;
          break;
        }
      }
 
      if (!layerFound) 
      {
        return false;
      }
    }
 
    return true;
  }
 
 
  // debug callback function
  static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,    // specifies the severity of the message
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, // message details
    void* pUserData) 
  {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
 
    return VK_FALSE;
  }

  void createSurface() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }

  }

};

 
 
int main()
{
  HelloTriangleApplication app;
 
  try
  {
    app.run();
  } 
  catch (const std::exception& e) 
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
 
  return EXIT_SUCCESS;
}