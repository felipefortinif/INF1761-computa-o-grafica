#include <memory>
class Luxor;
using LuxorPtr = std::shared_ptr<Luxor>;

#ifndef LUXO_H
#define LUXO_H

#include <wgpu.h>
#include <vector>
#include "node.h"
#include "phongmaterial.h"
#include "luxorengine.h"

class Luxor
{
  NodePtr m_node;
  NodePtr m_light_node;
  LuxorEnginePtr m_engine;
  PhongMaterialPtr m_red, m_white;
protected:
  Luxor (WGPUDevice device);
public:
  virtual ~Luxor ();
  static LuxorPtr Make (WGPUDevice device);
  NodePtr GetNode ();
  NodePtr GetLightNode ();
  LuxorEnginePtr GetEngine ();
  std::vector<PhongMaterialPtr> GetMaterials ();
};

#endif
