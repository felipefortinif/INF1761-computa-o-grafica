#include <memory>
class Algorithm;
using AlgorithmPtr = std::shared_ptr<Algorithm>;

#ifndef ALGORITHM_H
#define ALGORITHM_H

#include <utility>
#include "framebuffer.h"
#include "scene.h"
#include "camera.h"

// Encoder/render-pass/submit mechanics for multi-pass techniques (shadow
// mapping, planar shadows, reflections) that don't fit Renderer's
// one-pass shape. Stateless - build once, reuse for the app's lifetime.
// Attachments come from a caller-built Framebuffer; pipeline/transform
// selection stays the app's job via Node.
//
// Unlike python's Algorithm, RenderPass takes the attachment size
// explicitly rather than reading it off a GPUTextureView - webgpu.h has
// no such introspection call (wgpu-py's GPUTextureView.size is a
// binding-level convenience with no native C equivalent), and the
// caller already knows the size from whichever texture it built the
// view from.
class Algorithm {
  WGPUDevice m_device;
protected:
  Algorithm (WGPUDevice device);
public:
  static AlgorithmPtr Make (WGPUDevice device);

  // Encodes, runs and submits one full render pass into `framebuffer`,
  // rendering `scene` through `camera`. Self-contained - safe to call
  // multiple times per frame for multi-pass techniques.
  void RenderPass (const Framebuffer& framebuffer, ScenePtr scene, CameraPtr camera, std::pair<int, int> size);
};

#endif
