## Add the possibility to use implicit arrays in **Ghost Cells**, **Append Datasets** and **Append Geometry**

It is now possible to use implicit arrays in the folowing filters:
- **Ghost Cells**
- **Append Datasets**
- **Append Geometry**

Using implicit array improves the execution time of the filter and reduces the memory consumption of the output. However, accessing arrays can be slower. Implicit arrays are disabled by default.
