/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSelectTimeSet.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVSelectTimeSet - Special time selection widget used by PVEnSightReaderModule
// .SECTION Description
// This is a PVWidget specially designed to be used with PVEnSightReaderModule.
// It provides support for multiple sets. The time value selected by
// the user is passed to the EnSight reader with a SetTimeValue() call.

#ifndef __vtkPVSelectTimeSet_h
#define __vtkPVSelectTimeSet_h

#include "vtkPVWidget.h"

class vtkKWLabel;
class vtkKWMenu;
class vtkKWFrameLabeled;
class vtkDataArrayCollection;

class VTK_EXPORT vtkPVSelectTimeSet : public vtkPVWidget
{
public:
  static vtkPVSelectTimeSet* New();
  vtkTypeRevisionMacro(vtkPVSelectTimeSet, vtkPVWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void Create(vtkKWApplication *pvApp);

  //BTX
  // Description:
  // Called when accept button is pushed.
  // Sets objects variable to the widgets value.
  // Adds a trace entry.  Side effect is to turn modified flag off.
  virtual void Accept();
  //ETX

  // Description:
  // Called when the reset button is pushed.
  // Sets widget's value to the object-variable's value.
  // Side effect is to turn the modified flag off.
  virtual void ResetInternal();

  // Description:
  // Initialize the widget after creation.
  virtual void Initialize();

  // Description:
  // This is the labeled frame around the timeset tree.
  vtkGetObjectMacro(LabeledFrame, vtkKWFrameLabeled);

  // Description:
  // Label displayed on the labeled frame.
  void SetLabel(const char* label);
  const char* GetLabel();

  // Description:
  // Updates the time value label and the time ivar.
  void SetTimeValue(float time);
  vtkGetMacro(TimeValue, float);

  // Description:
  // Calls this->SetTimeValue () and Reader->SetTimeValue()
  // with currently selected time value.
  void SetTimeValueCallback(const char* item);

//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVSelectTimeSet* ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  // Description:
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

  // Description:
  // Save this widget to a file.
  virtual void SaveInBatchScript(ofstream *file);
 
protected:
  vtkPVSelectTimeSet();
  ~vtkPVSelectTimeSet();

  vtkPVSelectTimeSet(const vtkPVSelectTimeSet&); // Not implemented
  void operator=(const vtkPVSelectTimeSet&); // Not implemented

  void CommonReset();

  vtkSetStringMacro(FrameLabel);
  vtkGetStringMacro(FrameLabel);

  vtkKWWidget* Tree;
  vtkKWWidget* TreeFrame;
  vtkKWLabel* TimeLabel;
  vtkKWFrameLabeled* LabeledFrame;

  void AddRootNode(const char* name, const char* text);
  void AddChildNode(const char* parent, const char* name, 
                    const char* text, const char* data);

  float TimeValue;
  char* FrameLabel;

  vtkDataArrayCollection* TimeSets;
  vtkClientServerID ServerSideID;

  // Fill the TimeSets collection with that from the actual reader.
  void SetTimeSetsFromReader();

//BTX
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX
  
  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);

  // Description:
  // An interface for saving a widget into a script.
  virtual void SaveInBatchScriptForPart(ofstream *file, vtkClientServerID);

};

#endif
