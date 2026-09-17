"""
Sistema Sol–Terra–Lua com WebGPU (Python)
Tarefa 2 – INF1761 Computação Gráfica

Animação da vista superior do sistema: a terra descreve um movimento de
translação circular em torno do sol (que fica parado no centro da tela) e a
lua descreve um movimento circular em torno da terra.

A tarefa é resolvida usando **diretamente a API do WebGPU** (sem grafo de
cena): a geometria de um disco de raio 1 é enviada uma única vez para a GPU e,
a cada quadro, o código de atualização recalcula a posição de cada astro e
escreve os uniforms correspondentes. Um mesmo pipeline e uma mesma malha
desenham os três corpos, mudando apenas centro, raio e cor.

Usa a API WebGPU por meio da biblioteca `wgpu-py` (bindings para wgpu-native),
com shaders escritos em WGSL — a mesma linguagem da especificação WebGPU. A
janela é aberta via GLFW através da biblioteca `rendercanvas`.

Dependências (ver requirements.txt):
    pip install -r requirements.txt

Uso:
    python sistema_solar.py

Controles:
    espaço      pausa / retoma a animação
    + e -       acelera / desacelera
    r           reinicia
"""

import math
import time
from dataclasses import dataclass

import numpy as np
import wgpu

# ── Janela ────────────────────────────────────────────────────────────────
WIDTH, HEIGHT = 800, 800
# Cor de fundo em espaço linear: o canvas usa um formato sRGB, então a
# conversão para a tela clareia o valor (0.02 linear ≈ 15% de brilho na tela).
BACKGROUND = (0.00, 0.00, 0.000, 1.0)

# ── Dimensões da cena, em unidades de mundo ───────────────────────────────
# Não estão em escala real (a terra seria invisível ao lado do sol); foram
# escolhidas para a leitura do movimento ficar clara.
SUN_RADIUS = 1.20
EARTH_RADIUS = 0.45
MOON_RADIUS = 0.18

EARTH_ORBIT_RADIUS = 6.00  # raio da órbita da terra em torno do sol
MOON_ORBIT_RADIUS = 1.20   # raio da órbita da lua em torno da terra

# Metade da altura visível: cabe a órbita da terra somada à da lua.
WORLD_HALF_HEIGHT = 8.0

# ── Tempos da animação, em segundos ───────────────────────────────────────
EARTH_ORBIT_PERIOD = 18.0  # uma volta da terra em torno do sol ("um ano")
MOON_ORBIT_PERIOD = 1.5    # uma volta da lua em torno da terra ("um mês")
EARTH_SPIN_PERIOD = 2.0    # rotação da terra em torno do próprio eixo

# ── Cores ─────────────────────────────────────────────────────────────────
SUN_COLOR = (1.00, 0.30, 0.10, 1.00)
EARTH_COLOR = (0.10, 0.15, 1.00, 1.00)
MOON_COLOR = (0.80, 0.80, 0.84, 1.00)
ORBIT_COLOR = (1.00, 1.00, 1.00, 0.18)

# Desenha as trajetórias como anéis finos (apoio visual, não exigido pela tarefa)
SHOW_ORBITS = False

DISK_SEGMENTS = 64    # segmentos usados para aproximar o disco por triângulos
RING_SEGMENTS = 160

