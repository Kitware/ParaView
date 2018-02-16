# Plotting full dataset over time

A newly added filter **Plot Data Over Time** supports plotting entire datasets,
and not just the selected subset. When in distributed mode, however, the current
implementation show stats for each rank separately. This will be fixed in a future
release to correctly compute distributed statistical summaries.
