Proxy Hints {#ProxyHints}
===========

This page documents *Proxy Hints*, which are XML tags accepted under *Hints*
for a *Proxy* element in the Server-Manager configuration XMLs.

WarnOnRepresentationChange
--------------------------
Warn the user on changing to a specific representation type.

For the motivation behind this hint, see BUG #15117.
This is used to indicate to the pqDisplayRepresentationWidget that the user must
be prompted with a *'Are you sure?'* if they manually switch to this
representation from the UI.

    <RepresentationProxy ...>
      ...
      <Hints>
        <WarnOnRepresentationChange value="Volume" />
      </Hints>
    </RepresentationProxy>

ReaderFactory
-------------
Mark a proxy as reader proxy so that it's used to open files from the **File |
Open** dialog.

This hint is used to mark a proxy as a reader. It provides the ParaView
applicaiton with information about extensions supported by this reader.
**extensions** attribute to list the supported extensions e.g. "foo foo.bar" for
files named as somename.foo or somename.foo.bar.
**filename_patterns** attribute is used to list the filename patterns to match.
The format is similar to what one would use for `ls` or `dir` using wildcards e.g.
spcth\* to match spcta, spctb etc.

    <!-- using extensions -->
    <SourceProxy ...>
      ...
      <Hints>
        <ReaderFactory extensions="[space separated extensions w/o leading '.']"
            filename_patterns="[space separated filename patters (using wildcards)]"
            file_description="[user-friendly description]" />
      </Hints>
    </SourceProxy>

View
----
Specify the default view to use for showing the output produced by a
source/filter.

This hint is used to indicate the name of the view to use by default for showing
the output of this source/filter on first *Apply*. To sepecify the view type for
a specific output port, you can use the optional attribute **port**.

    <SourceProxy ...>
      ...
      <Hints>
        <View type="XYChartView" />
      </Hints>
    </SourceProxy>

Plotable
--------
Mark output data as plotable in 2D chart views.

Chart views in ParaView e.g. **Bar Chart View**, **Line Chart View**, support
plotting data of any type. However, since such plots don't use distributed
rendering techniques, to avoid accidentally plotting large datasets, the plots
by default can only show sources/filters that produce `vtkTable` as the output.
If a source/filter doesn't produce  a `vtkTable`, but produces data that should
indeed be plotted by such views, one can use this hint.

    <SourceProxy ...>
      <Hints>
        <Plotable />
      </Hints>
    </SourceProxy>

Representation
--------------
Specify the representation type to use by default when showing the output from a
source/filter in a particular view.

This hint is used to indicate the default representation type in any/all views.
The **view** attribute is optional. When not specified it matches all views.
Likewise, **port** attribute is optional. When not specified it matches all
output ports. The hints are processed in order. Hence when specifying multiple
Representation elements, start with most restrictive to least restrictive.

    <SourceProxy ...>
      ...
      <Hints>
        <Representation view="ComparativeRenderView" type="Surface" port="1"/>
        <Representation view="RenderView" type="Wireframe" />
      </Hints>
    </SourceProxy>
