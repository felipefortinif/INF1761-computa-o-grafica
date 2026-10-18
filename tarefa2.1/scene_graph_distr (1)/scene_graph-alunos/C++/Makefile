
TARGET2 = t2
TARGET3 = t3

all: $(TARGET2) $(TARGET3)

INC = -I. -I/opt/homebrew/include
LIB = -L. -L/opt/homebrew/lib

CPP = clang++
CPPFLAGS = -g -Wall -std=c++20 -DGL_SILENCE_DEPRECATION
OBJCFLAGS = -x objective-c -fno-objc-arc -I/opt/homebrew/include -DGLFW_EXPOSE_NATIVE_COCOA

LDFLAGS = -lwgpu_native -lglfw -lobjc -framework QuartzCore -framework Metal \
          -framework CoreVideo -framework IOKit -framework IOSurface -framework Cocoa

CORE_SRC = \
  wgpucontext.cpp \
  error.cpp \
  graphicsmath.cpp \
  wgslreflect.cpp \
  uniformbuffer.cpp \
  state.cpp \
  shader.cpp \
  pipeline.cpp \
  renderer.cpp \
  algorithm.cpp \
  node.cpp \
  scene.cpp \
  transform.cpp \
  camera2d.cpp \
  camera3d.cpp \
  arcball.cpp \
  light.cpp \
  pointlight.cpp \
  directionallight.cpp \
  material.cpp \
  colormaterial.cpp \
  phongmaterial.cpp \
  textureset.cpp \
  texture.cpp \
  texcube.cpp \
  texdepth.cpp \
  sampler.cpp \
  clipplane.cpp \
  grid.cpp \
  triangle.cpp \
  cube.cpp \
  quad.cpp \
  sphere.cpp \
  square.cpp \
  skybox.cpp \
  mesh.cpp \

LUXOR_SRC = \
  luxor/linearinterpolator.cpp \
  luxor/cubicinterpolator.cpp \
  luxor/movement.cpp \
  luxor/animation.cpp \
  luxor/luxorengine.cpp \
  luxor/luxor.cpp \

CORE_OBJ = $(addsuffix .o, $(basename $(CORE_SRC))) glfw3webgpu.o
LUXOR_OBJ = $(addsuffix .o, $(basename $(LUXOR_SRC)))

.PRECIOUS: luxor/%.o

.cpp.o:
	$(CPP) $(INC) $(CPPFLAGS) -c $< -o $@

glfw3webgpu.o: glfw3webgpu.c
	clang $(OBJCFLAGS) -c $< -o $@

luxor/%.o: luxor/%.cpp
	$(CPP) $(INC) $(CPPFLAGS) -c $< -o $@

main_2d.o: main_2d.cpp
	$(CPP) $(INC) $(CPPFLAGS) -c $< -o $@

main_3d.o: main_3d.cpp
	$(CPP) $(INC) $(CPPFLAGS) -c $< -o $@

$(TARGET2): $(CORE_OBJ) main_2d.o Makefile
	$(CPP) $(LIB) -o $@ $(CORE_OBJ) main_2d.o $(LDFLAGS)

$(TARGET3): $(CORE_OBJ) main_3d.o Makefile
	$(CPP) $(LIB) -o $@ $(CORE_OBJ) main_3d.o $(LDFLAGS)

clean:
	rm -f $(TARGET2) $(TARGET3) $(CORE_OBJ) $(LUXOR_OBJ) main_2d.o main_3d.o
