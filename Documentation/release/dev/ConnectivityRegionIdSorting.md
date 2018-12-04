# **Connectivity** filter can sort RegionIds by cell count

A new option **Region Id Assignment Mode** that can be used to order the RegionIds
assigned to cells and points. Supported modes are **Unspecified** where RegionIds will be assigned
in no particular order, **Cell Count Descending** assigns increasing region IDs to connected
components with progressively smaller cell counts, and **Cell Count Ascending**
assigns increasing region IDs to connected components with progressively larger cell counts.