# ── Shader WGSL ───────────────────────────────────────────────────────────
# Cada objeto desenhado tem seu próprio uniform buffer com: a área visível
# (câmera ortográfica), o centro do astro em coordenadas de mundo, sua cor, seu
# raio e seu ângulo de rotação própria. O vertex shader posiciona a malha de
# raio 1 no mundo e converte para clip-space.
#
# Layout do uniform (regras de alinhamento do WGSL — 48 bytes no total):
#   half_extent  vec2f   offset  0
#   center       vec2f   offset  8
#   color        vec4f   offset 16
#   radius       f32     offset 32
#   angle        f32     offset 36
#   padding      vec2f   offset 40
SHADER_CODE = """
struct Uniforms {
    half_extent : vec2f,  // metade da largura/altura visível do mundo
    center      : vec2f,  // centro do astro, em coordenadas de mundo
    color       : vec4f,  // cor do astro
    radius      : f32,    // raio do astro, em unidades de mundo
    angle       : f32,    // rotação em torno do próprio centro
    padding     : vec2f,
};

@group(0) @binding(0) var<uniform> uni : Uniforms;

struct VertexIn {
    @location(0) position : vec2f,  // vértice da malha (disco de raio 1)
    @location(1) texcoord : vec2f,  // coordenada de textura
};

struct VertexOut {
    @builtin(position) position : vec4f,
    @location(0)       texcoord : vec2f,
};

@vertex
fn vs_main(v : VertexIn) -> VertexOut {
    // Rotação própria do astro
    let c = cos(uni.angle);
    let s = sin(uni.angle);
    let rotated = vec2f(v.position.x * c - v.position.y * s,
                        v.position.x * s + v.position.y * c);

    // Escala pelo raio e translada para o centro do astro
    let world = uni.center + rotated * uni.radius;

    var out : VertexOut;
    // Projeção ortográfica: a área visível é [-half_extent, +half_extent]
    out.position = vec4f(world / uni.half_extent, 0.0, 1.0);
    // A coordenada de textura ainda não é usada, mas já é interpolada e
    // repassada ao fragment shader pensando na aplicação de texturas.
    out.texcoord = v.texcoord;
    return out;
}

@fragment
fn fs_main(f : VertexOut) -> @location(0) vec4f {
    return uni.color;
}
"""

UNIFORM_SIZE = 48  # bytes, conforme o layout descrito acima


# ── Geometria ─────────────────────────────────────────────────────────────

@dataclass
class Mesh:
    """Malha já residente na GPU: buffers de vértices e de índices."""

    vertex_buffer: wgpu.GPUBuffer
    index_buffer: wgpu.GPUBuffer
    index_count: int


def create_mesh(device, vertices, indices):
    """Envia vértices (x, y, u, v) e índices para a GPU."""
    vertex_data = np.asarray(vertices, dtype=np.float32)
    index_data = np.asarray(indices, dtype=np.uint32)
    return Mesh(
        vertex_buffer=device.create_buffer_with_data(
            data=vertex_data.tobytes(), usage=wgpu.BufferUsage.VERTEX
        ),
        index_buffer=device.create_buffer_with_data(
            data=index_data.tobytes(), usage=wgpu.BufferUsage.INDEX
        ),
        index_count=index_data.size,
    )


def create_disk(device, segments=DISK_SEGMENTS):
    """Disco de raio 1 centrado na origem — a malha dos três astros.

    A malha é um leque de triângulos: um vértice no centro mais `segments`
    vértices na borda. Como o WebGPU não possui a topologia `triangle-fan`,
    o leque é montado com `triangle-list` + buffer de índices, em que cada
    segmento reaproveita o vértice central e dois vértices consecutivos da
    borda.

    O raio é 1 de propósito: o tamanho de cada astro é dado pelo uniform
    `radius`, e assim uma única malha serve para o sol, a terra e a lua.

    Coordenadas de textura: o disco é inscrito no quadrado [0,1]x[0,1] — o
    vértice da borda no ângulo `a` recebe (0.5 + 0.5·cos a, 0.5 + 0.5·sin a) e
    o centro recebe (0.5, 0.5). É o mapeamento natural para, no futuro, colar
    uma textura circular (a face de um planeta) sobre o disco.
    """
    vertices = [0.0, 0.0, 0.5, 0.5]  # centro
    for i in range(segments):
        angle = (i / segments) * math.tau
        x, y = math.cos(angle), math.sin(angle)
        vertices += [x, y, 0.5 + 0.5 * x, 0.5 + 0.5 * y]

    indices = []
    for i in range(segments):
        indices += [0, 1 + i, 1 + (i + 1) % segments]

    return create_mesh(device, vertices, indices)


def create_ring(device, radius, thickness, segments=RING_SEGMENTS):
    """Anel fino de raio `radius` — usado só para mostrar as trajetórias.

    Diferente do disco, já é criado com as dimensões finais em unidades de
    mundo (é desenhado com radius = 1) para que a espessura do traço não
    acompanhe o tamanho da órbita.

    Coordenadas de textura: u percorre a volta (0 a 1) e v vai de 0 na borda
    interna a 1 na externa.
    """
    inner = radius - thickness / 2.0
    outer = radius + thickness / 2.0

    vertices = []
    for i in range(segments):
        angle = (i / segments) * math.tau
        c, s = math.cos(angle), math.sin(angle)
        u = i / segments
        vertices += [c * inner, s * inner, u, 0.0]
        vertices += [c * outer, s * outer, u, 1.0]

    indices = []
    for i in range(segments):
        i0 = 2 * i
        o0 = i0 + 1
        i1 = 2 * ((i + 1) % segments)  # fecha o anel no último segmento
        o1 = i1 + 1
        indices += [i0, o0, o1, i0, o1, i1]

    return create_mesh(device, vertices, indices)


