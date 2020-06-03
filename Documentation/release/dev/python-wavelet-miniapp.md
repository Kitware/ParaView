## Simple Python-based simulation for Catalyst tests / demos

We have added a new Python demo called `wavelet_miniapp`. This is simple demo miniapp
that can be used as the simulation code or driver to tests certain Catalyst python scripts.
The simulation generates an image dataset using the same data producer as the
"Wavelet" source in ParaView.

To get all support options, launched this demo as follows using either `pvbatch` or `pvpython`

```
> pvbatch -m paraview.demos.wavelet_miniapp --help
```
