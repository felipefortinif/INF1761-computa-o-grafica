"""
Relógio Analógico com WebGPU (Python)
Trabalho Prático – INF1761 Computação Gráfica

Renderiza um relógio analógico usando a API WebGPU por meio da biblioteca
`wgpu-py` (bindings Python para wgpu-native), com shaders escritos em WGSL —
a mesma linguagem de shading da especificação WebGPU. A janela é aberta via
GLFW através da biblioteca `rendercanvas`.

Dependências (ver requirements.txt):
    pip install -r requirements.txt

Uso:
    python relogio.py
"""

import math
from datetime import datetime

import numpy as np
import wgpu

# ── Configuração da janela e do mostrador ──────────────────────────────────
WIDTH, HEIGHT = 600, 600
CX, CY = WIDTH / 2, HEIGHT / 2
R = min(WIDTH, HEIGHT) / 2 - 10  # raio do relógio
SEGS = 64  # segmentos usados para aproximar discos/anéis por triângulos
MAX_DYNAMIC_VERTS = 3000  # tamanho do buffer dinâmico (ponteiros + pino central)

# ── Shader WGSL ──────────────────────────────────────────────────────────
# Mesma linguagem (WGSL) e estrutura usadas por uma implementação WebGPU no
# navegador: um uniform com a resolução do canvas, vértices com posição
# (vec2f) e cor (vec4f), e conversão de pixels para clip-space no vertex shader.
SHADER_CODE = """
struct Uniforms {
    resolution : vec2f,
};

@group(0) @binding(0) var<uniform> uni : Uniforms;

struct VertIn {
    @location(0) pos   : vec2f,
    @location(1) color : vec4f,
};

struct VertOut {
    @builtin(position) position : vec4f,
    @location(0)       color    : vec4f,
};

@vertex
fn vs_main(v : VertIn) -> VertOut {
    var out : VertOut;
    // Converte de espaço em pixels para clip-space
    let clip = (v.pos / uni.resolution) * 2.0 - vec2f(1.0, 1.0);
    out.position = vec4f(clip.x, -clip.y, 0.0, 1.0);
    out.color = v.color;
    return out;
}

@fragment
fn fs_main(f : VertOut) -> @location(0) vec4f {
    return f.color;
}
"""


# ── Funções auxiliares de geometria ────────────────────────────────────────
# Toda a cena é composta apenas por triângulos (topologia triangle-list).

def circle_triangles(cx, cy, radius, segments, r, g, b, a):
    """Disco preenchido: leque (fan) de triângulos a partir do centro."""
    verts = []
    for i in range(segments):
        a0 = (i / segments) * math.tau
        a1 = ((i + 1) / segments) * math.tau
        verts += [cx, cy, r, g, b, a]
        verts += [cx + math.cos(a0) * radius, cy + math.sin(a0) * radius, r, g, b, a]
        verts += [cx + math.cos(a1) * radius, cy + math.sin(a1) * radius, r, g, b, a]
    return verts


def ring_triangles(cx, cy, inner_r, outer_r, segments, r, g, b, a):
    """Anel (annulus) usado na borda do relógio: cada segmento forma 2 triângulos."""
    verts = []
    for i in range(segments):
        a0 = (i / segments) * math.tau
        a1 = ((i + 1) / segments) * math.tau
        ix0, iy0 = cx + math.cos(a0) * inner_r, cy + math.sin(a0) * inner_r
        ix1, iy1 = cx + math.cos(a1) * inner_r, cy + math.sin(a1) * inner_r
        ox0, oy0 = cx + math.cos(a0) * outer_r, cy + math.sin(a0) * outer_r
        ox1, oy1 = cx + math.cos(a1) * outer_r, cy + math.sin(a1) * outer_r
        verts += [ix0, iy0, r, g, b, a, ox0, oy0, r, g, b, a, ox1, oy1, r, g, b, a]
        verts += [ix0, iy0, r, g, b, a, ox1, oy1, r, g, b, a, ix1, iy1, r, g, b, a]
    return verts


