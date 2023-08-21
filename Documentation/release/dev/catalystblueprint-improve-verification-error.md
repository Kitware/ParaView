## CatalystBlueprint: improve error reporting

Passing a malformed conduit node to ParaViewCatalyst will now result in a
formatted string that indicates which child node has the issue along with any
other error message the conduit verification routines return.

Example of a previous error when mistyping `coords` to `coord` in `topologies/mesh/coordset`
```
(   0.078s) [pvbatch         ]vtkCatalystBlueprint.cx:395    ERR| Conduit Mesh blueprint validate failed!
(   0.078s) [pvbatch         ]   ParaViewCatalyst.cxx:333    ERR| invalid 'catalyst' node passed to 'catalyst_execute'. Execution failed.
```

New Example for the same error:
```
(   0.079s) [pvbatch         ]vtkCatalystBlueprint.cx:394    ERR| Conduit Mesh blueprint validate failed!
(   0.079s) [pvbatch         ]vtkCatalystBlueprint.cx:13     ERR| {
(   0.079s) [pvbatch         ]vtkCatalystBlueprint.cx:13     ERR| .   { topologies
(   0.079s) [pvbatch         ]vtkCatalystBlueprint.cx:13     ERR| .   .   { mesh
(   0.079s) [pvbatch         ]vtkCatalystBlueprint.cx:13     ERR| .   .   .   { coordset
(   0.079s) [pvbatch         ]vtkCatalystBlueprint.cx:13     ERR| .   .   .   } 0.000 s: coordset
(   0.079s) [pvbatch         ]vtkCatalystBlueprint.cx:20     ERR| .   .   .   Errors: 1
(   0.079s) [pvbatch         ]vtkCatalystBlueprint.cx:23     ERR| .   .   .   Error 0 : mesh: reference to non-existent coordset 'coord'
(   0.079s) [pvbatch         ]vtkCatalystBlueprint.cx:13     ERR| .   .   } 0.000 s: mesh
(   0.079s) [pvbatch         ]vtkCatalystBlueprint.cx:13     ERR| .   } 0.000 s: topologies
(   0.079s) [pvbatch         ]vtkCatalystBlueprint.cx:13     ERR| .   { fields
(   0.079s) [pvbatch         ]vtkCatalystBlueprint.cx:13     ERR| .   .   { velocity
(   0.079s) [pvbatch         ]vtkCatalystBlueprint.cx:13     ERR| .   .   .   { topology
(   0.079s) [pvbatch         ]vtkCatalystBlueprint.cx:13     ERR| .   .   .   } 0.000 s: topology
(   0.079s) [pvbatch         ]vtkCatalystBlueprint.cx:20     ERR| .   .   .   Errors: 1
(   0.079s) [pvbatch         ]vtkCatalystBlueprint.cx:23     ERR| .   .   .   Error 0 : mesh: reference to invalid topology 'mesh'
(   0.079s) [pvbatch         ]vtkCatalystBlueprint.cx:13     ERR| .   .   } 0.000 s: velocity
(   0.079s) [pvbatch         ]vtkCatalystBlueprint.cx:13     ERR| .   .   { pressure
(   0.079s) [pvbatch         ]vtkCatalystBlueprint.cx:13     ERR| .   .   .   { topology
(   0.079s) [pvbatch         ]vtkCatalystBlueprint.cx:13     ERR| .   .   .   } 0.000 s: topology
(   0.079s) [pvbatch         ]vtkCatalystBlueprint.cx:20     ERR| .   .   .   Errors: 1
(   0.079s) [pvbatch         ]vtkCatalystBlueprint.cx:23     ERR| .   .   .   Error 0 : mesh: reference to invalid topology 'mesh'
(   0.079s) [pvbatch         ]vtkCatalystBlueprint.cx:13     ERR| .   .   } 0.000 s: pressure
(   0.079s) [pvbatch         ]vtkCatalystBlueprint.cx:13     ERR| .   } 0.000 s: fields
(   0.079s) [pvbatch         ]vtkCatalystBlueprint.cx:13     ERR| } 0.000 s:
(   0.079s) [pvbatch         ]   ParaViewCatalyst.cxx:333    ERR| invalid 'catalyst' node passed to 'catalyst_execute'. Execution failed.
```
