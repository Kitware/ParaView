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
by configuring the files Templates/vtkKWWidgetSetSubclass.[h|cxx].in. 
The class name for S(T) is the class name of T postfixed by 'Set'.

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
allows developpers to associate a label (vtkKWLabel) to a widget T. Despite 
the fact that many core widgets already provide some sort of labelling
framework, labeled widgets are useful in situations that require more
flexibility in terms of rendering and positioning.

Labeled widgets are created automatically as subclasses of vtkKWWidgetLabeled 
by configuring the files Templates/vtkKWWidgetLabeledSubclass.[h|cxx].in.
The class name for L(T) is the class name of T postfixed by 'Labeled'.

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

==Migration==

* Labeled widgets used to be named 'vtkKWLabeledFoo'. Since sets of
  widgets are named 'vtkKWFooSet', this used to lead to some interesting
  situations: is 'vtkKWLabeledFooSet', a set of 'vtkKWLabeledFoo', or a labeled
  'vtkKWFooSet' ? This is one of the reason why the labeled widgets are now 
  named 'vtkKWFooLabeled'. Under Emacs, you can quickly search and replace
  'vtkKWLabeled*' by 'vtkKW*Labeled' by typing: Alt-X, then 
  query-replace-regexp, then vtkKWlabeled\(\w+\) as the first parameter, and 
  vtkKW\1Labeled as the second parameter.
* Labeled widgets of type T used to have a method GetT(). Example:
  vtkKWLabeledCheckButton had GetCheckButton(). And so forth for each type T.
  The method is now GetWidget() for all types T. Check the header or the online
  documentation of vtkKWWidgetLabeled and each vtkKW*Labeled for more.
* Set of widgets of type T used to have methods AddT() and GetT(), HasT(),
  ShowT(), etc. Example: vtkKWCheckButtonSet had AddButton(), 
  GetButton(...), HasButton(), etc (here, Button instead of CheckButton). 
  And so forth for each type T. These methods are now AddWidget(), GetWidget(),
  HasWidget(), etc. for all types T. Check the header or the online 
  documentation of vtkKWWidgetSet and each vtkKW*Set for more.
* The SetLabel(const char *) and SetText(const char *) methods where mixed and
  matched among the core widgets. Some classes even had inconsistent Set/Get
  pairs, vtkKWLabel* GetLabel(), and SetLabel(const char*). The SetText()
  is now the only one in use for the core widgets. Composite widgets, including
  the labeled widgets above, may provide a SetLabelText() method as a shortcut
  to GetLabel()->SetText() for example.