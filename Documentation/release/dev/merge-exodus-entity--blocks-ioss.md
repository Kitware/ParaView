## IOSSReader: Merge Exodus Entity Blocks

The `IOSSReader` can now merge entity blocks into a single block for the exodus format using the flag
`Merge Exodus Entity Blocks` which is off by default. This is useful e.g. for cases where the entity blocks just represent
different cell types but they actually describe the same block.