def hand_quad(cx, cy, angle, length, width_half, r, g, b, a):
    """Ponteiro poligonal (pentágono com rabicho) rotacionado por `angle`.

    `angle` = 0 corresponde às 12h (equivalente a -90° em coordenadas
    matemáticas padrão, tratado internamente pelo deslocamento de -pi/2).
    """
    rad = angle - math.pi / 2
    cos_a, sin_a = math.cos(rad), math.sin(rad)
    perp = (-sin_a, cos_a)

    tip_x, tip_y = cx + cos_a * length, cy + sin_a * length
    base1_x, base1_y = cx + perp[0] * width_half, cy + perp[1] * width_half
    base2_x, base2_y = cx - perp[0] * width_half, cy - perp[1] * width_half

    tail = length * 0.15  # pequena extensão atrás do centro
    tail1_x = cx - cos_a * tail + perp[0] * width_half * 0.6
    tail1_y = cy - sin_a * tail + perp[1] * width_half * 0.6
    tail2_x = cx - cos_a * tail - perp[0] * width_half * 0.6
    tail2_y = cy - sin_a * tail - perp[1] * width_half * 0.6

    return [
        base1_x, base1_y, r, g, b, a,
        base2_x, base2_y, r, g, b, a,
        tip_x, tip_y, r, g, b, a,

        tail1_x, tail1_y, r, g, b, a,
        tail2_x, tail2_y, r, g, b, a,
        base2_x, base2_y, r, g, b, a,

        tail1_x, tail1_y, r, g, b, a,
        base2_x, base2_y, r, g, b, a,
        base1_x, base1_y, r, g, b, a,
    ]


def tick_marks():
    """12 marcações de hora: retângulos finos (2 triângulos cada) na borda do mostrador."""
    verts = []
    for i in range(12):
        angle = (i / 12) * math.tau - math.pi / 2
        outer_r, inner_r, half_w = R - 2, R - 20, 4
        cos_a, sin_a = math.cos(angle), math.sin(angle)
        perp = (-sin_a, cos_a)

        ox1 = CX + cos_a * outer_r + perp[0] * half_w
        oy1 = CY + sin_a * outer_r + perp[1] * half_w
        ox2 = CX + cos_a * outer_r - perp[0] * half_w
        oy2 = CY + sin_a * outer_r - perp[1] * half_w
        ix1 = CX + cos_a * inner_r + perp[0] * half_w
        iy1 = CY + sin_a * inner_r + perp[1] * half_w
        ix2 = CX + cos_a * inner_r - perp[0] * half_w
        iy2 = CY + sin_a * inner_r - perp[1] * half_w

        verts += [ox1, oy1, 0, 0, 0, 1, ox2, oy2, 0, 0, 0, 1, ix2, iy2, 0, 0, 0, 1]
        verts += [ox1, oy1, 0, 0, 0, 1, ix2, iy2, 0, 0, 0, 1, ix1, iy1, 0, 0, 0, 1]
    return verts


def hand_angles(now):
    """Calcula os ângulos (em radianos, 0 = 12h) dos ponteiros para o instante `now`."""
    hr = now.hour % 12
    minute = now.minute
    sec = now.second
    ms = now.microsecond / 1000

    sec_angle = ((sec + ms / 1000) / 60) * math.tau
    min_angle = ((minute + (sec + ms / 1000) / 60) / 60) * math.tau
    hr_angle = ((hr + (minute + sec / 60) / 60) / 12) * math.tau
    return hr_angle, min_angle, sec_angle


def dynamic_vertices(now):
    """Monta a geometria dos ponteiros + pino central para o instante `now`."""
    hr_angle, min_angle, sec_angle = hand_angles(now)
    return [
        *hand_quad(CX, CY, hr_angle, R * 0.50, 10, 0, 0, 0, 1.0),          # ponteiro de hora
        *hand_quad(CX, CY, min_angle, R * 0.72, 6, 0, 0, 0, 1.0),          # ponteiro de minuto
        *hand_quad(CX, CY, sec_angle, R * 0.85, 2.5, 0.9, 0.1, 0.1, 1.0),  # ponteiro de segundo
        *circle_triangles(CX, CY, 7, 16, 0, 0, 0, 1.0),                   # pino central
    ]


def static_vertices():
    """Monta a geometria estática: face, borda e marcações de hora."""
    return [
        *circle_triangles(CX, CY, R, SEGS, 1.0, 1.0, 1.0, 1.0),       # face branca
        *ring_triangles(CX, CY, R - 6, R, SEGS, 0.0, 0.0, 0.0, 1.0),  # borda preta
        *tick_marks(),                                                # marcações de hora
    ]


