# Filter to extract subset from a structured grid starting with a seed point

Sometimes one encounters a multi-block of structured grids
that are not aligned in i-j-k space, however the blocks may be adjacent in x-y-z
space. In that case, **Extract Subset** does not work as expected since it only
takes in the volume of interest in i-j-k space. **Extract Subset With Seed**
takes in a XYZ location instead and can extract lines or planes in the i-j-k
space starting at that point in the dataset and following the structured grid
along the requested i-j-k directions.
