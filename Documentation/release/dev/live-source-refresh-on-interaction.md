# Add refresh_on_interaction LiveSource attribute

Add a new boolean attribute `refresh_on_interaction`, that allows live sources to be refreshed upon user interaction. The default value is false (current behavior).

Keep in mind that enabling this option could complicate interactions with large data sets.

```xml
    <SourceProxy ...>
      ...
      <Hints>
        <LiveSource interval="50" refresh_on_interaction="1" />
      </Hints>
    </SourceProxy>
```
