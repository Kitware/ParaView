## Adds RegionIds filter

The **RegionIds** filter is now available. It generates a cell array to identify region of a polygonal (surfacic) mesh.
A region is a portion of the surface where two adjacent cells have a relative angle less than a given threshold.

You can see **RegionIds** like a variant of the **Connectivity** filter, using an angle criteria like the **Feature Edges**.

> ![Superposition of Regions coloration and Feature Edges of a surface](!region-ids-filter.png)
>
> Superposition of Regions coloration and Feature Edges of a surface
