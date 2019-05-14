# Lagrangian Particle Tracker SMP implementation

The LagragianParticleTracker filter, provided in a Plugin in ParaView,
has been reimplemented with a multithreaded algorithm using VTK SMP backend
(remember to activate it when building ParaView)
Note that those changes imply some changes on any previously developer user-developed
Lagrangian Integration models as some method signatures have been modified.
Please refer to the related discourse
[post](https://discourse.paraview.org/t/new-multithreaded-lagrangianparticletracker/1838)
for more information.
