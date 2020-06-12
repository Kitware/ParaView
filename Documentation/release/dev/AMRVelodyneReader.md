## Added Velodyne AMR Reader into Paraview
Paraview now has the ability to read AMR files written by Velodyne. The reader classes were added in VTK and VTK was updated here to reflect it. Velodyne is a multi-physics code written by Corvid Technologies. It is a coupled Lagrangian-Eularian code where the Euler equations are solved using AMR. The resulting *.xamr files can be larger than 40GB. This reader was designed to read these files efficiently. An example visualization of warhead detonation is given below.
![AMR-Detonation](AMRDetonationVelodyne.png)
