#include "scene.h"

Scene::Scene (NodePtr root)
: m_root(root)
{
}

ScenePtr Scene::Make (NodePtr root)
{
  return ScenePtr(new Scene(root));
}
Scene::~Scene ()
{
}

NodePtr Scene::GetRoot () const
{
  return m_root;
}

void Scene::AddEngine (EnginePtr engine)
{
  m_engines.push_back(engine);
}

void Scene::Update (float dt) const
{
  for (auto e : m_engines)
    e->Update(dt);
}

void Scene::Render (StatePtr st)
{
  m_root->Render(st);
}