# ── Recursos WebGPU ───────────────────────────────────────────────────────

def create_pipeline(device, render_format):
    shader_module = device.create_shader_module(code=SHADER_CODE)
    return device.create_render_pipeline(
        layout="auto",
        vertex={
            "module": shader_module,
            "entry_point": "vs_main",
            "buffers": [
                {
                    "array_stride": 4 * 4,  # 2 floats de posição + 2 de textura
                    "attributes": [
                        {"shader_location": 0, "offset": 0, "format": "float32x2"},
                        {"shader_location": 1, "offset": 2 * 4, "format": "float32x2"},
                    ],
                }
            ],
        },
        fragment={
            "module": shader_module,
            "entry_point": "fs_main",
            "targets": [
                {
                    "format": render_format,
                    # Blend habilitado por causa das órbitas semitransparentes.
                    "blend": {
                        "color": {
                            "src_factor": "src-alpha",
                            "dst_factor": "one-minus-src-alpha",
                            "operation": "add",
                        },
                        "alpha": {
                            "src_factor": "one",
                            "dst_factor": "one-minus-src-alpha",
                            "operation": "add",
                        },
                    },
                }
            ],
        },
        primitive={"topology": "triangle-list"},
    )


def create_uniform_slots(device, pipeline, count):
    """Cria um uniform buffer + bind group para cada objeto desenhado no quadro."""
    layout = pipeline.get_bind_group_layout(0)
    slots = []
    for _ in range(count):
        buffer = device.create_buffer(
            size=UNIFORM_SIZE,
            usage=wgpu.BufferUsage.UNIFORM | wgpu.BufferUsage.COPY_DST,
        )
        bind_group = device.create_bind_group(
            layout=layout,
            entries=[
                {
                    "binding": 0,
                    "resource": {"buffer": buffer, "offset": 0, "size": UNIFORM_SIZE},
                }
            ],
        )
        slots.append((buffer, bind_group))
    return slots


def pack_uniforms(half_extent, center, color, radius, angle):
    """Monta os 48 bytes do uniform de um objeto."""
    return np.array(
        [
            half_extent[0], half_extent[1],
            center[0], center[1],
            color[0], color[1], color[2], color[3],
            radius,
            angle,
            0.0, 0.0,  # padding
        ],
        dtype=np.float32,
    ).tobytes()


# ── Atualização da animação ───────────────────────────────────────────────

def update(t):
    """Estado do sistema no instante `t` (em segundos de simulação).

    Movimento circular uniforme: o ângulo cresce linearmente com o tempo e a
    posição sai de (cos, sin) multiplicados pelo raio da órbita.

    * o sol fica fixo na origem — o centro da tela;
    * a terra gira em torno do sol, a EARTH_ORBIT_RADIUS da origem;
    * a lua gira em torno da terra: sua órbita é calculada no referencial da
      terra e somada à posição dela, de modo que a lua acompanha a terra ao
      longo de toda a translação.
    """
    earth_angle = math.tau * (t / EARTH_ORBIT_PERIOD)
    earth = (
        EARTH_ORBIT_RADIUS * math.cos(earth_angle),
        EARTH_ORBIT_RADIUS * math.sin(earth_angle),
    )

    moon_angle = math.tau * (t / MOON_ORBIT_PERIOD)
    moon = (
        earth[0] + MOON_ORBIT_RADIUS * math.cos(moon_angle),
        earth[1] + MOON_ORBIT_RADIUS * math.sin(moon_angle),
    )

    # Rotação da terra em torno do próprio eixo: só ficará visível quando o
    # disco receber uma textura, mas o shader já a aplica.
    earth_spin = math.tau * (t / EARTH_SPIN_PERIOD)

    return earth, moon, earth_spin


# ── Desenho ───────────────────────────────────────────────────────────────

