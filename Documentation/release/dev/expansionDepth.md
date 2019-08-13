# Adding expansion depth for tree view in xml

If a filter uses a tree widget (ArrayListDomain, ArraySelectionDomain, EnumerationDomain, CompositeTreeDomain), the default
expansion depth can now be controlled with a specific hint.
0 is the minimal expansion depth, only root node is expanded. -1 means expandAll.

  <IntVectorProperty command="..." name="...">
    <CompositeTreeDomain mode="all" name="tree">
      <RequiredProperties>
        <Property function="Input" name="Input" />
      </RequiredProperties>
    </CompositeTreeDomain>
    <Hints>
      <!-- This tag sets the height of the CompositeTreeDomain -->
      <Expansion depth="3" />
    </Hints>
  </IntVectorProperty>
