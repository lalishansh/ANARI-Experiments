#define BOOST_TEST_MODULE HelloWorldTests
#include <boost/test/included/unit_test.hpp>
#include <fmt/core.h>
#include <version.h>

BOOST_AUTO_TEST_CASE(hello_world_test) {
	std::string output = fmt::format("{}.{}.{}",
									 ANARI_Experiments_VERSION_MAJOR,
									 ANARI_Experiments_VERSION_MINOR,
									 ANARI_Experiments_VERSION_PATCH);
	BOOST_TEST(output == ANARI_Experiments_VERSION_STRING);
}

import "helper.hpp";
import "vkhelper.hpp";

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <thread>
#include <chrono>
#include <span>
#include <unordered_set>

#define ASSERT(condn)  do { if (!(condn)) { BOOST_TEST(false); return; } } while(0)
#define VK_CHECK(call) vkh::VkCheck(call, #call)

#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)
#define defer \
auto CONCAT(_scope_exit_, __LINE__) = nullptr + [&]()

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
													 VkDebugUtilsMessageTypeFlagsEXT message_type,
													 const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
													 void* user_data) {
	(void)message_type, (void)user_data;

	switch (message_severity) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			BOOST_TEST_MESSAGE(fmt::format("Vulkan error: {}: {}", callback_data->pMessageIdName, callback_data->pMessage));
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			BOOST_TEST_MESSAGE(fmt::format("Vulkan warning: {}: {}", callback_data->pMessageIdName, callback_data->pMessage));
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			BOOST_TEST_MESSAGE(fmt::format("Vulkan info: {}: {}", callback_data->pMessageIdName, callback_data->pMessage));
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			BOOST_TEST_MESSAGE(fmt::format("Vulkan verbose: {}: {}", callback_data->pMessageIdName, callback_data->pMessage));
			break;
		default:
			BOOST_TEST_MESSAGE(fmt::format("Vulkan unknown: {}: {}", callback_data->pMessageIdName, callback_data->pMessage));
			break;
	}
	return VK_FALSE;
}

