# Add support for showing information_only property widget in the Properties Panel.

Previously, it was not possible to show a widget for a information_only property.
It is now possible by declaring the `panel_visibility` attribute in the
declaration of the property.
By default, and contrary to non information only properties, the default value
(when not specified) is `never` (instead of `default` for standard properties).
Note that when the widget is shown, it will always be disabled (ie. not editable).

Here is a simple example on how exposing an information_only property in the GUI:
```
    <DoubleVectorProperty command="GetShrinkFactor"
                       information_only="1"
                       panel_visibility="default"
                       name="ShrinkFactorInformation">
    </DoubleVectorProperty>
```
