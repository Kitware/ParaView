==Core Widgets==

* vtkKWCanvas
* vtkKWCheckButton
* vtkKWEntry
* vtkKWFrame
* vtkKWLabel
* vtkKWListBox
* vtkKWMenu
* vtkKWMenuButton
* vtkKWOptionMenu
* vtkKWPushButton
* vtkKWRadioButton
* vtkKWScale
* vtkKWText
* vtkKWThumbWheel

==Set of Core Widgets==

Given a core widget type T, a set S(T) of widgets T is a basic container that
allows developpers to delegate allocation, deletion, creation and packing to a
single object S(T). Sets of widgets are useful in situations that require 
dynamic numbers of widgets. They can also be used to group related widgets 
together without maintaining ivars for each one of them. 

Sets of widgets are created automatically as subclasses of vtkKWWidgetSet 
by configuring the files Templates/vtkKWWidgetSetSubclass.[h|cxx]. 

Check the CMakeLists.txt file and the Doxygen online documentation for the
complete list of sets. At the moment:
     
    vtkKWCheckButton  => vtkKWCheckButtonSet
    vtkKWEntry        => vtkKWEntrySet
    vtkKWLabel        => vtkKWLabelSet
    vtkKWLabelLabeled => vtkKWLabelLabeledSet
    vtkKWPushButton   => vtkKWPushButtonSet
    vtkKWRadioButton  => vtkKWRadioButton
    vtkKWScale        => vtkKWScaleSet

==Labeled Widgets==

Given a core widget type T, a labeled widget L(T) is a composite widget that
allows developpers to associate a label (vtkKWLabel) to a widget T. Many core
widgets already provide some sort of labelling framework: nevertheless, labeled
widgets are useful in situations that require more flexibility in terms of
rendering and positioning.

Labeled widgets are created automatically as subclasses of vtkKWWidgetLabeled 
by configuring the files Templates/vtkKWWidgetLabeledSubclass.[h|cxx].

Check the CMakeLists.txt file and the Doxygen online documentation for the
complete list of labeled widgets. At the moment:

    vtkKWCheckButton    => vtkKWCheckButtonLabeled
    vtkKWCheckButtonSet => vtkKWCheckButtonSetLabeled
    vtkKWEntry          => vtkKWEntryLabeled
    vtkKWLabel          => vtkKWLabelLabeled
    vtkKWLoadSaveButton => vtkKWLoadSaveButtonLabeled
    vtkKWOptionMenu     => vtkKWOptionMenuLabeled
    vtkKWPopupButton    => vtkKWPopupButtonLabeled
    vtkKWPushButton     => vtkKWPushButtonLabeled
    vtkKWPushButtonSet  => vtkKWPushButtonSetLabeled
    vtkKWText           => vtkKWTextLabeled
    vtkKWRadioButtonSet => vtkKWRadioButtonSetLabeled
    vtkKWScaleSet       => vtkKWScaleSetLabeled
