## Add cell-centered data support to FDS reader

The FDS reader now correctly supports reading slice & boundaries files (.sf & .bf),
for both point & cell-centered data. Before these changes, only point data for slices
was correctly parsed and tested.
