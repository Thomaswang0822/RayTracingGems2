#include "vulkan_utils.h"
#include <array>
#include <iostream>
#include <stdexcept>
#include <vector>
#include "vulkanrt_utils.h"

PFN_vkCreateAccelerationStructureNV vkCreateAccelerationStructure = nullptr;
PFN_vkDestroyAccelerationStructureNV vkDestroyAccelerationStructure = nullptr;
PFN_vkBindAccelerationStructureMemoryNV vkBindAccelerationStructureMemory = nullptr;
PFN_vkGetAccelerationStructureHandleNV vkGetAccelerationStructureHandle = nullptr;
PFN_vkGetAccelerationStructureMemoryRequirementsNV vkGetAccelerationStructureMemoryRequirements =
    nullptr;
PFN_vkCmdBuildAccelerationStructureNV vkCmdBuildAccelerationStructure = nullptr;
PFN_vkCmdCopyAccelerationStructureNV vkCmdCopyAccelerationStructure = nullptr;
PFN_vkCmdWriteAccelerationStructuresPropertiesNV vkCmdWriteAccelerationStructuresProperties =
    nullptr;
PFN_vkCreateRayTracingPipelinesNV vkCreateRayTracingPipelines = nullptr;
PFN_vkGetRayTracingShaderGroupHandlesNV vkGetRayTracingShaderGroupHandles = nullptr;
PFN_vkCmdTraceRaysNV vkCmdTraceRays = nullptr;

