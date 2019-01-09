* Support for PipelineIcon filter hint in xml

Xml filter definition can now specify the pipeline icon to use for a specific filter or output port.
It can be either a view type name or an existing icon resource.

```
 <SourceProxy>
   <Hints>
     <PipelineIcon name="<view name or icon resource>" port="<output port number>" />
   </Hints>
 </SourceProxy>
```
