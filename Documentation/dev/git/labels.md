Labels
======

[Labels][] are used to annotate [Issues][] and [Merge Requests][]. They help
organize and provide information about their state in the development
workflow.

Labels are named using the form `[category]:[name]`. Labels in the same category
has the same color.

`area:...`
------------

Labels in this category identify a component of ParaView affected by the issue.
Current list includes `area:catalyst`, `area:doc`, `area:packaging`, `area:third-party`,
and `area:web`.

Reporters and developers can assign these labels to an issue to help organize.

`priority:...`
--------------

An issue gets assigned a **Milestone** to indicate which release a fix for it
is planned for. The `priority:...` label can be used to indicate how critical is
this issue for that milestone.


| label | issues | merge requests | description |
| ----- | -------| -------------- | ----------- |
| `priority:required` | x | | issue is **required** for a milestone (highest priority) |
| `priority:important` | x | | issue is **important** for a milestone but may be okay if missed |
| `priority:nice-to-have` | x | | issue is **nice-to-have**, but not critical or important |
| `priority:low` | x | | low priority issues for a particular milestone |

If an issue targeted for a milestone has no priority label, then it is assumed
the least priority.

`project:...`
---------------

These labels are associated with Issues to identify the project/customer that
wants the issue addressed. Project manager and developers working with their
managers can use these labels to denote the stakeholders for a particular issue.

`triage:...`
--------------

These labels can be assigned to Issues and Merge Requests to help triage as
indicated in the table below:

| label | issues | merge requests | description |
| ----- | -------| -------------- | ----------- |
| `triage:confirmed` | x |  | issue has been confirmed by someone other than the reporter |
| `triage:easy` | x | | added by developers to issues that are easy to fix |
| `triage:expired` | x | x | added to issues closed without resolving or merge requests closed without merging due to lack of activity |
| `triage:feature` | x |   | issue is a feature request; can be added by developers or reporters |
| `triage:needinfo` | x | x | on an open issue, this label is added to indicate that more information is needed from the reporter; on a closed issue, this label is added to indicate that more information is needed from the developer who closed the issue about how to test or review the issue; on a merge request this label is added to request more information from the developer before the merge request can be reviewed |
| `triage:needswork` |  | x | added to merge requests after review if the reviewer deems it needs more work before it can be merged |
| `triage:pending-dashboards` | | x | added to merge requests that are awaiting dashboards before they can be reviewed or merged |
| `triage:ready-for-review` | | x | added to merge requests that are ready for review by a developer |

These labels may be removed if the issue or merge request is no longer in the
state indicated by the label. For example, a merge request gets a `triage:ready-for-review`
label to request another developer to review it. If the reviewer deems it needs
more work, he should add the `triage:needswork` label and remove the `triage:ready-for-review`
label since the latter is no longer applicable.

`workflow:...`
--------------

These labels are added to issues to indicate their state in the development cycle.
Similar to `priority:...`, there can only be at most one workflow label on an issue
at a time.

| label | issues | merge requests | description |
| ----- | -------| -------------- | ----------- |
| `workflow:active-developement` | x | | added to an open issue that is under development |
| `workflow:customer-review` | x | | added to a closed issue that is ready for review by the customer/reporter |

[Labels]: https://gitlab.kitware.com/paraview/paraview/-/labels
[Issues]: https://gitlab.kitware.com/paraview/paraview/-/issues
[Merge Requests]: https://gitlab.kitware.com/paraview/paraview/-/merge_requests