namespace vkrt {

void load_nv_rtx(VkDevice &device)
{
    vkCreateAccelerationStructure = reinterpret_cast<PFN_vkCreateAccelerationStructureNV>(
        vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureNV"));
    vkDestroyAccelerationStructure = reinterpret_cast<PFN_vkDestroyAccelerationStructureNV>(
        vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureNV"));
    vkBindAccelerationStructureMemory = reinterpret_cast<PFN_vkBindAccelerationStructureMemoryNV>(
        vkGetDeviceProcAddr(device, "vkBindAccelerationStructureMemoryNV"));
    vkGetAccelerationStructureHandle = reinterpret_cast<PFN_vkGetAccelerationStructureHandleNV>(
        vkGetDeviceProcAddr(device, "vkGetAccelerationStructureHandleNV"));
    vkGetAccelerationStructureMemoryRequirements =
        reinterpret_cast<PFN_vkGetAccelerationStructureMemoryRequirementsNV>(
            vkGetDeviceProcAddr(device, "vkGetAccelerationStructureMemoryRequirementsNV"));
    vkCmdBuildAccelerationStructure = reinterpret_cast<PFN_vkCmdBuildAccelerationStructureNV>(
        vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructureNV"));
    vkCmdCopyAccelerationStructure = reinterpret_cast<PFN_vkCmdCopyAccelerationStructureNV>(
        vkGetDeviceProcAddr(device, "vkCmdCopyAccelerationStructureNV"));
    vkCmdWriteAccelerationStructuresProperties =
        reinterpret_cast<PFN_vkCmdWriteAccelerationStructuresPropertiesNV>(
            vkGetDeviceProcAddr(device, "vkCmdWriteAccelerationStructuresPropertiesNV"));
    vkCreateRayTracingPipelines = reinterpret_cast<PFN_vkCreateRayTracingPipelinesNV>(
        vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesNV"));
    vkGetRayTracingShaderGroupHandles = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesNV>(
        vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesNV"));
    vkCmdTraceRays =
        reinterpret_cast<PFN_vkCmdTraceRaysNV>(vkGetDeviceProcAddr(device, "vkCmdTraceRaysNV"));
}

static const std::array<const char *, 1> validation_layers = {"VK_LAYER_KHRONOS_validation"};

Device::Device()
{
    make_instance();
    select_physical_device();
    make_logical_device();

    load_nv_rtx(device);

    // Query the properties we'll use frequently
    vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_props);

    rt_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;
    VkPhysicalDeviceProperties2 props = {};
    props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    props.pNext = &rt_props;
    props.properties = {};
    vkGetPhysicalDeviceProperties2(physical_device, &props);
}

Device::~Device()
{
    if (instance != VK_NULL_HANDLE) {
        vkDestroyDevice(device, nullptr);
        vkDestroyInstance(instance, nullptr);
    }
}

Device::Device(Device &&d)
    : instance(d.instance),
      physical_device(d.physical_device),
      device(d.device),
      queue(d.queue),
      mem_props(d.mem_props),
      rt_props(d.rt_props)
{
    d.instance = VK_NULL_HANDLE;
    d.physical_device = VK_NULL_HANDLE;
    d.device = VK_NULL_HANDLE;
    d.queue = VK_NULL_HANDLE;
}

Device &Device::operator=(Device &&d)
{
    if (instance != VK_NULL_HANDLE) {
        vkDestroyDevice(device, nullptr);
        vkDestroyInstance(instance, nullptr);
    }
    instance = d.instance;
    physical_device = d.physical_device;
    device = d.device;
    queue = d.queue;
    mem_props = d.mem_props;
    rt_props = d.rt_props;

    d.instance = VK_NULL_HANDLE;
    d.physical_device = VK_NULL_HANDLE;
    d.device = VK_NULL_HANDLE;
    d.queue = VK_NULL_HANDLE;

    return *this;
}

VkDevice Device::logical_device()
{
    return device;
}

VkQueue Device::graphics_queue()
{
    return queue;
}

uint32_t Device::queue_index() const
{
    return graphics_queue_index;
}

VkCommandPool Device::make_command_pool(VkCommandPoolCreateFlagBits flags)
{
    VkCommandPool pool = VK_NULL_HANDLE;
    VkCommandPoolCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    create_info.flags = flags;
    create_info.queueFamilyIndex = graphics_queue_index;
    CHECK_VULKAN(vkCreateCommandPool(device, &create_info, nullptr, &pool));
    return pool;
}

uint32_t Device::memory_type_index(uint32_t type_filter, VkMemoryPropertyFlags props) const
{
    for (uint32_t i = 0; i < mem_props.memoryTypeCount; ++i) {
        if (type_filter & (1 << i) && (mem_props.memoryTypes[i].propertyFlags & props) == props) {
            return i;
        }
    }
    throw std::runtime_error("failed to find appropriate memory");
}

VkDeviceMemory Device::alloc(size_t nbytes, uint32_t type_filter, VkMemoryPropertyFlags props)
{
    VkMemoryAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    info.allocationSize = nbytes;
    info.memoryTypeIndex = memory_type_index(type_filter, props);
    VkDeviceMemory mem = VK_NULL_HANDLE;
    CHECK_VULKAN(vkAllocateMemory(device, &info, nullptr, &mem));
    return mem;
}

const VkPhysicalDeviceMemoryProperties &Device::memory_properties() const
{
    return mem_props;
}

const VkPhysicalDeviceRayTracingPropertiesNV &Device::raytracing_properties() const
{
    return rt_props;
}

void Device::make_instance()
{
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "ChameleonRT";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "None";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = 0;
    create_info.ppEnabledExtensionNames = nullptr;
#ifdef _WIN32
    create_info.enabledLayerCount = validation_layers.size();
    create_info.ppEnabledLayerNames = validation_layers.data();
#endif

    CHECK_VULKAN(vkCreateInstance(&create_info, nullptr, &instance));
}

void Device::select_physical_device()
{
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
    std::vector<VkPhysicalDevice> devices(device_count, VkPhysicalDevice{});
    vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

    for (const auto &d : devices) {
        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceProperties(d, &properties);
        vkGetPhysicalDeviceFeatures(d, &features);

        uint32_t extension_count = 0;
        vkEnumerateDeviceExtensionProperties(d, nullptr, &extension_count, nullptr);
        std::vector<VkExtensionProperties> extensions(extension_count, VkExtensionProperties{});
        vkEnumerateDeviceExtensionProperties(d, nullptr, &extension_count, extensions.data());

        // Check for RTX support on this device
        auto fnd =
            std::find_if(extensions.begin(), extensions.end(), [](const VkExtensionProperties &e) {
                return std::strcmp(e.extensionName, VK_NV_RAY_TRACING_EXTENSION_NAME) == 0;
            });

        if (fnd != extensions.end()) {
            physical_device = d;
            break;
        }
    }

    if (physical_device == VK_NULL_HANDLE) {
        std::cout << "Failed to find RTX capable GPU\n";
        throw std::runtime_error("Failed to find RTX capable GPU");
    }
}

void Device::make_logical_device()
{
    uint32_t num_queue_families = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &num_queue_families, nullptr);
    std::vector<VkQueueFamilyProperties> family_props(num_queue_families,
                                                      VkQueueFamilyProperties{});
    vkGetPhysicalDeviceQueueFamilyProperties(
        physical_device, &num_queue_families, family_props.data());
    for (uint32_t i = 0; i < num_queue_families; ++i) {
        if (family_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphics_queue_index = i;
            break;
        }
    }

    const float queue_priority = 1.f;

    VkDeviceQueueCreateInfo queue_create_info = {};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = graphics_queue_index;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queue_priority;

    VkPhysicalDeviceFeatures device_features = {};
    device_features.shaderFloat64 = true;
    device_features.shaderInt64 = true;

    VkPhysicalDeviceDescriptorIndexingFeaturesEXT device_desc_features = {};
    device_desc_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
    device_desc_features.shaderStorageBufferArrayNonUniformIndexing = true;
    device_desc_features.runtimeDescriptorArray = true;
    device_desc_features.descriptorBindingVariableDescriptorCount = true;
    device_desc_features.shaderSampledImageArrayNonUniformIndexing = true;

    const std::vector<const char *> device_extensions = {
        VK_NV_RAY_TRACING_EXTENSION_NAME,
        VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME};

    VkDeviceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount = 1;
    create_info.pQueueCreateInfos = &queue_create_info;
    create_info.enabledLayerCount = validation_layers.size();
    create_info.ppEnabledLayerNames = validation_layers.data();
    create_info.enabledExtensionCount = device_extensions.size();
    create_info.ppEnabledExtensionNames = device_extensions.data();
    create_info.pEnabledFeatures = &device_features;
    create_info.pNext = &device_desc_features;
    CHECK_VULKAN(vkCreateDevice(physical_device, &create_info, nullptr, &device));

    vkGetDeviceQueue(device, graphics_queue_index, 0, &queue);
}

VkBufferCreateInfo Buffer::create_info(size_t nbytes, VkBufferUsageFlags usage)
{
    VkBufferCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = nbytes;
    info.usage = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    return info;
}

std::shared_ptr<Buffer> Buffer::make_buffer(Device &device,
                                            size_t nbytes,
                                            VkBufferUsageFlags usage,
                                            VkMemoryPropertyFlags mem_props)
{
    auto buf = std::make_shared<Buffer>();
    buf->vkdevice = &device;
    buf->buf_size = nbytes;
    buf->host_visible = mem_props & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    auto create_info = Buffer::create_info(nbytes, usage);
    CHECK_VULKAN(vkCreateBuffer(device.logical_device(), &create_info, nullptr, &buf->buf));

    VkMemoryRequirements mem_reqs = {};
    vkGetBufferMemoryRequirements(device.logical_device(), buf->buf, &mem_reqs);
    buf->mem = device.alloc(mem_reqs.size, mem_reqs.memoryTypeBits, mem_props);

    vkBindBufferMemory(device.logical_device(), buf->buf, buf->mem, 0);

    return buf;
}

Buffer::~Buffer()
{
    if (buf != VK_NULL_HANDLE) {
        vkDestroyBuffer(vkdevice->logical_device(), buf, nullptr);
        vkFreeMemory(vkdevice->logical_device(), mem, nullptr);
    }
}

Buffer::Buffer(Buffer &&b)
    : buf_size(b.buf_size),
      buf(b.buf),
      mem(b.mem),
      vkdevice(b.vkdevice),
      host_visible(b.host_visible)
{
    b.buf_size = 0;
    b.buf = VK_NULL_HANDLE;
    b.mem = VK_NULL_HANDLE;
    b.vkdevice = nullptr;
}

Buffer &Buffer::operator=(Buffer &&b)
{
    if (buf != VK_NULL_HANDLE) {
        vkDestroyBuffer(vkdevice->logical_device(), buf, nullptr);
        vkFreeMemory(vkdevice->logical_device(), mem, nullptr);
    }
    buf_size = b.buf_size;
    buf = b.buf;
    mem = b.mem;
    vkdevice = b.vkdevice;
    host_visible = b.host_visible;

    b.buf_size = 0;
    b.buf = VK_NULL_HANDLE;
    b.mem = VK_NULL_HANDLE;
    b.vkdevice = nullptr;
    return *this;
}

std::shared_ptr<Buffer> Buffer::host(Device &device,
                                     size_t nbytes,
                                     VkBufferUsageFlags usage,
                                     VkMemoryPropertyFlagBits extra_mem_props)
{
    return make_buffer(
        device, nbytes, usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | extra_mem_props);
}

std::shared_ptr<Buffer> Buffer::device(Device &device,
                                       size_t nbytes,
                                       VkBufferUsageFlags usage,
                                       VkMemoryPropertyFlagBits extra_mem_props)
{
    return make_buffer(
        device, nbytes, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | extra_mem_props);
}

void *Buffer::map()
{
    assert(host_visible);
    void *mapping = nullptr;
    CHECK_VULKAN(vkMapMemory(vkdevice->logical_device(), mem, 0, buf_size, 0, &mapping));
    return mapping;
}

void *Buffer::map(size_t offset, size_t size)
{
    assert(host_visible);
    assert(offset + size < buf_size);
    void *mapping = nullptr;
    CHECK_VULKAN(vkMapMemory(vkdevice->logical_device(), mem, offset, size, 0, &mapping));
    return mapping;
}

void Buffer::unmap()
{
    assert(host_visible);
    vkUnmapMemory(vkdevice->logical_device(), mem);
}

size_t Buffer::size() const
{
    return buf_size;
}

VkBuffer Buffer::handle() const
{
    return buf;
}

Texture2D::~Texture2D()
{
    if (image != VK_NULL_HANDLE) {
        vkDestroyImageView(vkdevice->logical_device(), view, nullptr);
        vkDestroyImage(vkdevice->logical_device(), image, nullptr);
        vkFreeMemory(vkdevice->logical_device(), mem, nullptr);
    }
}

Texture2D::Texture2D(Texture2D &&t)
    : tdims(t.tdims),
      img_format(t.img_format),
      img_layout(t.img_layout),
      image(t.image),
      mem(t.mem),
      view(t.view),
      vkdevice(t.vkdevice)
{
    t.image = VK_NULL_HANDLE;
    t.mem = VK_NULL_HANDLE;
    t.view = VK_NULL_HANDLE;
    t.vkdevice = nullptr;
}

Texture2D &Texture2D::operator=(Texture2D &&t)
{
    if (image != VK_NULL_HANDLE) {
        vkDestroyImageView(vkdevice->logical_device(), view, nullptr);
        vkDestroyImage(vkdevice->logical_device(), image, nullptr);
        vkFreeMemory(vkdevice->logical_device(), mem, nullptr);
    }
    tdims = t.tdims;
    img_format = t.img_format;
    img_layout = t.img_layout;
    image = t.image;
    mem = t.mem;
    view = t.view;
    vkdevice = t.vkdevice;

    t.image = VK_NULL_HANDLE;
    t.view = VK_NULL_HANDLE;
    t.vkdevice = nullptr;
    return *this;
}

std::shared_ptr<Texture2D> Texture2D::device(Device &device,
                                             glm::uvec2 dims,
                                             VkFormat img_format,
                                             VkImageUsageFlags usage)
{
    auto texture = std::make_shared<Texture2D>();
    texture->img_format = img_format;
    texture->tdims = dims;
    texture->vkdevice = &device;

    VkImageCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    create_info.imageType = VK_IMAGE_TYPE_2D;
    create_info.format = texture->img_format;
    create_info.extent.width = texture->tdims.x;
    create_info.extent.height = texture->tdims.y;
    create_info.extent.depth = 1;
    create_info.mipLevels = 1;
    create_info.arrayLayers = 1;
    create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    create_info.usage = usage;
    create_info.initialLayout = texture->img_layout;
    CHECK_VULKAN(vkCreateImage(device.logical_device(), &create_info, nullptr, &texture->image));

    VkMemoryRequirements mem_reqs = {};
    vkGetImageMemoryRequirements(device.logical_device(), texture->image, &mem_reqs);
    texture->mem =
        device.alloc(mem_reqs.size, mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    CHECK_VULKAN(vkBindImageMemory(device.logical_device(), texture->image, texture->mem, 0));

    VkImageViewCreateInfo view_create_info = {};
    view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_create_info.image = texture->image;
    view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_create_info.format = texture->img_format;

    view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_create_info.subresourceRange.baseMipLevel = 0;
    view_create_info.subresourceRange.levelCount = 1;
    view_create_info.subresourceRange.baseArrayLayer = 0;
    view_create_info.subresourceRange.layerCount = 1;

    CHECK_VULKAN(
        vkCreateImageView(device.logical_device(), &view_create_info, nullptr, &texture->view));
    return texture;
}

size_t Texture2D::pixel_size() const
{
    switch (img_format) {
    case VK_FORMAT_R16_UINT:
        return 2;
    case VK_FORMAT_R32_UINT:
    case VK_FORMAT_R32_SFLOAT:
    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_B8G8R8A8_UNORM:
    case VK_FORMAT_R8G8B8A8_SRGB:
        return 4;
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        return 16;
    default:
        throw std::runtime_error("Unhandled image format!");
    }
}

VkFormat Texture2D::pixel_format() const
{
    return img_format;
}

glm::uvec2 Texture2D::dims() const
{
    return tdims;
}

VkImage Texture2D::image_handle() const
{
    return image;
}

VkImageView Texture2D::view_handle() const
{
    return view;
}

ShaderModule::ShaderModule(Device &vkdevice, const uint32_t *code, size_t code_size)
    : device(&vkdevice)
{
    VkShaderModuleCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = code_size;
    info.pCode = code;
    CHECK_VULKAN(vkCreateShaderModule(vkdevice.logical_device(), &info, nullptr, &module));
}

ShaderModule::~ShaderModule()
{
    if (module != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device->logical_device(), module, nullptr);
    }
}

ShaderModule::ShaderModule(ShaderModule &&sm) : device(sm.device), module(sm.module)
{
    sm.device = nullptr;
    sm.module = VK_NULL_HANDLE;
}
ShaderModule &ShaderModule::operator=(ShaderModule &&sm)
{
    if (module != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device->logical_device(), module, nullptr);
    }
    device = sm.device;
    module = sm.module;

    sm.device = nullptr;
    sm.module = VK_NULL_HANDLE;
    return *this;
}

CombinedImageSampler::CombinedImageSampler(const std::shared_ptr<Texture2D> &t, VkSampler sampler)
    : texture(t), sampler(sampler)
{
}

DescriptorSetLayoutBuilder &DescriptorSetLayoutBuilder::add_binding(uint32_t binding,
                                                                    uint32_t count,
                                                                    VkDescriptorType type,
                                                                    uint32_t stage_flags,
                                                                    uint32_t ext_flags)
{
    VkDescriptorSetLayoutBinding desc = {};
    desc.binding = binding;
    desc.descriptorCount = count;
    desc.descriptorType = type;
    desc.stageFlags = stage_flags;
    bindings.push_back(desc);
    binding_ext_flags.push_back(ext_flags);
    return *this;
}

VkDescriptorSetLayout DescriptorSetLayoutBuilder::build(Device &device)
{
    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT ext_flags = {};
    ext_flags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
    ext_flags.bindingCount = binding_ext_flags.size();
    ext_flags.pBindingFlags = binding_ext_flags.data();

    VkDescriptorSetLayoutCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    create_info.bindingCount = bindings.size();
    create_info.pBindings = bindings.data();
    create_info.pNext = &ext_flags;

    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    CHECK_VULKAN(
        vkCreateDescriptorSetLayout(device.logical_device(), &create_info, nullptr, &layout));
    return layout;
}

DescriptorSetUpdater &DescriptorSetUpdater::write_acceleration_structure(
    VkDescriptorSet set, uint32_t binding, const std::unique_ptr<TopLevelBVH> &bvh)
{
    VkWriteDescriptorSetAccelerationStructureNV info = {};
    info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;
    info.accelerationStructureCount = 1;
    info.pAccelerationStructures = &bvh->bvh;

    WriteDescriptorInfo write;
    write.dst_set = set;
    write.binding = binding;
    write.count = 1;
    write.type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;
    write.as_index = accel_structs.size();

    accel_structs.push_back(info);
    writes.push_back(write);
    return *this;
}

DescriptorSetUpdater &DescriptorSetUpdater::write_storage_image(
    VkDescriptorSet set, uint32_t binding, const std::shared_ptr<Texture2D> &img)
{
    VkDescriptorImageInfo img_desc = {};
    img_desc.imageView = img->view_handle();
    img_desc.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    WriteDescriptorInfo write;
    write.dst_set = set;
    write.binding = binding;
    write.count = 1;
    write.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    write.img_index = images.size();

    images.push_back(img_desc);
    writes.push_back(write);
    return *this;
}

DescriptorSetUpdater &DescriptorSetUpdater::write_ubo(VkDescriptorSet set,
                                                      uint32_t binding,
                                                      const std::shared_ptr<Buffer> &buf)
{
    VkDescriptorBufferInfo buf_desc = {};
    buf_desc.buffer = buf->handle();
    buf_desc.offset = 0;
    buf_desc.range = buf->size();

    WriteDescriptorInfo write;
    write.dst_set = set;
    write.binding = binding;
    write.count = 1;
    write.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.buf_index = buffers.size();

    buffers.push_back(buf_desc);
    writes.push_back(write);
    return *this;
}

DescriptorSetUpdater &DescriptorSetUpdater::write_ssbo(VkDescriptorSet set,
                                                       uint32_t binding,
                                                       const std::shared_ptr<Buffer> &buf)
{
    VkDescriptorBufferInfo buf_desc = {};
    buf_desc.buffer = buf->handle();
    buf_desc.offset = 0;
    buf_desc.range = buf->size();

    WriteDescriptorInfo write;
    write.dst_set = set;
    write.binding = binding;
    write.count = 1;
    write.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.buf_index = buffers.size();

    buffers.push_back(buf_desc);
    writes.push_back(write);
    return *this;
}

DescriptorSetUpdater &DescriptorSetUpdater::write_ssbo_array(
    VkDescriptorSet set, uint32_t binding, const std::vector<std::shared_ptr<Buffer>> &bufs)
{
    WriteDescriptorInfo write;
    write.dst_set = set;
    write.binding = binding;
    write.count = bufs.size();
    write.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.buf_index = buffers.size();

    std::transform(bufs.begin(),
                   bufs.end(),
                   std::back_inserter(buffers),
                   [](const std::shared_ptr<Buffer> &b) {
                       VkDescriptorBufferInfo buf_desc = {};
                       buf_desc.buffer = b->handle();
                       buf_desc.offset = 0;
                       buf_desc.range = b->size();
                       return buf_desc;
                   });

    writes.push_back(write);
    return *this;
}

DescriptorSetUpdater &DescriptorSetUpdater::write_combined_sampler_array(
    VkDescriptorSet set,
    uint32_t binding,
    const std::vector<CombinedImageSampler> &combined_samplers)
{
    WriteDescriptorInfo write;
    write.dst_set = set;
    write.binding = binding;
    write.count = combined_samplers.size();
    write.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.img_index = images.size();

    std::transform(combined_samplers.begin(),
                   combined_samplers.end(),
                   std::back_inserter(images),
                   [](const CombinedImageSampler &cs) {
                       VkDescriptorImageInfo desc = {};
                       desc.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                       desc.imageView = cs.texture->view_handle();
                       desc.sampler = cs.sampler;
                       return desc;
                   });

    writes.push_back(write);
    return *this;
}

void DescriptorSetUpdater::update(Device &device)
{
    std::vector<VkWriteDescriptorSet> desc_writes;
    std::transform(writes.begin(),
                   writes.end(),
                   std::back_inserter(desc_writes),
                   [&](const WriteDescriptorInfo &w) {
                       VkWriteDescriptorSet wd = {};
                       wd.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                       wd.dstSet = w.dst_set;
                       wd.dstBinding = w.binding;
                       wd.descriptorCount = w.count;
                       wd.descriptorType = w.type;

                       if (wd.descriptorType == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV) {
                           wd.pNext = &accel_structs[w.as_index];
                       } else if (wd.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
                                  wd.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
                           wd.pBufferInfo = &buffers[w.buf_index];
                       } else if (wd.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ||
                                  wd.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
                           wd.pImageInfo = &images[w.img_index];
                       }
                       return wd;
                   });
    vkUpdateDescriptorSets(
        device.logical_device(), desc_writes.size(), desc_writes.data(), 0, nullptr);
}

}
