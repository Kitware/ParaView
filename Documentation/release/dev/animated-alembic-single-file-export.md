# Alembic export over time as a single file

ParaView now supports combining all exported timesteps of an animated Alembic
export into a single Alembic file, from the `File -> Export scene` menu,
thanks to the new **Write Timesteps as Single File** option. When enabled,
each timestep's mesh points, topology, normals, and texture coordinates are
written as native Alembic time samples into one archive, instead of one
independent `.abc` file per timestep. Texture images are still written as
separate PNG files, but only when a texture's contents actually change from
one timestep to the next.