def draw_frame(device, context, pipeline, meshes, slots, t):
    disk, earth_orbit, moon_orbit = meshes
    earth, moon, earth_spin = update(t)

    texture = context.get_current_texture()
    # A câmera mantém WORLD_HALF_HEIGHT unidades visíveis na vertical e ajusta
    # a horizontal pela proporção da janela, para os discos não deformarem
    # quando a janela é redimensionada.
    aspect = texture.width / texture.height
    half_extent = (WORLD_HALF_HEIGHT * aspect, WORLD_HALF_HEIGHT)

    # Objetos do quadro, na ordem de desenho (não há teste de profundidade):
    # as órbitas ao fundo e os astros por cima.
    objects = []
    if SHOW_ORBITS:
        objects += [
            (earth_orbit, (0.0, 0.0), ORBIT_COLOR, 1.0, 0.0),
            (moon_orbit, earth, ORBIT_COLOR, 1.0, 0.0),
        ]
    objects += [
        (disk, (0.0, 0.0), SUN_COLOR, SUN_RADIUS, 0.0),
        (disk, earth, EARTH_COLOR, EARTH_RADIUS, earth_spin),
        (disk, moon, MOON_COLOR, MOON_RADIUS, 0.0),
    ]

    # Atualiza os uniforms de todos os objetos (cada um tem o seu buffer).
    for (_, center, color, radius, angle), (buffer, _) in zip(objects, slots):
        device.queue.write_buffer(
            buffer, 0, pack_uniforms(half_extent, center, color, radius, angle)
        )

    encoder = device.create_command_encoder()
    render_pass = encoder.begin_render_pass(
        color_attachments=[
            {
                "view": texture.create_view(),
                "clear_value": BACKGROUND,
                "load_op": "clear",
                "store_op": "store",
            }
        ]
    )
    render_pass.set_pipeline(pipeline)

    for (mesh, *_), (_, bind_group) in zip(objects, slots):
        render_pass.set_bind_group(0, bind_group)
        render_pass.set_vertex_buffer(0, mesh.vertex_buffer)
        render_pass.set_index_buffer(mesh.index_buffer, wgpu.IndexFormat.uint32)
        render_pass.draw_indexed(mesh.index_count)

    render_pass.end()
    device.queue.submit([encoder.finish()])


# ── Aplicação ─────────────────────────────────────────────────────────────

def main():
    from rendercanvas.auto import RenderCanvas, loop

    canvas = RenderCanvas(
        size=(WIDTH, HEIGHT),
        title="Sistema Sol-Terra-Lua - WebGPU (Python)",
        update_mode="continuous",
        max_fps=60,
    )
    context = canvas.get_context("wgpu")

    adapter = wgpu.gpu.request_adapter_sync(canvas=canvas)
    device = adapter.request_device_sync()

    render_format = context.get_preferred_format(adapter)
    context.configure(device=device, format=render_format, alpha_mode="opaque")

    pipeline = create_pipeline(device, render_format)
    meshes = (
        create_disk(device),
        create_ring(device, EARTH_ORBIT_RADIUS, thickness=0.03),
        create_ring(device, MOON_ORBIT_RADIUS, thickness=0.02),
    )
    slots = create_uniform_slots(device, pipeline, count=5)

    # Relógio da animação: tempo de simulação acumulado a partir do tempo real.
    clock = {"time": 0.0, "speed": 1.0, "paused": False, "last": None}

    def tick():
        now = time.perf_counter()
        if clock["last"] is None:
            dt = 0.0
        else:
            # Limita o passo para a animação não "saltar" depois de a janela
            # ficar bloqueada (arrastada, minimizada, etc.).
            dt = min(now - clock["last"], 0.1)
        clock["last"] = now
        if not clock["paused"]:
            clock["time"] += dt * clock["speed"]
        draw_frame(device, context, pipeline, meshes, slots, clock["time"])

    def on_key(event):
        key = event.get("key", "")
        if key in (" ", "Space"):
            clock["paused"] = not clock["paused"]
        elif key in ("+", "="):
            clock["speed"] = min(clock["speed"] * 1.5, 32.0)
        elif key in ("-", "_"):
            clock["speed"] = max(clock["speed"] / 1.5, 1.0 / 32.0)
        elif key in ("r", "R"):
            clock["time"] = 0.0
            clock["speed"] = 1.0
            clock["paused"] = False

    canvas.add_event_handler(on_key, "key_down")
    canvas.request_draw(tick)
    loop.run()


if __name__ == "__main__":
    main()
