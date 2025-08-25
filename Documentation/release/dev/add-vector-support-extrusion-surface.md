## Add support of 3-component vector array and normal recalculation in **Extrusion Surface** representation

The **Extrusion Surface** representation can now process 3-component vector array as input data. If such an input is selected, it will be treated as a displacement on the GPU (see image below).

The option **Recalcuate Normals** has also been added in order to compute the surface normals after the extrusion to prevent bad visual results due to flat surface (for example with a plane).

![vector extrusion support image](vector-extrusion-support.png)
