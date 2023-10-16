## Add a WarnOnPropertyChange string property widget hint

Display a warning message box when a *string* property has been changed using a hint. Set the `onlyonce` attribute to only show the message on the first property change.
The `Text` tag specifies the message box title and body text.

For example
```xml
<Hints>
  <WarnOnPropertyChange onlyonce="true">
    <Text title="Warning: Potentially slow operation">
OSPRay pathtracer may need to transfer default materials from client to server. This operation can take a few seconds in client-server mode.
    </Text>
  </WarnOnPropertyChange>
</Hints>
```
