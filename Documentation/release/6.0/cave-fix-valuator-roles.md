## CAVE: Fix valuator role bindings

Calling GetValuatorRole(eventName) has never worked because valuator events are configured with a single id, which is the index of a single channel of valuator data. However, valuator events are delivered with all ids/indices of valuator as a single event.

Since the relationship between roles and valuator events isn't 1:1, the GetValuatorRole method has been removed. Now vtkSMVRInteractorStyleProxy provides a method to get the id/index associated with given a role name.
