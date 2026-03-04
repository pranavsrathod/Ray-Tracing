# Ray Tracer

A CPU-based ray tracer written in C++ for USC's CSCI 420: Computer Graphics.  
Renders 640×480 images of scenes containing spheres and triangles with Phong shading and shadow computation.

---

## Features

- Ray generation per pixel
- Ray-sphere and ray-triangle intersection
- Phong shading (ambient, diffuse, specular)
- Shadow rays for each light source
- JPEG output

## Extra Credit

| Feature | Branch |
|---|---|
| Anti-Aliasing | [antialiasing](https://github.com/pranavsrathod/Ray-Tracing/tree/antialiasing) |
| Recursive Reflections | [reflections](https://github.com/pranavsrathod/Ray-Tracing/tree/reflections) |

---

## Build Instructions

### macOS (Apple Silicon / Intel)

Build the included libjpeg from source first:

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

---

## Usage

```bash
./hw3 <scene_file> [output.jpg]
```

Display only:
```bash
./hw3 test1.scene
```

Display and save:
```bash
./hw3 spheres.scene spheres.jpg
```

---

## Scene Files

| Scene | Description |
|---|---|
| `test1.scene` | Single gray sphere with one white light |
| `test2.scene` | Triangle ground plane with a sphere |
| `spheres.scene` | Five spheres |
| `table.scene` | Table and boxes |
| `SIGGRAPH.scene` | SIGGRAPH logo |
| `toy.scene` | Toy scene |
| `snow.scene` | Snowman scene |

---

## Example Renders

| Scene | Flat Shading | Sphere Shading | Triangle Shading | Shadows | Anti-Aliasing |
|---|---|---|---|---|---|
| Snow | ![](raytracer-showcase/images/01-flat-shading/snow.jpg) | ![](raytracer-showcase/images/02-sphere-shading/snow.jpg) | ![](raytracer-showcase/images/03-triangle-shading/snow.jpg) | ![](raytracer-showcase/images/04-shadows/snow.jpg) | ![](raytracer-showcase/images/05-antialiasing/snow.jpg) |
| Spheres | ![](raytracer-showcase/images/01-flat-shading/spheres.jpg) | ![](raytracer-showcase/images/02-sphere-shading/spheres.jpg) | ![](raytracer-showcase/images/03-triangle-shading/spheres.jpg) | ![](raytracer-showcase/images/04-shadows/spheres.jpg) | ![](raytracer-showcase/images/05-antialiasing/spheres.jpg) |
| Table | ![](raytracer-showcase/images/01-flat-shading/table.jpg) | ![](raytracer-showcase/images/02-sphere-shading/table.jpg) | ![](raytracer-showcase/images/03-triangle-shading/table.jpg) | ![](raytracer-showcase/images/04-shadows/table.jpg) | ![](raytracer-showcase/images/05-antialiasing/table.jpg) |
| Test2 | ![](raytracer-showcase/images/01-flat-shading/test2.jpg) | ![](raytracer-showcase/images/02-sphere-shading/test2.jpg) | ![](raytracer-showcase/images/03-triangle-shading/test2.jpg) | ![](raytracer-showcase/images/04-shadows/test2.jpg) | ![](raytracer-showcase/images/05-antialiasing/test2.jpg) |
| Toy | ![](raytracer-showcase/images/01-flat-shading/toy.jpg) | ![](raytracer-showcase/images/02-sphere-shading/toy.jpg) | ![](raytracer-showcase/images/03-triangle-shading/toy.jpg) | ![](raytracer-showcase/images/04-shadows/toy.jpg) | ![](raytracer-showcase/images/05-antialiasing/toy.jpg) |
| SIGGRAPH | ![](raytracer-showcase/images/01-flat-shading/SIGGRAPH.jpg) | ![](raytracer-showcase/images/02-sphere-shading/SIGGRAPH.jpg) | ![](raytracer-showcase/images/03-triangle-shading/SIGGRAPH.jpg) | ![](raytracer-showcase/images/04-shadows/SIGGRAPH.jpg) | ![](raytracer-showcase/images/05-antialiasing/SIGGRAPH.jpg) |

Recursive reflections at varying depth:

| Scene | Depth 1 | Depth 2 | Depth 3 |
|---|---|---|---|
| Spheres | ![](raytracer-showcase/images/06-recursive-reflections/01/spheres.jpg) | ![](raytracer-showcase/images/06-recursive-reflections/02/spheres.jpg) | ![](raytracer-showcase/images/06-recursive-reflections/03/spheres.jpg) |
| Test2 | ![](raytracer-showcase/images/06-recursive-reflections/01/test2.jpg) | ![](raytracer-showcase/images/06-recursive-reflections/02/test2.jpg) | ![](raytracer-showcase/images/06-recursive-reflections/03/test2.jpg) |
| SIGGRAPH | ![](raytracer-showcase/images/06-recursive-reflections/01/SIGGRAPH.jpg) | ![](raytracer-showcase/images/06-recursive-reflections/02/SIGGRAPH.jpg) | ![](raytracer-showcase/images/06-recursive-reflections/03/SIGGRAPH.jpg) |

---

## Cleanup

```bash
make clean
```
