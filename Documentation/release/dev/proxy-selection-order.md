# Proxy sources selection order is now respected

So far the order of selection of the proxys in the pipeline was not resepected.
For instance, selecting a box first and then a sphere in the pipeline before
applying a Groupe DataSets filter did not ensure that block 0 will be the box.
Selection order is now respected - note that order was already respected when
using the Change Inputs dialog.
