# Projeto 1 — Mini-sistema solar 2D (INF1761 - Computação Gráfica)

Animação de um mini-sistema solar bidimensional (Sol, Mercúrio, Terra e Lua)
modelada com um **grafo de cena**, com texturas nos astros e no fundo do
espaço. Desenvolvido em **Python** com WebGPU via
[`wgpu-py`](https://github.com/pygfx/wgpu-py).

A pasta é **autossuficiente**: além dos arquivos do trabalho, ela já traz os
módulos do framework `scene_graph` de que depende, então roda sozinha, sem
precisar de nenhuma outra pasta.

## Como executar

Pré-requisitos (Python 3.10+):

```bash
pip install wgpu rendercanvas glfw numpy pillow
```

Rodar a animação (abre uma janela; tecle **Q** ou **ESC** para sair):

```bash
cd solar_system
python main_solar.py
```

Na primeira execução, as texturas dos astros são geradas automaticamente em
`images/solar/`. Para gerar uma imagem estática (sem abrir janela), útil para
o relatório:

```bash
python screenshot.py 3.5 sistema_solar.png     # instante t = 3.5 s
```

## Requisitos do enunciado × implementação

| Requisito | Onde/como |
|---|---|
| Grafo de cena modelando o sistema | `build_scene` em `main_solar.py` |
| Translação da Terra em torno do Sol | nó-pivô `orbita_terra` (engine `Rotate`) |
| Rotação da Terra no próprio eixo | shader do globo (`earth.wgsl`), engine `SpinGlobe` |
| A Lua **não** herda a rotação da Terra | `orbita_lua` é **irmã** do disco da Terra (ambos sob `pos_terra`) |
| Translação da Lua em torno da Terra | nó-pivô `orbita_lua` |
| Astro extra entre Sol e Terra | **Mercúrio**, com sua própria órbita |
| Texturas nos astros | `textured.wgsl` + `TextureSet` por astro |
| Textura da Terra identifica a rotação | globo (`earth.wgsl`) que gira no eixo revelando **todos** os continentes |
| Textura de fundo do espaço | retângulo grande com `space_bg.png` |

## Estrutura do grafo de cena

```
root  (Pipeline com o shader de textura)
├── fundo                       (retângulo do espaço)
└── sol                         (na origem)
    ├── giro_sol                → disco do Sol
    ├── orbita_mercurio         → pos_mercurio → giro_mercurio → disco
    └── orbita_terra            → pos_terra
                                   ├── disco da Terra   (pipeline do globo)
                                   └── orbita_lua → pos_lua → disco da Lua
```

As **órbitas** vêm da **hierarquia de transformações**: um nó-pivô que só gira
(`orbita_*`) faz os filhos deslocados descreverem uma órbita. A **rotação da
Terra no próprio eixo** é feita no shader do globo (`earth.wgsl`), que a cada
quadro recebe um deslocamento de longitude (`spin`): a esfera gira mostrando
todos os continentes, em vez de um disco fixo girando no plano (que exibiria
sempre o mesmo hemisfério). Como esse giro é local ao nó da Terra (vive no seu
shader) e `orbita_lua` é irmã do disco da Terra (e não filha), a Lua acompanha
a **posição** orbital da Terra mas nunca a sua **rotação de eixo** — exatamente
o que o enunciado pede.

A animação é feita por engines: `Rotate` (giro de uma transformação em torno de
Z, para órbitas e giro do Sol/Mercúrio) e `SpinGlobe` (acumula a longitude da
Terra e a envia ao shader). Todas avançam proporcionalmente ao `dt`.

## Arquivos

Arquivos desenvolvidos para o trabalho (na raiz):

| Arquivo | Descrição |
|---|---|
| `main_solar.py` | Aplicação: monta o grafo de cena, as engines e o laço de render |
| `gen_textures.py` | Gera as texturas procedurais + prepara o mapa equiretangular da Terra |
| `screenshot.py` | Render offscreen para gerar figuras (não abre janela) |
| `shaders/textured.wgsl` | Shader 2D com mapeamento de textura (Sol, Mercúrio, Lua, fundo) |
| `shaders/earth.wgsl` | Shader do globo terrestre: projeção da esfera + giro no eixo (longitude) |
| `images/earth.jpg` | Mapa da Terra (fonte do globo) |
| `images/solar/` | Texturas geradas (Sol, Terra, Lua, Mercúrio, fundo) |

A pasta `python/` contém somente os módulos do framework `scene_graph` (cópia
dos arquivos de `scene_graph-alunos/python` necessários), incluídos para a
pasta rodar sozinha.

## Detalhes de implementação

- **Astros como discos**: cada astro é um `Quad` (quadrado `[0,1]²`) texturizado.
  As texturas dos astros têm os cantos transparentes (fora do disco); o shader
  descarta fragmentos com `alpha < 0.01`, e o pipeline usa *alpha blending*
  para bordas suaves e para a corona translúcida do Sol.
- **Terra como globo giratório**: o shader `earth.wgsl` recebe o mapa
  equiretangular completo (`earth_map.png`, derivado de `earth.jpg`) e, por
  fragmento, projeta a esfera (projeção ortográfica) somando um deslocamento de
  longitude animado. Com sampler em modo *repeat* no eixo U, a longitude
  "dá a volta" e, ao longo de uma rotação, **todos os continentes** passam pela
  face visível — enquanto uma luz fixa dá o aspecto de esfera iluminada.
- **Sol, Lua e Mercúrio**: gerados proceduralmente (gradiente radial + ruído
  para o Sol; crateras + sombreamento esférico para os corpos rochosos).
