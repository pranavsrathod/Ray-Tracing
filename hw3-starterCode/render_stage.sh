#!/bin/bash
STAGE=$1  # e.g. "01-flat-shading"
OUTDIR="../raytracer-showcase/images/$STAGE"
mkdir -p $OUTDIR

scenes=("SIGGRAPH" "snow" "spheres" "table" "test1" "test2" "toy")
for scene in "${scenes[@]}"; do
  echo "Rendering $scene..."
  ./hw3 $scene.scene $OUTDIR/$scene.jpg
done
echo "Done! Images saved to $OUTDIR"
