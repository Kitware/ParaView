## Developper notes: vtkSMSettings priorities update

Some `vtkSMSettings` API ask for a priority as a numeric value.

We added a `GetUserPriority()`, used as default, for the writeable settings.
This is the highest priority used in ParaView.
We also added a `GetApplicationPriority()` to put all ParaView
defined settings (*aka* site-settings) under the same priority,
lower than `GetUserPrioriry()`.

Those replace hardcoded values. If you define custom settings priority,
please base them on this new API.
