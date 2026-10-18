// Iluminacao por fragmento (Phong shading) com uma fonte de luz de spot.
// O shader trabalha no espaco da camera: "vertex" leva o vertice do espaco
// do objeto para o espaco da camera, a posicao e a direcao do spot chegam ja
// mapeadas para esse espaco (ver SpotLight.map) e o observador fica na origem.
// vs_main so transforma posicao/normal e entrega os vetores normal, luz e
// vista interpolados; o coeficiente de spot e a formula de Phong (ambiente +
// difusa + especular) sao avaliados em fs_main, por pixel.

struct Matrix {
  vertex: mat4x4<f32>,
  normal: mat4x4<f32>,
}
@group(1) @binding(0) var<storage, read> matrix: array<Matrix>;

struct Global {
  projection: mat4x4<f32>,   // espaco da camera -> NDC
  light_ambient: vec3<f32>,
  light_diffuse: vec3<f32>,
  light_specular: vec3<f32>,
  light_position: vec4<f32>, // s_pos, no espaco da camera
  spot_direction: vec3<f32>, // s_dir, unitario, no espaco da camera
  spot_cutoff: f32,          // cos(theta)
  spot_exponent: f32,        // s_exp
}
@group(0) @binding(0) var<uniform> global: Global;

struct Material {
  base_color: vec3<f32>,
  opacity: f32,

  specular_color: vec3<f32>,
  shininess: f32,
}
@group(2) @binding(0) var<uniform> material: Material;

struct VertexOutput {
  @builtin(position) clip_position: vec4<f32>,
  @location(0) normal: vec3<f32>,
  @location(1) light: vec3<f32>,
  @location(2) view: vec3<f32>,
}

@vertex
fn vs_main (@builtin(instance_index) instance_index: u32, @location(0) coord: vec3<f32>, @location(1) normal: vec3<f32>) -> VertexOutput {
  let m = matrix[instance_index];
  var out: VertexOutput;
  let veye = (m.vertex * vec4<f32>(coord, 1.0)).xyz;
  // nao normaliza aqui: s_pos - p varia linearmente sobre o triangulo, entao
  // a interpolacao e exata e o vetor so e normalizado no fragmento
  out.light = global.light_position.xyz - veye;
  out.view = -veye;
  out.normal = (m.normal * vec4<f32>(normal, 0.0)).xyz;
  out.clip_position = global.projection * vec4<f32>(veye, 1.0);
  return out;
}

// k_spot = max(c,0)^s_exp se c >= cos(theta), 0 caso contrario,
// com c = -l . s_dir (cosseno entre o eixo do spot e a direcao fonte -> ponto)
fn spot_coefficient (light: vec3<f32>) -> f32 {
  let c = dot(-light, global.spot_direction);
  if (c < global.spot_cutoff) {
    return 0.0;
  }
  if (global.spot_exponent == 0.0) {
    return 1.0;   // intensidade constante no cone (pow(0,0) e indefinido em WGSL)
  }
  return pow(max(c, 0.0), global.spot_exponent);
}

@fragment
fn fs_main (in: VertexOutput) -> @location(0) vec4<f32> {
  let normal = normalize(in.normal);
  let light = normalize(in.light);
  let view = normalize(in.view);

  // a parcela ambiente nao depende do cone: faz o papel de luz ambiente da
  // cena e mantem visivel o que esta fora do spot
  var color = material.base_color * global.light_ambient;
  let kspot = spot_coefficient(light);
  let ndotl = dot(normal, light);
  if (kspot > 0.0 && ndotl > 0.0) {
    // k_spot multiplica a intensidade (cor) da fonte
    color += material.base_color * (kspot * global.light_diffuse) * ndotl;
    let refl = reflect(-light, normal);
    color += material.specular_color * (kspot * global.light_specular) * pow(max(0.0, dot(refl, view)), material.shininess);
  }
  return vec4<f32>(color, material.opacity);
}
