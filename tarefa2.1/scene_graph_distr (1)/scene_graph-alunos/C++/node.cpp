#include "node.h"
#include "error.h"
#include <algorithm>

Node::Node (PipelinePtr pipeline, TransformPtr trf,
            std::initializer_list<AppearancePtr> apps,
            std::initializer_list<ShapePtr> shps
           )
: m_parent(),
  m_pipeline(pipeline),
  m_trf(trf),
  m_apps(apps),
  m_shps(shps),
  m_nodes()
{
}
NodePtr Node::Make (PipelinePtr pipeline,
                    TransformPtr trf,
                    std::initializer_list<AppearancePtr> apps,
                    std::initializer_list<ShapePtr> shps,
                    std::initializer_list<NodePtr> nodes
                   )
{
  NodePtr ptr(new Node(pipeline,trf,apps,shps));
  for (auto node : nodes)
    ptr->AddNode(node);
  return ptr;
}
NodePtr Node::Make (PipelinePtr pipeline,
                    TransformPtr trf,
                    std::initializer_list<AppearancePtr> apps,
                    std::initializer_list<NodePtr> nodes
                   )
{
  NodePtr ptr(new Node(pipeline,trf,apps,{}));
  for (auto node : nodes)
    ptr->AddNode(node);
  return ptr;
}
NodePtr Node::Make (PipelinePtr pipeline,
                    TransformPtr trf,
                    std::initializer_list<NodePtr> nodes
                   )
{
  NodePtr ptr(new Node(pipeline,trf,{},{}));
  for (auto node : nodes)
    ptr->AddNode(node);
  return ptr;
}
NodePtr Node::Make (PipelinePtr pipeline,
                    std::initializer_list<NodePtr> nodes
                   )
{
  NodePtr ptr(new Node(pipeline,nullptr,{},{}));
  for (auto node : nodes)
    ptr->AddNode(node);
  return ptr;
}
NodePtr Node::Make (PipelinePtr pipeline,
                    std::initializer_list<AppearancePtr> apps,
                    std::initializer_list<ShapePtr> shps,
                    std::initializer_list<NodePtr> nodes
                   )
{
  NodePtr ptr(new Node(pipeline,nullptr,apps,shps));
  for (auto node : nodes)
    ptr->AddNode(node);
  return ptr;
}
NodePtr Node::Make (PipelinePtr pipeline,
                    std::initializer_list<AppearancePtr> apps,
                    std::initializer_list<NodePtr> nodes
                   )
{
  NodePtr ptr(new Node(pipeline,nullptr,apps,{}));
  for (auto node : nodes)
    ptr->AddNode(node);
  return ptr;
}
NodePtr Node::Make (PipelinePtr pipeline,
                    std::initializer_list<ShapePtr> shps,
                    std::initializer_list<NodePtr> nodes
                   )
{
  NodePtr ptr(new Node(pipeline,nullptr,{},shps));
  for (auto node : nodes)
    ptr->AddNode(node);
  return ptr;
}
NodePtr Node::Make (PipelinePtr pipeline, TransformPtr trf,
                    std::initializer_list<ShapePtr> shps,
                    std::initializer_list<NodePtr> nodes
                   )
{
  NodePtr ptr(new Node(pipeline,trf,{},shps));
  for (auto node : nodes)
    ptr->AddNode(node);
  return ptr;
}
NodePtr Node::Make (TransformPtr trf,
                    std::initializer_list<AppearancePtr> apps,
                    std::initializer_list<ShapePtr> shps,
                    std::initializer_list<NodePtr> nodes
                   )
{
  NodePtr ptr(new Node(nullptr,trf,apps,shps));
  for (auto node : nodes)
    ptr->AddNode(node);
  return ptr;
}
NodePtr Node::Make (TransformPtr trf,
                    std::initializer_list<AppearancePtr> apps,
                    std::initializer_list<NodePtr> nodes
                   )
{
  NodePtr ptr(new Node(nullptr,trf,apps,{}));
  for (auto node : nodes)
    ptr->AddNode(node);
  return ptr;
}
NodePtr Node::Make (std::initializer_list<NodePtr> nodes)
{
  NodePtr ptr(new Node(nullptr,nullptr,{},{}));
  for (auto node : nodes)
    ptr->AddNode(node);
  return ptr;
}
NodePtr Node::Make (std::initializer_list<AppearancePtr> apps,
                    std::initializer_list<ShapePtr> shps,
                    std::initializer_list<NodePtr> nodes
                   )
{
  NodePtr ptr(new Node(nullptr,nullptr,apps,shps));
  for (auto node : nodes)
    ptr->AddNode(node);
  return ptr;
}
NodePtr Node::Make (std::initializer_list<AppearancePtr> apps,
                    std::initializer_list<NodePtr> nodes
                   )
{
  NodePtr ptr(new Node(nullptr,nullptr,apps,{}));
  for (auto node : nodes)
    ptr->AddNode(node);
  return ptr;
}
NodePtr Node::Make (std::initializer_list<ShapePtr> shps,
                    std::initializer_list<NodePtr> nodes
                   )
{
  NodePtr ptr(new Node(nullptr,nullptr,{},shps));
  for (auto node : nodes)
    ptr->AddNode(node);
  return ptr;
}
NodePtr Node::Make (TransformPtr trf,
                    std::initializer_list<ShapePtr> shps,
                    std::initializer_list<NodePtr> nodes
                   )
{
  NodePtr ptr(new Node(nullptr,trf,{},shps));
  for (auto node : nodes)
    ptr->AddNode(node);
  return ptr;
}

