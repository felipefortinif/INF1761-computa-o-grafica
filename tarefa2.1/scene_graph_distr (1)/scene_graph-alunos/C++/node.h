#include <memory>
class Node;
using NodePtr = std::shared_ptr<Node>;

#ifndef NODE_H
#define NODE_H

#include "appearance.h"
#include "pipeline.h"
#include "shape.h"
#include "transform.h"
#include "graphicsmath.h"
#include <vector>
#include <initializer_list>

// A scene graph node: holds an optional Pipeline/Transform, this node's
// own Appearances and Shapes, and child Nodes. Pipeline, Transform, and
// Appearances are all inherited by descendants that don't set their own -
// only Shapes are strictly local to a Node.
class Node : public std::enable_shared_from_this<Node>
{
  std::weak_ptr<Node> m_parent;
  PipelinePtr m_pipeline;
  TransformPtr m_trf;
  std::vector<AppearancePtr> m_apps;
  std::vector<ShapePtr> m_shps;
  std::vector<NodePtr> m_nodes;
protected:
  Node (PipelinePtr pipeline=nullptr,
        TransformPtr trf=nullptr,
        std::initializer_list<AppearancePtr> apps={},
        std::initializer_list<ShapePtr> shps={}
       );
public:
  static NodePtr Make (PipelinePtr pipeline,
                       TransformPtr trf,
                       std::initializer_list<AppearancePtr> apps,
                       std::initializer_list<ShapePtr> shps,
                       std::initializer_list<NodePtr> nodes={}
                      );
  static NodePtr Make (PipelinePtr pipeline,
                       TransformPtr trf,
                       std::initializer_list<AppearancePtr> apps,
                       std::initializer_list<NodePtr> nodes={}
                      );
  static NodePtr Make (PipelinePtr pipeline,
                       TransformPtr trf,
                       std::initializer_list<NodePtr> nodes={}
                      );
  static NodePtr Make (PipelinePtr pipeline,
                       std::initializer_list<NodePtr> nodes={});
  static NodePtr Make (PipelinePtr pipeline,
                       std::initializer_list<AppearancePtr> apps,
                       std::initializer_list<ShapePtr> shps,
                       std::initializer_list<NodePtr> nodes={}
                      );
  static NodePtr Make (PipelinePtr pipeline,
                       std::initializer_list<AppearancePtr> apps,
                       std::initializer_list<NodePtr> nodes={}
                      );
  static NodePtr Make (PipelinePtr pipeline,
                       std::initializer_list<ShapePtr> shps,
                       std::initializer_list<NodePtr> nodes={}
                      );
  static NodePtr Make (PipelinePtr pipeline,
                       TransformPtr trf,
                       std::initializer_list<ShapePtr> shps,
                       std::initializer_list<NodePtr> nodes={}
                      );

  static NodePtr Make (TransformPtr trf,
                       std::initializer_list<AppearancePtr> apps,
                       std::initializer_list<ShapePtr> shps,
                       std::initializer_list<NodePtr> nodes={}
                      );
  static NodePtr Make (TransformPtr trf,
                       std::initializer_list<AppearancePtr> apps={},
                       std::initializer_list<NodePtr> nodes={}
                      );
  static NodePtr Make (std::initializer_list<NodePtr> nodes={});
  static NodePtr Make (std::initializer_list<AppearancePtr> apps,
                       std::initializer_list<ShapePtr> shps,
                       std::initializer_list<NodePtr> nodes={}
                      );
  static NodePtr Make (std::initializer_list<AppearancePtr> apps,
                       std::initializer_list<NodePtr> nodes={}
                      );
  static NodePtr Make (std::initializer_list<ShapePtr> shps,
                       std::initializer_list<NodePtr> nodes={}
                      );
  static NodePtr Make (TransformPtr trf,
                       std::initializer_list<ShapePtr> shps,
                       std::initializer_list<NodePtr> nodes={}
                      );
  static NodePtr Make (TransformPtr trf,
                       std::initializer_list<NodePtr> nodes
                      );
  virtual ~Node ();
  void SetPipeline (PipelinePtr pipeline);
  PipelinePtr GetPipeline () const;
  void SetTransform (TransformPtr trf);
  void AddAppearance (AppearancePtr app);
  void AddShape (ShapePtr shp);
  void AddNode (NodePtr node);
  void RemoveNode (NodePtr node);
  void SetParent (NodePtr parent);
  NodePtr GetParent () const;
  Mat4 GetMatrix () const;
  Mat4 GetModelMatrix ();
  void Render (StatePtr st);
};

#endif
