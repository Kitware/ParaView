## USD export over time as a single file

ParaView now supports combining all exported timesteps into a single USD file thanks to the new **Write Timesteps To Single File** option. When enabled, each timestep's mesh points, topology, normals, and texture coordinates are written as native USD time samples into one file, instead of one independent `.usd` file per timestep. Texture images are still written as separate PNG files, but textures with the same contents are written once and shared among meshes within the USD file.
