#include <memory>
class Scene;
using ScenePtr = std::shared_ptr<Scene>;

#ifndef SCENE_H
#define SCENE_H

#include "node.h"
#include "engine.h"
#include "state.h"
#include <vector>

// Top-level container: a root Node plus the Engines (simulation/logic
// updated once per frame, e.g. animation) that drive it.
class Scene
{
  NodePtr m_root;
  std::vector<EnginePtr> m_engines;
protected:
  Scene (NodePtr root);
public:
  static ScenePtr Make (NodePtr root);
  ~Scene ();
  NodePtr GetRoot () const;
  void AddEngine (EnginePtr engine);
  void Update (float dt) const;
  void Render (StatePtr st);
};

#endif