# ── Configuração do pipeline WebGPU ────────────────────────────────────────

def create_pipeline(device, render_format):
    shader_module = device.create_shader_module(code=SHADER_CODE)
    return device.create_render_pipeline(
        layout="auto",
        vertex={
            "module": shader_module,
            "entry_point": "vs_main",
            "buffers": [
                {
                    "array_stride": 6 * 4,  # 2 floats de posição + 4 floats de cor
                    "attributes": [
                        {"shader_location": 0, "offset": 0, "format": "float32x2"},
                        {"shader_location": 1, "offset": 2 * 4, "format": "float32x4"},
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
                    "blend": {
                        "color": {"src_factor": "src-alpha", "dst_factor": "one-minus-src-alpha", "operation": "add"},
                        "alpha": {"src_factor": "one", "dst_factor": "one-minus-src-alpha", "operation": "add"},
                    },
                }
            ],
        },
        primitive={"topology": "triangle-list"},
    )


class Scene:
    """Recursos WebGPU do relógio: pipeline, buffers e bind group."""

    def __init__(self, device, render_format):
        self.device = device
        self.pipeline = create_pipeline(device, render_format)

        # Uniform buffer com a resolução do canvas
        self.uniform_buffer = device.create_buffer(
            size=8,  # vec2f = 8 bytes
            usage=wgpu.BufferUsage.UNIFORM | wgpu.BufferUsage.COPY_DST,
        )
        device.queue.write_buffer(self.uniform_buffer, 0, np.array([WIDTH, HEIGHT], dtype=np.float32).tobytes())

        self.bind_group = device.create_bind_group(
            layout=self.pipeline.get_bind_group_layout(0),
            entries=[{"binding": 0, "resource": {"buffer": self.uniform_buffer, "offset": 0, "size": 8}}],
        )

        # Geometria estática (face, borda, marcações): escrita uma única vez
        static_array = np.array(static_vertices(), dtype=np.float32)
        self.static_buffer = device.create_buffer_with_data(
            data=static_array.tobytes(), usage=wgpu.BufferUsage.VERTEX
        )
        self.static_count = static_array.size // 6

        # Geometria dinâmica (ponteiros + pino central): reescrita a cada quadro
        self.dynamic_buffer = device.create_buffer(
            size=MAX_DYNAMIC_VERTS * 6 * 4,
            usage=wgpu.BufferUsage.VERTEX | wgpu.BufferUsage.COPY_DST,
        )

    def draw(self, context):
        dyn_array = np.array(dynamic_vertices(datetime.now()), dtype=np.float32)
        self.device.queue.write_buffer(self.dynamic_buffer, 0, dyn_array.tobytes())
        dyn_count = dyn_array.size // 6

        encoder = self.device.create_command_encoder()
        view = context.get_current_texture().create_view()
        render_pass = encoder.begin_render_pass(
            color_attachments=[
                {
                    "view": view,
                    "clear_value": (0.53, 0.53, 0.53, 1.0),
                    "load_op": "clear",
                    "store_op": "store",
                }
            ]
        )
        render_pass.set_pipeline(self.pipeline)
        render_pass.set_bind_group(0, self.bind_group)

        render_pass.set_vertex_buffer(0, self.static_buffer)
        render_pass.draw(self.static_count)

        render_pass.set_vertex_buffer(0, self.dynamic_buffer)
        render_pass.draw(dyn_count)

        render_pass.end()
        self.device.queue.submit([encoder.finish()])


def main():
    from rendercanvas.auto import RenderCanvas, loop

    canvas = RenderCanvas(
        size=(WIDTH, HEIGHT),
        title="Relógio Analógico - WebGPU (Python)",
        update_mode="continuous",
        max_fps=60,
    )
    context = canvas.get_context("wgpu")

    adapter = wgpu.gpu.request_adapter_sync(canvas=canvas)
    device = adapter.request_device_sync()

    render_format = context.get_preferred_format(adapter)
    context.configure(device=device, format=render_format, alpha_mode="opaque")

    scene = Scene(device, render_format)
    canvas.request_draw(lambda: scene.draw(context))
    loop.run()


if __name__ == "__main__":
    main()