BOOST_AUTO_TEST_CASE(hello_triangle_with_window_and_headless_vulkan_test) {
	constexpr ivec2 window_size = { 640, 480 };

	// Initialize GLFW
	ASSERT(glfwInit());
	defer {
		glfwTerminate();
	};

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(window_size.x, window_size.y, "Unit test", nullptr, nullptr);
	ASSERT(window != nullptr);
	defer {
		glfwDestroyWindow(window);
	};

	glfwMakeContextCurrent(window);
	// Create Vulkan Objects
	struct
	{
		VkInstance instance = VK_NULL_HANDLE;
		operator VkInstance() const { return instance; }

		VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
		std::vector<VkPhysicalDevice> physicalDevices;
	} instance;
	{
		// no need for swapchain as we are headless
		const char* const enable_extensions[] = {
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
		#if defined(PLATFORM__MACOS) // an assumption that the macOS platform will always support these extensions
			VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
			VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
		#endif
		};
		{
			u32 instance_extension_count = 0;
			VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, nullptr));
			std::vector<VkExtensionProperties> instance_extensions(instance_extension_count);
			VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, instance_extensions.data()));

			for (const char* extension : enable_extensions) {
				ASSERT(std::find_if(instance_extensions.begin(), instance_extensions.end(),
									[&extension](const VkExtensionProperties& extension_property) {
										return strcmp(extension, extension_property.extensionName) == 0;
									}) != instance_extensions.end());
			}
		}

		auto enable_layers = [] {
			std::vector<const char*> layers_by_priority[4] = {
				// The preferred validation layer is "VK_LAYER_KHRONOS_validation"
				{"VK_LAYER_KHRONOS_validation"},

				// Otherwise we fallback to using the LunarG meta layer
				{"VK_LAYER_LUNARG_standard_validation"},

				// Otherwise we attempt to enable the individual layers that compose the LunarG meta layer since it doesn't exist
				{
					"VK_LAYER_GOOGLE_threading",
					"VK_LAYER_LUNARG_parameter_validation",
					"VK_LAYER_LUNARG_object_tracker",
					"VK_LAYER_LUNARG_core_validation",
					"VK_LAYER_GOOGLE_unique_objects",
				},

				// Otherwise as a last resort we fallback to attempting to enable the LunarG core layer
				{"VK_LAYER_LUNARG_core_validation"}
			};

			u32 instance_layer_count = 0;
			VK_CHECK(vkEnumerateInstanceLayerProperties(&instance_layer_count, nullptr));
			std::vector<VkLayerProperties> instance_layers(instance_layer_count);
			VK_CHECK(vkEnumerateInstanceLayerProperties(&instance_layer_count, instance_layers.data()));

			for (const auto& layers : layers_by_priority) {
				if (std::all_of(layers.begin(), layers.end(), [&instance_layers](const char* layer) {
					return std::find_if(instance_layers.begin(), instance_layers.end(),
										[&layer](const VkLayerProperties& layer_property) {
											return strcmp(layer, layer_property.layerName) == 0;
										}) != instance_layers.end();
				})) {
					return std::move(layers);
				}
			}
			return std::vector<const char*>{};
		}();

		const auto app_info = VkApplicationInfo{
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pApplicationName = "Hello Triangle",
			.pEngineName = "Vulkan Samples",
			.apiVersion = VK_API_VERSION_1_3,
		};

		const auto debug_messenger_info = VkDebugUtilsMessengerCreateInfoEXT{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT,
			.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
			.pfnUserCallback = debug_callback,
			.pUserData = nullptr,
		};

		const auto instance_info = VkInstanceCreateInfo{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.flags = 0
			#if defined(PLATFORM__MACOS)
				| VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR
			#endif
			,
			.pApplicationInfo = &app_info,
			.enabledLayerCount = (u32)std::size(enable_layers),
			.ppEnabledLayerNames = enable_layers.data(),
			.enabledExtensionCount = (u32)std::size(enable_extensions),
			.ppEnabledExtensionNames = enable_extensions,
		};
		VK_CHECK(vkCreateInstance(&instance_info, nullptr, &instance.instance));
		auto vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
		VK_CHECK(vkCreateDebugUtilsMessengerEXT(instance, &debug_messenger_info, nullptr, &instance.debugMessenger));

		u32 physical_device_count = 0;
		VK_CHECK(vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr));
		ASSERT(physical_device_count > 0);
		instance.physicalDevices.resize(physical_device_count);
		VK_CHECK(vkEnumeratePhysicalDevices(instance, &physical_device_count, instance.physicalDevices.data()));
	}
	auto vkCmdBeginDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT"));
	auto vkCmdEndDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT"));
	auto vkCmdInsertDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdInsertDebugUtilsLabelEXT"));
	ASSERT(vkCmdBeginDebugUtilsLabelEXT && vkCmdEndDebugUtilsLabelEXT && vkCmdInsertDebugUtilsLabelEXT);

	struct
	{
		VkDevice device = VK_NULL_HANDLE;
		VkPhysicalDevice physicalDevice;
		VkPhysicalDeviceMemoryProperties memoryProperties;
		VkPhysicalDeviceVulkan13Features enabledFeatures {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};

		struct
		{
			u32 compute = 0, transfer = 0, graphics = 0;
		} queueFamilies;
		struct
		{
			VkQueue compute = VK_NULL_HANDLE, transfer = VK_NULL_HANDLE, graphics = VK_NULL_HANDLE;
		} queue;
		struct
		{
			VkCommandPool compute = VK_NULL_HANDLE, transfer = VK_NULL_HANDLE, graphics = VK_NULL_HANDLE;
		} commandPools;

		operator VkDevice() const { return device; }
	} device;
	{
		device.physicalDevice = instance.physicalDevices.front();
		for (auto vk_device = instance.physicalDevices.begin() + 1; vk_device != instance.physicalDevices.end(); ++vk_device) {
			VkPhysicalDeviceProperties device_properties;
			vkGetPhysicalDeviceProperties(*vk_device, &device_properties);
			if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
				device.physicalDevice = *vk_device;
				break;
			}
		}
		vkGetPhysicalDeviceMemoryProperties(device.physicalDevice, &device.memoryProperties);

		u32 queue_families_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device.physicalDevice, &queue_families_count, nullptr);
		std::vector<VkQueueFamilyProperties> queue_family_properties(queue_families_count);
		vkGetPhysicalDeviceQueueFamilyProperties(device.physicalDevice, &queue_families_count, queue_family_properties.data());

		device.queueFamilies = decltype(device.queueFamilies){u32(-1), u32(-1), u32(-1)};
		for (u32 i = 0; i < queue_families_count; ++i) {
			const auto& queue_family = queue_family_properties[i];
			if (device.queueFamilies.compute == u32(-1) && (queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT) && !(queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
				device.queueFamilies.compute = i;
			}
			if (device.queueFamilies.compute == u32(-1) && (queue_family.queueFlags & VK_QUEUE_TRANSFER_BIT) && !(queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
				device.queueFamilies.transfer = i;
			}
			if (device.queueFamilies.compute == u32(-1) && queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				device.queueFamilies.graphics = i;
			}
		}
		for (u32 i = 0; i < queue_families_count; ++i) {
			const auto& queue_family = queue_family_properties[i];
			if (device.queueFamilies.compute == u32(-1) && (queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
				device.queueFamilies.compute = i;
			}
			if (device.queueFamilies.compute == u32(-1) && (queue_family.queueFlags & VK_QUEUE_TRANSFER_BIT)) {
				device.queueFamilies.transfer = i;
			}
		}
		ASSERT(device.queueFamilies.compute != u32(-1) && device.queueFamilies.transfer != u32(-1) && device.queueFamilies.graphics != u32(-1));

		device.enabledFeatures = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES, .synchronization2 = VK_TRUE, .dynamicRendering = VK_TRUE};

		std::unordered_set<u32> unique_queue_families = {device.queueFamilies.compute, device.queueFamilies.transfer, device.queueFamilies.graphics};
		VkDeviceQueueCreateInfo queue_create_infos[3]; // compute, transfer, graphics
		const auto queue_priority = 1.0f;
		for (auto* ptr = queue_create_infos; const auto queue_family : unique_queue_families) {
            *ptr = VkDeviceQueueCreateInfo{
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = queue_family,
                .queueCount = 1,
            	.pQueuePriorities = &queue_priority,
            };
			++ptr;
        }

		// no need for swapchain as we are headless
		#if defined(PLATFORM__MACOS)
	        const char* const enable_extensions[] = {
				VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
	        };
			{
				u32 device_extension_count = 0;
				vkEnumerateDeviceExtensionProperties(device.physicalDevice, nullptr, &device_extension_count, nullptr);
				std::vector<VkExtensionProperties> device_extensions(device_extension_count);
				vkEnumerateDeviceExtensionProperties(device.physicalDevice, nullptr, &device_extension_count, device_extensions.data());

				ASSERT(std::all_of(enable_extensions, enable_extensions + std::size(enable_extensions), [&device_extensions](const char* extension) {
	                return std::find_if(device_extensions.begin(), device_extensions.end(),
	                                    [&extension](const VkExtensionProperties& extension_property) {
	                                        return strcmp(extension, extension_property.extensionName) == 0;
	                                    }) != device_extensions.end();
	            }));
			}
		#endif

		auto physical_device_features2 = VkPhysicalDeviceFeatures2{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .pNext = &device.enabledFeatures,
        };

		auto device_info = VkDeviceCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pNext = &physical_device_features2,
            .queueCreateInfoCount = (u32)std::size(queue_create_infos),
			.pQueueCreateInfos = queue_create_infos,
		#if defined(PLATFORM__MACOS)
			.enabledExtensionCount = std::size(enable_extensions),
			.ppEnabledExtensionNames = enable_extensions,
		#endif
			.pEnabledFeatures = nullptr,
        };

		VK_CHECK(vkCreateDevice(device.physicalDevice, &device_info, nullptr, &device.device));

		for (u32 i = 0; auto queue_family: unique_queue_families) {
			defer {
				++i;
			};
			VkQueue queue;
			vkGetDeviceQueue(device, queue_family, 0, &queue);

			VkCommandPool command_pool;
			auto command_pool_info = VkCommandPoolCreateInfo{
				.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
				.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
				.queueFamilyIndex = queue_family,
			};
			VK_CHECK(vkCreateCommandPool(device, &command_pool_info, nullptr, &command_pool));

			if (queue_family == device.queueFamilies.compute) {
				device.queue.compute = queue;
				device.commandPools.compute = command_pool;
			}
			if (queue_family == device.queueFamilies.transfer) {
				device.queue.transfer = queue;
				device.commandPools.transfer = command_pool;
			}
			if (queue_family == device.queueFamilies.graphics) {
				device.queue.graphics = queue;
				device.commandPools.graphics = command_pool;
			}
		}
	}

	static constexpr u32 pipeline_count = 1;
	struct ubo_t {
        mat4 model, view, proj;
    } mvp;

	// We will use a single pipeline for now
	struct
	{
		// why fence? because semaphore is for synchronization between commands rather than synchronization between entire command chain
		VkFence fence = VK_NULL_HANDLE;
		struct
		{
			VkCommandBuffer compute = VK_NULL_HANDLE, transfer = VK_NULL_HANDLE, graphics = VK_NULL_HANDLE;
		} commandBuffers;

		struct
		{
			VkFormat format = VK_FORMAT_UNDEFINED;
			VkImage image = VK_NULL_HANDLE;
			VkImageView view = VK_NULL_HANDLE;
			VkDeviceMemory memory = VK_NULL_HANDLE;
		} colorAttachment, depthAttachment;

		// kapil suggested to use VkDescriptorPool for new descriptor sets (for every pipeline) every frame then reset it on begining of frame
		// SET FORGET descriptor sets ... RESET descriptor pool
		// NOTE: we will need to reconstruct the pool when number of pipelines are changed
		VkDescriptorPool uboDescriptorPool = VK_NULL_HANDLE; // uniform buffer object descriptor pool, see DescriptorAllocatorGrowable, https://github.com/vblanco20-1/vulkan-guide/blob/all-chapters-2/shared/vk_descriptors.cpp

		// TODO: we will need to reconstruct the pool when number of pipelines are changed
		static_assert(pipeline_count == 1);
		struct
		{
			const usize size = sizeof(ubo_t);
			VkBuffer buffer = VK_NULL_HANDLE;
			VkDeviceMemory memory = VK_NULL_HANDLE;
			u8* mapped = nullptr;
		} ubos[pipeline_count];
	} frames[2];
	{
		for (auto& frame: frames) {
			VkFenceCreateInfo fence_info = {
	            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
	            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
	        };
			VK_CHECK(vkCreateFence(device, &fence_info, nullptr, &frame.fence));
		}

		VkCommandBufferAllocateInfo command_buffer_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = std::size(frames),
        };
		std::vector<VkCommandBuffer> command_buffers(std::size(frames));
		// graphics
		command_buffer_info.commandPool = device.commandPools.graphics;
		VK_CHECK(vkAllocateCommandBuffers(device, &command_buffer_info, command_buffers.data()));
		for (u32 i = 0; i < std::size(frames); ++i) {
            frames[i].commandBuffers.graphics = command_buffers[i];
        }
		// transfer
		command_buffer_info.commandPool = device.commandPools.transfer;
		VK_CHECK(vkAllocateCommandBuffers(device, &command_buffer_info, command_buffers.data()));
		for (u32 i = 0; i < std::size(frames); ++i) {
            frames[i].commandBuffers.transfer = command_buffers[i];
        }
		// compute
		command_buffer_info.commandPool = device.commandPools.compute;
		VK_CHECK(vkAllocateCommandBuffers(device, &command_buffer_info, command_buffers.data()));
		for (u32 i = 0; i < std::size(frames); ++i) {
            frames[i].commandBuffers.compute = command_buffers[i];
        }

		const VkFormat color_format = VK_FORMAT_B8G8R8A8_UNORM;
		const VkFormat depth_format = [&device] {
			VkFormat depth_formats_by_priority[] = {
				VK_FORMAT_D32_SFLOAT_S8_UINT,
				VK_FORMAT_D32_SFLOAT,
				VK_FORMAT_D24_UNORM_S8_UINT,
				VK_FORMAT_D16_UNORM_S8_UINT,
				VK_FORMAT_D16_UNORM
			};
			return *std::find_if(std::begin(depth_formats_by_priority), std::end(depth_formats_by_priority), [&device](const VkFormat format) {
                VkFormatProperties format_properties;
                vkGetPhysicalDeviceFormatProperties(device.physicalDevice, format, &format_properties);
                return format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
            });
		}();

		for (auto& frame : frames) {
			VkImageCreateInfo image_info = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
				.imageType = VK_IMAGE_TYPE_2D,
				.extent = {window_size.x, window_size.y, 1},
				.mipLevels = 1,
				.arrayLayers = 1,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.tiling = VK_IMAGE_TILING_OPTIMAL,
				.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			};
			VkMemoryAllocateInfo memory_info = {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            };
			VkMemoryRequirements memory_requirements;
			VkImageViewCreateInfo view_info = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .subresourceRange = {
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            };

			// color attachment
			image_info.format = frame.colorAttachment.format = color_format;
			image_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			VK_CHECK(vkCreateImage(device, &image_info, nullptr, &frame.colorAttachment.image));
			vkGetImageMemoryRequirements(device, frame.colorAttachment.image, &memory_requirements);
			memory_info.allocationSize = memory_requirements.size;
			memory_info.memoryTypeIndex = vkh::find_memory_type(device.memoryProperties, memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			VK_CHECK(vkAllocateMemory(device, &memory_info, nullptr, &frame.colorAttachment.memory));
			VK_CHECK(vkBindImageMemory(device, frame.colorAttachment.image, frame.colorAttachment.memory, 0));
			view_info.format = frame.colorAttachment.format;
			view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			view_info.image = frame.colorAttachment.image;
			VK_CHECK(vkCreateImageView(device, &view_info, nullptr, &frame.colorAttachment.view));
			// depth attachment
			image_info.format = frame.depthAttachment.format = depth_format;
			image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			VK_CHECK(vkCreateImage(device, &image_info, nullptr, &frame.depthAttachment.image));
			vkGetImageMemoryRequirements(device, frame.depthAttachment.image, &memory_requirements);
			memory_info.allocationSize = memory_requirements.size;
			memory_info.memoryTypeIndex = vkh::find_memory_type(device.memoryProperties, memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			VK_CHECK(vkAllocateMemory(device, &memory_info, nullptr, &frame.depthAttachment.memory));
			VK_CHECK(vkBindImageMemory(device, frame.depthAttachment.image, frame.depthAttachment.memory, 0));
			view_info.format = frame.depthAttachment.format;
			view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			if (depth_format == VK_FORMAT_D32_SFLOAT_S8_UINT || depth_format == VK_FORMAT_D24_UNORM_S8_UINT || depth_format == VK_FORMAT_D16_UNORM_S8_UINT) {
                view_info.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
			view_info.image = frame.depthAttachment.image;
			VK_CHECK(vkCreateImageView(device, &view_info, nullptr, &frame.depthAttachment.view));
		}

		for (auto& frame: frames) {
			static_assert(pipeline_count == 1);
			VkDescriptorPoolSize pool_sizes[] = {
				{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = pipeline_count},
			};
			VkDescriptorPoolCreateInfo pool_info = {
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
				.maxSets = pipeline_count,
				.poolSizeCount = std::size(pool_sizes),
				.pPoolSizes = pool_sizes,
			};
			VK_CHECK(vkCreateDescriptorPool(device, &pool_info, nullptr, &frame.uboDescriptorPool));

			for (auto& ubo: frame.ubos) {
				const VkBufferCreateInfo buffer_info = {
			        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			        .size = ubo.size,
			        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			    };

				VK_CHECK(vkCreateBuffer(device, &buffer_info, nullptr, &ubo.buffer));
				VkMemoryRequirements memory_requirements;
				vkGetBufferMemoryRequirements(device, ubo.buffer, &memory_requirements);
				const VkMemoryAllocateInfo memory_info = {
                    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                    .allocationSize = memory_requirements.size,
                    .memoryTypeIndex = vkh::find_memory_type(device.memoryProperties, memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
                };

				VK_CHECK(vkAllocateMemory(device, &memory_info, nullptr, &ubo.memory));
				VK_CHECK(vkBindBufferMemory(device, ubo.buffer, ubo.memory, 0));
				VK_CHECK(vkMapMemory(device, ubo.memory, 0, ubo.size, 0, reinterpret_cast<void**>(&ubo.mapped)));
			}
		}
	}

	struct geom_t
	{
		using vertex = struct { vec3 pos, color; };
		constexpr auto vertices[] = {
            vertex{ {  1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
            vertex{ { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
            vertex{ {  0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
        };
		constexpr u32 indices[] = { 0, 1, 2 };

		VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
        VkBuffer indexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
        u32 indexCount = 0;
	} triangle_vk_object;
	{
		const VkDeviceSize vertex_buffer_size = sizeof(geom_t::vertex) * std::size(triangle_vk_object.vertices);
		const VkDeviceSize index_buffer_size = sizeof(u32) * std::size(triangle_vk_object.indices);


	}

	(void)mvp;

	// Main Loop
	using time = std::chrono::system_clock;
	auto start_time = time::now();

	while (!glfwWindowShouldClose(window))
	{
		glfwSwapBuffers(window);

		glfwPollEvents();
		if (time::now() - start_time > std::chrono::seconds(5)) {
			break;
		}
	}

	BOOST_TEST(true);
}