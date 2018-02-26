# Introduce a new plugin called DigitalRockPhysics for rock analysis

This plugin introduces two new filters to help the analysis of digital rock.

Material Cluster Analysis filter produces a multiblock dataset that contains
two blocks:
i/ a table (block 2) that contains the volume (number of cells of every cluster)
and barycenter of every cluster (set of connected cells that have the same point
data material attribute) ;
i/ a copy of the input data image (block 1) with new point data arrays that
correspond to the volume and barycenter of the material cluster it belongs to.

Material Cluster Explode filter creates an exploded surface mesh of the clusters
(set of connected cells that have the same material attribute) described in the
input image data.
