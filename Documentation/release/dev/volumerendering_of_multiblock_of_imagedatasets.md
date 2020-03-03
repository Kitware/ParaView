## Volume rendering multiblock datasets with 3D uniform grids

If a multiblock dataset comprises of blocks of 3D uniform grids i.e.
`vtkImageData` or `vtkUniformGrid`, such a dataset can now be volume rendered.
In distributed environnents, it is assumed that blocks are distributed among ranks
in such a fashion that a sorting order can be deduced between the ranks by using
th local data bounds i.e. bounds do not overlap between ranks.
