This example demonstrates how to use the ParaView application framework for
developing custom applications with work-flow different from that of ParaView's.

This is a simple spreadsheet application that can be used to inspect raw data.
The user loads a supported datafile and we immediately allow the user to look at
the data attributes.

This example demonstrates the following:
* Building user inferface by subclassing QMainWindow
  - Interface does not use pipeline-browser/object-inspector framework at at
    all.
* Unlike standard ParaView, we are not allowing user to split views or create
    different types of views i.e. the central widget in the MainWindow is
    custom-built.
