/*=========================================================================

  Program:   ParaView
  Module:    vtkPVVectorEntry.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVVectorEntry - ParaView entry widget.
// .SECTION Description
// This widget allows user to enter text. It has a label and can have
// multiple fields. The maximum number of fields is 6. Once the widget
// is created the number of fields cannot be changed. When the user
// modifies the entry, the modified callback is called.

#ifndef __vtkPVVectorEntry_h
#define __vtkPVVectorEntry_h

#include "vtkPVObjectWidget.h"

class vtkStringList;
class vtkKWEntry;
class vtkKWApplication;
class vtkKWLabel;
class vtkKWWidgetCollection;

//BTX
template<class KeyType,class DataType> class vtkArrayMap;
//ETX

class VTK_EXPORT vtkPVVectorEntry : public vtkPVObjectWidget
{
public:
  static vtkPVVectorEntry* New();
  vtkTypeRevisionMacro(vtkPVVectorEntry, vtkPVObjectWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void Create(vtkKWApplication *pvApp);

  // Description:
  // I will eventually remove access to internal widgets once I figure
  // out how to get the vectors value in Tcl with any number of componenets.
  vtkGetObjectMacro(LabelWidget, vtkKWLabel);
  //BTX
  vtkGetObjectMacro(Entries, vtkKWWidgetCollection);
  //ETX
  vtkKWEntry* GetEntry(int idx);

  // Description:
  // Methods to set this widgets value from a script.
  void SetValue(char* v);
  void SetValue(char* v1, char* v2);
  void SetValue(char* v1, char* v2, char* v3);
  void SetValue(char* v1, char* v2, char* v3, char* v4);
  void SetValue(char* v1, char* v2, char* v3, char* v4, char* v5);
  void SetValue(char* v1, char* v2, char* v3, char* v4, char* v5, char* v6);
  void SetValue(char** vals, int num);
  void SetValue(float* vals, int num);

  // Description:
  // Access values in the vector. Argument num is the size of the
  // values array. It has to be smaller than number of items.
  void GetValue(float* values, int num);
  float *GetValue6();

  // Description:
  // Check if the entry was modified and call modified event.
  void CheckModifiedCallback(const char*);
  
  // Description:
  // I need a solution:  I want to run ParaView with a low resolution
  // data set, but create a batch simulation with high resolution data.
  //  When this widget is saved in a VTK script, this value is used.
  vtkSetStringMacro(ScriptValue);
  vtkGetStringMacro(ScriptValue);

  // Description:
  // adds a script to the menu of the animation interface.
  virtual void AddAnimationScriptsToMenu(vtkKWMenu *menu, vtkPVAnimationInterfaceEntry *ai);

  // Description:
  // Called when menu item (above) is selected.  Neede for tracing.
  // Would not be necessary if menus traced invocations.
  void AnimationMenuCallback(vtkPVAnimationInterfaceEntry *ai, int idx);

  // Description:
  // This is the data type the vtk object is expecting.
  vtkSetMacro(DataType, int); 
  vtkGetMacro(DataType, int); 
  
  // Description:
  // The label.
  void SetLabel(const char* label);

  // Description:
  // This class redefines SetBalloonHelpString since it
  // has to forward the call to a widget it contains.
  virtual void SetBalloonHelpString(const char *str);

  // Description:
  // Sets the length of the vector
  vtkSetMacro(VectorLength, int);
  vtkGetMacro(VectorLength, int);

  // Description:
  // Set the entry value.
  void SetEntryValue(int index, const char* value);

//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVVectorEntry* ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  // Description:
  // Move widget state to vtk object or back.
  virtual void Accept();
  virtual void ResetInternal();

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
 
protected:
  vtkPVVectorEntry();
  ~vtkPVVectorEntry();
  
  vtkKWLabel *LabelWidget;
  vtkKWWidgetCollection *Entries;

  vtkSetStringMacro(EntryLabel);
  vtkGetStringMacro(EntryLabel);
  char* EntryLabel;

  int DataType;
  int VectorLength;

  char *ScriptValue;

  // Description:
  // Get the stored entry values.
  char *EntryValues[6];

  vtkPVVectorEntry(const vtkPVVectorEntry&); // Not implemented
  void operator=(const vtkPVVectorEntry&); // Not implemented

//BTX
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX
  
  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);

};

#endif
