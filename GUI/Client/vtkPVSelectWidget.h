/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSelectWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVSelectWidget - Select different subwidgets.
// .SECTION Description
// This widget has a selection menu which will pack different
// pvWidgets associated with selection values.  There is also an object
// varible assumed to have different string values for each of the entries.
// This widget was made for selecting clip functions or clip by scalar values.


#ifndef __vtkPVSelectWidget_h
#define __vtkPVSelectWidget_h

#include "vtkPVObjectWidget.h"

class vtkStringList;
class vtkKWOptionMenu;
class vtkKWLabel;
class vtkCollection;
class vtkPVSource;
class vtkPVStringWidgetProperty;
class vtkKWLabeledFrame;
class vtkStringList;

//BTX
template <class key, class data> 
class vtkArrayMap;
//ETX

class VTK_EXPORT vtkPVSelectWidget : public vtkPVObjectWidget
{
public:
  //BTX

  // What type of elements are in the widget
  // this is needed to pass the correct type to the
  // clientserver stream in accept internal
  // Description:
  // Normally, SelectWidget executes a command of the form
  // <ObjectID> Set<VariableName> <CurrentValue> where
  // ObjectID usually corresponds to the underlying VTK
  // object, VariableName is an ivar of that object and the
  // CurrentValue is a value assigned in AddItem (vtkVal argument).
  // This enum allows the type of CurrentValue to be known.
  enum ElementTypes{ INT, FLOAT, STRING, OBJECT};
  //ETX
  static vtkPVSelectWidget* New();
  vtkTypeRevisionMacro(vtkPVSelectWidget, vtkPVObjectWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Creates common widgets.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Add widgets to the possible selection.  The vtkValue
  // is value used to set the vtk object variable.
  void AddItem(const char* labelVal, vtkPVWidget *pvw, const char* vtkVal);
  
  // Description:
  // Access to the widgets for tracing.
  vtkPVWidget *GetPVWidget(const char* label);

  // Description:
  // Set the label of the menu.
  void SetLabel(const char *label);

  // Description:
  // Looks at children to determine modified state.
  virtual int GetModifiedFlag();

  // Description:
  // This method is called when the source that contains this widget
  // is selected. 
  virtual void Select();

  // Description:
  // This method is called when the source that contains this widget
  // is deselected. 
  virtual void Deselect();

  // Description:
  // This is how the user can query the state of the selection.
  // The value is the label of the widget item.
  const char* GetCurrentValue();
  void SetCurrentValue(const char* val);

  // Description:
  // This method gets called when the menu changes.
  void MenuCallback();

  // Description:
  // All sub widgets should have this frame as their parent.
  vtkKWWidget *GetFrame();

  // Description:
  // Methods used internally by accept and reset to 
  // Set and Get the widget selection.
  const char* GetCurrentVTKValue();
  const char* GetVTKValue(int i);
    
//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVSelectWidget* ClonePrototype(vtkPVSource* pvSource,
                                    vtkArrayMap<vtkPVWidget*, 
                                    vtkPVWidget*>* map);
//ETX

  //BTX
  // Description:
  // Called when accept button is pushed.
  // Sets the objects variable from UI.
  virtual void AcceptInternal(vtkClientServerID);
  virtual void PostAccept();
  //ETX

  // Description:
  // Called when reset button is pushed.
  // Sets UI current value from objects variable.
  virtual void ResetInternal();

  // Description:
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

  // Description:
  // Set the property to use with this widget.
  virtual void SetProperty(vtkPVWidgetProperty *prop);
  
  // Description:
  // Create the right property for use with this widget.
  virtual vtkPVWidgetProperty* CreateAppropriateProperty();

  // Description:
  // adds a script to the menu of the animation interface.
  virtual void AddAnimationScriptsToMenu(vtkKWMenu *menu, vtkPVAnimationInterfaceEntry *ai);
 
  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();
 
  // Description:
  // Save this widget to a file.
  virtual void SaveInBatchScript(ofstream *file);

protected:
  vtkPVSelectWidget();
  ~vtkPVSelectWidget();

  int FindIndex(const char* str, vtkStringList *list);
  void SetCurrentIndex(int idx);

  vtkKWLabeledFrame *LabeledFrame;
  vtkKWOptionMenu *Menu;

  vtkSetStringMacro(EntryLabel);
  vtkGetStringMacro(EntryLabel);
  char* EntryLabel;

  // Using this list as an array of strings.
  vtkStringList *Labels;
  vtkStringList *Values;
  vtkCollection *WidgetProperties;

  int CurrentIndex;
  ElementTypes ElementType;
  
  vtkPVStringWidgetProperty *Property;
  
//BTX
  virtual vtkPVWidget* ClonePrototypeInternal(vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX
  
  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);

private:
  vtkPVSelectWidget(const vtkPVSelectWidget&); // Not implemented
  void operator=(const vtkPVSelectWidget&); // Not implemented
};

#endif
