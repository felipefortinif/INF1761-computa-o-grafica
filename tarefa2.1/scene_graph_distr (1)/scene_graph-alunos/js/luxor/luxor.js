// Builds the Luxo-lamp scene graph (base, arm segments, head, bulb) as
// nested Nodes, plus the matching LuxorEngine that animates it. Ported
// from python/luxor/luxor.py.

import { Node } from "../node.js";
import { Mesh } from "../mesh.js";
import { PhongMaterial } from "../phongmaterial.js";
import { Transform } from "../transform.js";
import { LuxorEngine } from "./luxorengine.js";

export class Luxor {
  // Use Luxor.load - fetching the 9 .msh mesh files is async, unlike
  // python's synchronous Mesh(device, filename).
  constructor (engine, node, lightNode, materials) {
    this.engine = engine;
    this.node = node;
    this.lightNode = lightNode;
    this.materials = materials;
  }

  // Loads meshes/materials, builds the Node hierarchy (node the root,
  // lightNode the bulb), and constructs the matching LuxorEngine.
  static async load (device) {
    const [baseA, baseB, haste1, haste2, haste3A, haste3B, cupulaA, cupulaB, lampada] = await Promise.all([
      Mesh.load(device, "../../meshes/luxor/base_a.msh"),
      Mesh.load(device, "../../meshes/luxor/base_b.msh"),
      Mesh.load(device, "../../meshes/luxor/haste1.msh"),
      Mesh.load(device, "../../meshes/luxor/haste2.msh"),
      Mesh.load(device, "../../meshes/luxor/haste3_a.msh"),
      Mesh.load(device, "../../meshes/luxor/haste3_b.msh"),
      Mesh.load(device, "../../meshes/luxor/cupula_a.msh"),
      Mesh.load(device, "../../meshes/luxor/cupula_b.msh"),
      Mesh.load(device, "../../meshes/luxor/lampada.msh"),
    ]);
    const red = new PhongMaterial(1.0, 0.0, 0.0);
    const white = new PhongMaterial(1.0, 1.0, 1.0);
    const materials = [red, white];

    const trfAll = new Transform();
    const trfBase = new Transform();
    const trfHaste1 = new Transform();
    const trfHaste2 = new Transform();
    const trfHaste3 = new Transform();
    const trfCupula = new Transform();
    const trfLampada = new Transform();
    trfHaste1.translate(0.0, 4.0, 0.0);
    trfHaste2.translate(0.0, 17.15, 0.0);
    trfHaste3.translate(0.0, 16.78, 0.0);
    trfCupula.translate(0.0, 18.12, 0.0);
    trfLampada.translate(0.0, 8.4, 9.0);

    const lightNode = new Node({ trf: trfLampada, apps: [white], shps: [lampada] });
    const node = new Node({
      trf: trfAll, apps: [red],
      nodes: [
        new Node({
          trf: trfBase, shps: [baseA, baseB],
          nodes: [
            new Node({
              trf: trfHaste1, shps: [haste1],
              nodes: [
                new Node({
                  trf: trfHaste2, shps: [haste2],
                  nodes: [
                    new Node({
                      trf: trfHaste3, shps: [haste3A, haste3B],
                      nodes: [
                        new Node({
                          trf: trfCupula, shps: [cupulaA, cupulaB],
                          nodes: [lightNode],
                        }),
                      ],
                    }),
                  ],
                }),
              ],
            }),
          ],
        }),
      ],
    });

    const engine = new LuxorEngine(trfAll, trfBase, trfHaste1, trfHaste2, trfHaste3, trfCupula, trfLampada);

    return new Luxor(engine, node, lightNode, materials);
  }

  getNode () {
    return this.node;
  }

  getLightNode () {
    return this.lightNode;
  }

  getEngine () {
    return this.engine;
  }

  // Returns every Material this Luxor lamp uses, so the caller can
  // shader.addMaterial(...) each of them once its Shader exists.
  getMaterials () {
    return this.materials;
  }
}
