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
class vtkPVSource;
class vtkPVWidgetCollection;
class vtkKWFrameLabeled;
class vtkStringList;

//BTX
template <class key, class data> 
class vtkArrayMap;
//ETX

class VTK_EXPORT vtkPVSelectWidget : public vtkPVObjectWidget
{
public:
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
  virtual void Accept();
  virtual void PostAccept();
  //ETX

  // Description:
  // Called when reset button is pushed.
  // Sets UI current value from objects variable.
  virtual void Reset();
  virtual void ResetInternal();

  // Description:
  // Calls Initialiaze() on currently selected widget.
  virtual void Initialize();

  // Description:
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

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

  // Description:
  // Forwards UpdateVTKObjects to the currently selected widget.
  virtual void UpdateVTKObjects();

protected:
  vtkPVSelectWidget();
  ~vtkPVSelectWidget();

  int FindIndex(const char* str, vtkStringList *list);
  void SetCurrentIndex(int idx);

  vtkKWFrameLabeled *LabeledFrame;
  vtkKWOptionMenu *Menu;

  vtkSetStringMacro(EntryLabel);
  vtkGetStringMacro(EntryLabel);
  char* EntryLabel;

  // Using this list as an array of strings.
  vtkStringList *Labels;
  vtkStringList *Values;
  vtkPVWidgetCollection *Widgets;

  int CurrentIndex;
  
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
