# Alembic export over time

ParaView now supports animated export of a render view to Alembic, from the `File -> Export scene` menu, thanks to the new **Write Timesteps** option. Which time steps are exported can be controlled with the **Stride** and **Frame Window** properties. This feature creates one Alembic file and a set of supporting files (texture images) per exported timestep.
