This example contains:

1.  A server manager xml file (helix.xml) which defines a programmable source 
    that creates a helix.  It also contains extra properties for variables in
    the python script.

2.  A custom gui created in the Qt designer.  When opening the designer, a
    blank widget may be used.  Place all labels and widgets in their places,
    add a spacer at the bottom then lay them out in a grid.  The widgets are
    named after their associated server manager property.  For example,
    NumberOfRounds is a string property with 2 values.  The first value is the
    name of the python variable, and the second is the value of the python 
    variable.  The widget is named NumberOfRounds_1, where _1 tells the GUI to
    tie the value of the widget with the second value of the server manager
    property.

3.  A helix.qrc file specifying the .ui file to include in the resource.
4.  A binary resource helix.bqrc, created from helix.qrc by the Qt's rcc command:
    rcc -binary -o helix.bqrc helix.qrc

  Although the Qt files aren't required, it does give a better user interface
  to the helix source.

