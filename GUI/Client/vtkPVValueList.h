/*=========================================================================

  Program:   ParaView
  Module:    vtkPVValueList.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVValueList maintains a list of floats.
// .SECTION Description
// This widget lets the user add or delete floats from a list.
// It is used for contours, cut and clip plane offsets..

#ifndef __vtkPVValueList_h
#define __vtkPVValueList_h

#include "vtkPVWidget.h"

class vtkContourValues;
class vtkKWFrame;
class vtkKWLabel;
class vtkKWLabeledFrame;
class vtkKWListBox;
class vtkKWPushButton;
class vtkKWRange;
class vtkKWScale;

class VTK_EXPORT vtkPVValueList : public vtkPVWidget
{
public:
  static vtkPVValueList* New();
  vtkTypeRevisionMacro(vtkPVValueList, vtkPVWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Called when the Accept button is pressed.
  virtual void Accept();

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Set the label.  The label can be used to get this widget
  // from a script.
  void SetLabel (const char* label);
  const char* GetLabel();
  
  // Description:
  // Access to this widget from a script. (RemoveAllValues is also a button
  // callback.)
  void AddValue(double val);
  void RemoveAllValues();
  
  // Description:
  // Button callbacks.
  void AddValueCallback();
  void DeleteValueCallback();
  void GenerateValuesCallback();

  // Description:
  // adds a script to the menu of the animation interface.
  virtual void AddAnimationScriptsToMenu(vtkKWMenu *menu, 
                                         vtkPVAnimationInterfaceEntry *ai);

  // Description:
  // This class redefines SetBalloonHelpString since it
  // has to forward the call to a widget it contains.
  virtual void SetBalloonHelpString(const char *str);

  // Description:
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

  // Description:
  // Update UI from Property object. This is an internal
  // method to be only used by the tracing interface. Use at
  // your own risk.
  void Update();
  
  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();
 
protected:
  vtkPVValueList();
  ~vtkPVValueList();

  static const int MAX_NUMBER_ENTRIES;

  void AddValueNoModified(double val);

  vtkContourValues *ContourValues;
  
  vtkKWLabeledFrame* ContourValuesFrame;
  vtkKWFrame* ContourValuesFrame2;
  vtkKWListBox* ContourValuesList;

  vtkKWFrame* ContourValuesButtonsFrame;
  vtkKWPushButton* DeleteValueButton;
  vtkKWPushButton* DeleteAllButton;

  vtkKWLabeledFrame* NewValueFrame;
  vtkKWLabel* NewValueLabel;
  vtkKWScale* NewValueEntry;
  vtkKWPushButton* AddValueButton;

  vtkKWLabeledFrame* GenerateFrame;
  vtkKWFrame* GenerateNumberFrame;
  vtkKWFrame* GenerateRangeFrame;

  vtkKWLabel* GenerateLabel;
  vtkKWLabel* GenerateRangeLabel;
  vtkKWScale* GenerateEntry;
  vtkKWPushButton* GenerateButton;

  vtkKWRange* GenerateRangeWidget;

  virtual int ComputeWidgetRange() {return 0;}
  
  vtkPVValueList(const vtkPVValueList&); // Not implemented
  void operator=(const vtkPVValueList&); // Not implemented

//BTX
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX
  
  int ReadXMLAttributes(vtkPVXMLElement* element,      
                        vtkPVXMLPackageParser* parser);

};

#endif
