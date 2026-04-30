## Static mesh optimization

ParaView shows significant speedup for filters working on geometry
when input mesh does not change over time.
This includes the following filters:
- **Append DataSets**
- **Append Geometry**
- **Clip**
- **Slice**
- **Slice with Plane**
- **Transform**

This requires a compatible reader that marks the mesh as static over time.
For now only the VTKHDF reader supports this. If you know that your data is actually static,
but not read as such (non-VTKHDF format) you can enforce the behavior with the **Force Static Mesh** filter.

Note that the speedup are most significant with unstructured data.

For early benchmark, you can see this [blog post](https://www.kitware.com/staticmeshplugin/)
