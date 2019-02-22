# Improve csv, tsv and txt output

* Improving the csv, tsv and txt output to work better in parallel
  using a pass-the-baton parallel approach instead of aggregating
  all data to process 0 and then writing to disk. Additionally,
  composite datasets are now written out into a single file instead
  of a file per block.
