#include "wgpucontext.h"
#include "glfw3webgpu.h"
#include "error.h"
#include <cstdio>

#if defined(__APPLE__)
#  define WGPU_BACKEND WGPUInstanceBackend_Metal
#elif defined(_WIN32)
#  define WGPU_BACKEND WGPUInstanceBackend_DX12
#else
#  define WGPU_BACKEND WGPUInstanceBackend_Vulkan
#endif

static void AdapterCallback (WGPURequestAdapterStatus status, WGPUAdapter adapter,
                              WGPUStringView msg, void* ud1, void* ud2)
{
  (void) msg; (void) ud2;
  WGPUAdapter* out = (WGPUAdapter*) ud1;
  if (status == WGPURequestAdapterStatus_Success) *out = adapter;
  else Error::Fatal("wgpuInstanceRequestAdapter falhou");
}

static void DeviceCallback (WGPURequestDeviceStatus status, WGPUDevice device,
                             WGPUStringView msg, void* ud1, void* ud2)
{
  (void) msg; (void) ud2;
  WGPUDevice* out = (WGPUDevice*) ud1;
  if (status == WGPURequestDeviceStatus_Success) *out = device;
  else Error::Fatal("wgpuAdapterRequestDevice falhou");
}

WgpuContext::WgpuContext ()
  : m_instance(nullptr), m_surface(nullptr), m_adapter(nullptr),
    m_device(nullptr), m_queue(nullptr), m_surfaceFormat(WGPUTextureFormat_Undefined)
{
}

WgpuContextPtr WgpuContext::Make (GLFWwindow* window, int width, int height)
{
  WgpuContextPtr ctx(new WgpuContext());

  WGPUInstanceExtras extras = {};
  extras.chain.sType = (WGPUSType) WGPUSType_InstanceExtras;
  extras.backends = WGPU_BACKEND;

  WGPUInstanceDescriptor instanceDesc = {};
  instanceDesc.nextInChain = &extras.chain;
  ctx->m_instance = wgpuCreateInstance(&instanceDesc);
  if (!ctx->m_instance) Error::Fatal("wgpuCreateInstance falhou");

  ctx->m_surface = glfwCreateWindowWGPUSurface(ctx->m_instance, window);
  if (!ctx->m_surface) Error::Fatal("glfwCreateWindowWGPUSurface falhou");

  WGPURequestAdapterOptions adapterOptions = {};
  adapterOptions.compatibleSurface = ctx->m_surface;
  adapterOptions.powerPreference = WGPUPowerPreference_HighPerformance;

  WGPUAdapter adapter = nullptr;
  WGPURequestAdapterCallbackInfo adapterCbInfo = {};
  adapterCbInfo.mode = WGPUCallbackMode_AllowSpontaneous;
  adapterCbInfo.callback = AdapterCallback;
  adapterCbInfo.userdata1 = &adapter;
  wgpuInstanceRequestAdapter(ctx->m_instance, &adapterOptions, adapterCbInfo);
  ctx->WaitFor([&]() { return adapter != nullptr; });
  ctx->m_adapter = adapter;

  WGPUDeviceDescriptor deviceDesc = {};
  deviceDesc.label = { "device", WGPU_STRLEN };
  deviceDesc.deviceLostCallbackInfo = Error::MakeDeviceLostCallbackInfo();
  deviceDesc.uncapturedErrorCallbackInfo = Error::MakeErrorCallbackInfo();

  WGPUDevice device = nullptr;
  WGPURequestDeviceCallbackInfo deviceCbInfo = {};
  deviceCbInfo.mode = WGPUCallbackMode_AllowSpontaneous;
  deviceCbInfo.callback = DeviceCallback;
  deviceCbInfo.userdata1 = &device;
  wgpuAdapterRequestDevice(ctx->m_adapter, &deviceDesc, deviceCbInfo);
  ctx->WaitFor([&]() { return device != nullptr; });
  ctx->m_device = device;

  ctx->m_queue = wgpuDeviceGetQueue(ctx->m_device);

  WGPUSurfaceCapabilities caps = WGPU_SURFACE_CAPABILITIES_INIT;
  wgpuSurfaceGetCapabilities(ctx->m_surface, ctx->m_adapter, &caps);
  ctx->m_surfaceFormat = caps.formatCount > 0 ? caps.formats[0] : WGPUTextureFormat_BGRA8Unorm;
  wgpuSurfaceCapabilitiesFreeMembers(caps);

  ctx->Configure(width, height);

  return ctx;
}

WgpuContext::~WgpuContext ()
{
  if (m_surface) wgpuSurfaceUnconfigure(m_surface);
  if (m_queue) wgpuQueueRelease(m_queue);
  if (m_device) wgpuDeviceRelease(m_device);
  if (m_adapter) wgpuAdapterRelease(m_adapter);
  if (m_surface) wgpuSurfaceRelease(m_surface);
  if (m_instance) wgpuInstanceRelease(m_instance);
}

void WgpuContext::Configure (int width, int height)
{
  WGPUSurfaceConfiguration config = {};
  config.device = m_device;
  config.format = m_surfaceFormat;
  config.usage = WGPUTextureUsage_RenderAttachment;
  config.presentMode = WGPUPresentMode_Fifo;
  config.width = (uint32_t) width;
  config.height = (uint32_t) height;
  wgpuSurfaceConfigure(m_surface, &config);
}

WGPUDevice WgpuContext::GetDevice () const { return m_device; }
WGPUQueue WgpuContext::GetQueue () const { return m_queue; }
WGPUSurface WgpuContext::GetSurface () const { return m_surface; }
WGPUTextureFormat WgpuContext::GetSurfaceFormat () const { return m_surfaceFormat; }

WGPUSurfaceTexture WgpuContext::GetCurrentTexture () const
{
  WGPUSurfaceTexture tex = WGPU_SURFACE_TEXTURE_INIT;
  wgpuSurfaceGetCurrentTexture(m_surface, &tex);
  return tex;
}

void WgpuContext::Present () const
{
  wgpuSurfacePresent(m_surface);
}

void WgpuContext::WaitFor (const std::function<bool()>& done) const
{
  while (!done()) wgpuInstanceProcessEvents(m_instance);
}
