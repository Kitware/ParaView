/*=========================================================================

  Program:   ParaView
  Module:    vtkPVStringEntry.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVStringEntry - string entry widget
// .SECTION Description
// Widget for entering text.

#ifndef __vtkPVStringEntry_h
#define __vtkPVStringEntry_h

#include "vtkPVObjectWidget.h"

class vtkKWEntry;
class vtkKWLabel;

class VTK_EXPORT vtkPVStringEntry : public vtkPVObjectWidget
{
public:
  static vtkPVStringEntry* New();
  vtkTypeRevisionMacro(vtkPVStringEntry, vtkPVObjectWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void Create(vtkKWApplication *pvApp);
  
  // Description:
  // This method allows scripts to modify the widgets value.
  void SetValue(const char* fileName);
  const char* GetValue();

  // Description:
  // The label.
  void SetLabel(const char* label);

  // Description:
  // This class redefines SetBalloonHelpString since it
  // has to forward the call to a widget it contains.
  virtual void SetBalloonHelpString(const char *str);

//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVStringEntry* ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  //BTX
  // Description:
  // Called when accept button is pushed.  
  // Sets objects variable to the widgets value.
  // Side effect is to turn modified flag off.
  virtual void Accept();
  //ETX

  // Description:
  // Called when the reset button is pushed.
  // Sets widget's value to the last accepted value.
  // Side effect is to turn the modified flag off.
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
  vtkPVStringEntry();
  ~vtkPVStringEntry();
  
  vtkKWLabel *LabelWidget;
  vtkKWEntry *Entry;

  vtkPVStringEntry(const vtkPVStringEntry&); // Not implemented
  void operator=(const vtkPVStringEntry&); // Not implemented

  vtkSetStringMacro(EntryLabel);
  vtkGetStringMacro(EntryLabel);
  char* EntryLabel;

//BTX
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX
  
  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);

};

#endif