NodePtr Node::Make (TransformPtr trf,
                    std::initializer_list<NodePtr> nodes
                   )
{
  NodePtr ptr(new Node(nullptr,trf,{},{}));
  for (auto node : nodes)
    ptr->AddNode(node);
  return ptr;
}

Node::~Node ()
{
}

void Node::SetPipeline (PipelinePtr pipeline)
{
  m_pipeline = pipeline;
}
PipelinePtr Node::GetPipeline () const
{
  return m_pipeline;
}
void Node::SetTransform (TransformPtr trf)
{
  m_trf = trf;
}
void Node::AddAppearance (AppearancePtr app)
{
  m_apps.push_back(app);
}
void Node::AddShape (ShapePtr shp)
{
  m_shps.push_back(shp);
}
void Node::AddNode (NodePtr node)
{
  if (node->GetParent() != nullptr)
    Error::Fatal("Node already has a parent - remove it from its current parent first");
  for (NodePtr ancestor = shared_from_this(); ancestor != nullptr; ancestor = ancestor->GetParent())
    if (ancestor == node)
      Error::Fatal("Node is self or an ancestor of self - would create a cycle");
  m_nodes.push_back(node);
  node->SetParent(shared_from_this());
}
void Node::RemoveNode (NodePtr node)
{
  auto it = std::find(m_nodes.begin(), m_nodes.end(), node);
  if (it == m_nodes.end())
    Error::Fatal("node is not a child of this Node");
  m_nodes.erase(it);
  node->SetParent(nullptr);
}
void Node::SetParent (NodePtr parent)
{
  m_parent = parent;
}
NodePtr Node::GetParent () const
{
  return m_parent.lock();
}
Mat4 Node::GetMatrix () const
{
  return m_trf ? m_trf->GetMatrix() : Mat4Identity();
}
Mat4 Node::GetModelMatrix ()
{
  Mat4 mat = GetMatrix();
  NodePtr node = GetParent();
  while (node != nullptr) {
    mat = Mat4Multiply(node->GetMatrix(), mat);
    node = node->GetParent();
  }
  return mat;
}
void Node::Render (StatePtr st)
{
  // load
  if (m_pipeline)
    m_pipeline->Load(st);
  if (m_trf)
    m_trf->Load(st);
  for (AppearancePtr app : m_apps)
    app->Load(st);
  // draw - whichever Pipeline is current is already bound on the render
  // pass: Pipeline::Load binds it on the way in, and a nested Pipeline's
  // Unload rebinds the enclosing one on the way out
  if (!m_shps.empty()) {
    st->LoadMatrices();
    for (ShapePtr shp : m_shps)
      shp->Draw(st);
    st->UnloadMatrices();
  }
  for (NodePtr node : m_nodes)
    node->Render(st);
  // unload in reverse order
  for (auto it = m_apps.rbegin(); it != m_apps.rend(); ++it)
    (*it)->Unload(st);
  if (m_trf)
    m_trf->Unload(st);
  if (m_pipeline)
    m_pipeline->Unload(st);
}
