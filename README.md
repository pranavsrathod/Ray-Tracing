# Ray Tracer

A CPU-based ray tracer built in C++ for USC's Computer Graphics course. The ray tracer renders 3D scenes containing spheres and triangles with Phong shading and shadow rays, outputting 640x480 images.

## Features

- Ray generation from camera through each pixel
- Sphere and triangle intersection
- Phong shading (diffuse + specular + ambient)
- Shadow rays for each light source
- JPEG image output

## Build Instructions

### Prerequisites
- macOS or Linux
- OpenGL and GLUT
- g++

### macOS Setup (Apple Silicon / Intel)

First, build the included libjpeg from source:

```bash
cd external/jpeg-9a-mac
chmod +x configure
./configure --prefix=$(pwd)
make clean
make
chmod +x install-sh
make install
```

Then build the ray tracer:

```bash
cd hw3-starterCode
make
```

### Linux

```bash
sudo apt-get install freeglut3-dev libjpeg-dev
cd hw3-starterCode
make
```

## How to Run

```bash
./hw3 <scene_file> [output.jpg]
```

- `<scene_file>` — required, path to a `.scene` description file
- `[output.jpg]` — optional, if provided saves the render to a JPEG file

### Examples

Display only:
```bash
./hw3 test1.scene
```

Display and save to JPEG:
```bash
./hw3 spheres.scene spheres.jpg
```

### Available Scenes

| Scene | Description |
|-------|-------------|
| `test1.scene` | Single gray sphere with one white light |
| `test2.scene` | Triangle ground plane with a sphere |
| `spheres.scene` | Five spheres |
| `table.scene` | A table and two boxes |
| `SIGGRAPH.scene` | SIGGRAPH logo scene |
| `toy.scene` | Toy scene |
| `snow.scene` | Snowman scene |

## Cleanup

```bash
make clean
```