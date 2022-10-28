## Add Prism Plugin

ParaView now has a new plugin named Prism. Prism is a plugin to that includes a custom view and representations to
visualize EOS (Equation of State) files, such as sesame, for multi physics simulations.

The purpose of this plugin is to provide a way to understand how simulation data behave in the EOS space, and understand
where interesting EOS-behaviors happen in the geometric space. This is done by visualizing the simulation data in the
Render view using a Geometry representation and in the Prism View using a Prism representation. The Prism representation
of simulation data, basically converts all the cells into vertices whose coordinates are defined by the 3 desired EOS
variables. To understand, how those cells/vertices behave in the EOS space, you need to first load material data, such
as sesame, into the Prism View. The material data basically define the domain of the EOS space in which the simulation
data can exist.

### Typical workflow

1. Load the simulation data, such as an exodus file, into ParaView (apply filters as needed).
2. Load the Material data, such as a SESAME file, into ParaView, select the desired table, 3 EOS variables and
   conversion units, and decide if you will read the curves (if available) or not.
3. After loading the Material data, a Prism view will automatically be created and the material data will be displayed
   in a cubical space as a surface, where the coordinates are the selected 3 EOS variables.
   1. It should be noted that you should NOT load any simulation into the Prism View without first loading the
      Material data, because the simulation data needs the bounds of the Material data to be displayed correctly.
4. The Prism View allows you to:
   1. Apply log-scaling to the coordinates of all the data that are displayed (material data and simulation data).
   2. Change the aspect ratio of the cubical space (default is 1:1:1).
   3. Threshold all the data that are displayed using the bounds of the material data.
   4. View the axis of the Prism view which are automatically set to the names of the selected 3 EOS variables.
5. Then, you can visualize the simulation data in the Prism View. To do that, you need to enable the hidden eye in the
   pipeline, and select the 3 EOS variables that will be used to display the simulation data in the Prism View under the
   Properties -> Representation -> Prism Parameters tab.
6. After visualizing the simulation data, you can select cells in the Prism view and see where these cells lie in the
   geometric representation of the Render View (and the other way around).
