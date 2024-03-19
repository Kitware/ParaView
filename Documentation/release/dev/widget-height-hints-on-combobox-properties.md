## Add a widget height hints on ComboBox properties

You can now specify `WidgetHeight` hints on properties that use a `QComboBox` such as `EnumerationDomain`, `ProxyListDomains` or `StringListDomain`. This hint will control the maximum allowed number of item in the combobox before a scroll appear.

```xml
    <StringVectorProperty command="" name="...">
        <StringListDomain name="array_list">
        <RequiredProperties>
            <Property function="..." name="..."/>
        </RequiredProperties>
        </StringListDomain>
        <Hints>
            <!-- This tag sets the height of the QComboBox -->
            <WidgetHeight number_of_rows="5" />
        </Hints>
    </StringVectorProperty>
```
