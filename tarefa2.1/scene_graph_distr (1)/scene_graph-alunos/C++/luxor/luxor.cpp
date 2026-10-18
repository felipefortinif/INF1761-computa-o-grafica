#include "luxor.h"
#include "mesh.h"
#include "transform.h"

Luxor::Luxor (WGPUDevice device)
{
  MeshPtr base_a = Mesh::Make(device, "../../meshes/luxor/base_a.msh");
  MeshPtr base_b = Mesh::Make(device, "../../meshes/luxor/base_b.msh");
  MeshPtr haste1 = Mesh::Make(device, "../../meshes/luxor/haste1.msh");
  MeshPtr haste2 = Mesh::Make(device, "../../meshes/luxor/haste2.msh");
  MeshPtr haste3_a = Mesh::Make(device, "../../meshes/luxor/haste3_a.msh");
  MeshPtr haste3_b = Mesh::Make(device, "../../meshes/luxor/haste3_b.msh");
  MeshPtr cupula_a = Mesh::Make(device, "../../meshes/luxor/cupula_a.msh");
  MeshPtr cupula_b = Mesh::Make(device, "../../meshes/luxor/cupula_b.msh");
  MeshPtr lampada = Mesh::Make(device, "../../meshes/luxor/lampada.msh");
  m_red = PhongMaterial::Make(1.0f,0.0f,0.0f);
  m_white = PhongMaterial::Make(1.0f,1.0f,1.0f);
  TransformPtr trf_all = Transform::Make();
  TransformPtr trf_base = Transform::Make();
  TransformPtr trf_haste1 = Transform::Make();
  TransformPtr trf_haste2 = Transform::Make();
  TransformPtr trf_haste3 = Transform::Make();
  TransformPtr trf_cupula = Transform::Make();
  TransformPtr trf_lampada = Transform::Make();
  trf_haste1->Translate(0.0f,4.0f,0.0f);
  trf_haste2->Translate(0.0f,17.15f,0.0f);
  trf_haste3->Translate(0.0f,16.78f,0.0f);
  trf_cupula->Translate(0.0f,18.12f,0.0f);
  trf_lampada->Translate(0.0f,8.4f,9.0f);
  m_light_node = Node::Make(trf_lampada,{m_white},{lampada});
  m_node = Node::Make(trf_all,{m_red},{
    Node::Make(trf_base,{base_a,base_b},{
      Node::Make(trf_haste1,{haste1},{
        Node::Make(trf_haste2,{haste2},{
          Node::Make(trf_haste3,{haste3_a,haste3_b},{
            Node::Make(trf_cupula,{cupula_a,cupula_b},{
              m_light_node
            })
          })
        })
      })
    })
  });
  m_engine = LuxorEngine::Make(trf_all,trf_base,trf_haste1,trf_haste2,trf_haste3,trf_cupula,trf_lampada);
}

LuxorPtr Luxor::Make (WGPUDevice device)
{
  return LuxorPtr(new Luxor(device));
}

Luxor::~Luxor ()
{
}

NodePtr Luxor::GetNode ()
{
  return m_node;
}

NodePtr Luxor::GetLightNode ()
{
  return m_light_node;
}

LuxorEnginePtr Luxor::GetEngine ()
{
  return m_engine;
}

std::vector<PhongMaterialPtr> Luxor::GetMaterials ()
{
  return {m_red, m_white};
}
