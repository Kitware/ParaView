<!DOCTYPE DCF>
<assistantconfig version="3.2.0">

<profile>
  <property name="name">ParaView</property>
  <property name="title">ParaView</property>
  <property name="applicationicon">images/handbook.png</property>
  <property name="startpage">Documentation/index.html</property>
  <property name="aboutmenutext">About ParaView</property>
  <property name="abouturl">about.txt</property>
  <property name="assistantdocs">doc</property>
</profile>

<DCF ref="Documentation/index.html" icon="images/handbook.png" title="ParaView">
  <section ref="Documentation/GUIOverview.html" title="Interface Overview"/>
  <section ref="Documentation/ObjectInspector.html" title="Object Inspector">
    <keyword ref="Documentation/ObjectInspector.html">Object Inspector</keyword>
    <keyword ref="Documentation/Properties.html">Properties Panel</keyword>
    <keyword ref="Documentation/Display.html">Display Panel</keyword>
    <keyword ref="Documentation/Information.html">Information Panel</keyword>
    <section ref="Documentation/Properties.html" title="Properties Panel"/>
    <section ref="Documentation/Display.html" title="Display Panel">
      <keyword ref="Documentation/Display.html#View">View Section</keyword>
      <keyword ref="Documentation/Display.html#Color">Color Section</keyword>
      <keyword ref="Documentation/Display.html#Style">Style Section</keyword>
      <keyword ref="Documentation/Display.html#Transformation">Transformation Section</keyword>
      <keyword ref="Documentation/Display.html#Bar Chart View">Bar Chart Settings</keyword>
      <keyword ref="Documentation/Display.html#XY Plot View">XY Plot Settings</keyword>
      <section ref="Documentation/Display.html#View" title="View Section"/>
      <section ref="Documentation/Display.html#Color" title="Color Section"/>
      <section ref="Documentation/Display.html#Style" title="Style Section"/>
      <section ref="Documentation/Display.html#Transformation" title="Transformation Section"/>
      <section ref="Documentation/Display.html#Bar Chart View" title="Bar Chart Settings"/>
      <section ref="Documentation/Display.html#XY Plot View" title="XY Plot Settings"/>
    </section>
    <section ref="Documentation/Information.html" title="Information Panel"/>
  </section>
  <section ref="Documentation/PipelineBrowser.html" title="Pipeline Browser">
    <keyword ref="Documentation/PipelineBrowser.html">Pipeline Browser</keyword>
  </section>
  <section ref="Documentation/Animation.html" title="Animation">
    <keyword ref="Documentation/Animation.html">Animation</keyword>
  </section>
  <section ref="Documentation/Views.html" title="Views">
    <keyword ref="Documentation/Views.html">3D View</keyword>
    <keyword ref="Documentation/Views.html">Bar Chart View</keyword>
    <keyword ref="Documentation/Views.html">XY Plot View</keyword>
    <keyword ref="Documentation/Views.html">3D View (Comparative)</keyword>
    <keyword ref="Documentation/Views.html">Spreadsheet View</keyword>
  </section>
  <section ref="Documentation/Lookmarks.html" title="Lookmarks">
    <keyword ref="Documentation/Lookmarks.html">Lookmarks</keyword>
  </section>
  <!-- Keywords in the next 4 sections (before each closing section tag) are
       filled in by vtkSMExtractDocumentation. -->
  <section ref="Documentation/ParaViewReaders.html" title="Readers">
  </section>
  <section ref="Documentation/ParaViewSources.html" title="Sources">
  </section>
  <section ref="Documentation/ParaViewFilters.html" title="Filters">
  </section>
  <section ref="Documentation/ParaViewWriters.html" title="Writers">
  </section>
</DCF>
</assistantconfig> 
