#!/bin/bash

OUTDIR="../raytracer-showcase/images/07-path-tracing"
mkdir -p "$OUTDIR"

scenes=("SIGGRAPH" "snow" "spheres" "table" "test2" "toy")

if [ "$#" -gt 0 ]; then
  scenes=("$@")
fi

for scene in "${scenes[@]}"; do
  echo "Rendering $scene..."
  ./hw3 "$scene.scene" "$OUTDIR/$scene.jpg"
done

echo "Done. Images saved to $OUTDIR"
