## InteractiveBox: Add Matrix tab with live synchronization

InteractiveBox now provides a dedicated matrix tab to directly input or edit 4×4 matrices.
This mode coexists with the existing field-based editor (Position/Rotation/Scale), and switching
between the two keeps values synchronized in both directions.

![matrix-driven-interactive-box](add-matrix-driven-interactive-box.png)

### Developer notes
- Usage: keep using `panel_widget="InteractiveBox"` in the XML.
- Requirements: the proxy must expose `Position`, `Rotation`, and `Scale` properties; when present,
  the Matrix tab appears.
- Synchronization: matrix and PRS are converted both ways.
