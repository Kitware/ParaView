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

class vtkKWFrame;
class vtkKWLabel;
class vtkKWLabeledFrame;
class vtkKWListBox;
class vtkKWPushButton;
class vtkKWRange;
class vtkKWScale;
class vtkPVContourWidgetProperty;

class VTK_EXPORT vtkPVValueList : public vtkPVWidget
{
public:
  static vtkPVValueList* New();
  vtkTypeRevisionMacro(vtkPVValueList, vtkPVWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Set the label.  The label can be used to get this widget
  // from a script.
  void SetLabel (const char* label);
  const char* GetLabel();
  
  // Description:
  // Access to this widget from a script.
  void AddValue(float val);
  void RemoveAllValues();

  // Description:
  // Button callbacks.
  void AddValueCallback();
  void DeleteValueCallback();
  void DeleteAllValuesCallback();
  void GenerateValuesCallback();

  // Description:
  // adds a script to the menu of the animation interface.
  virtual void AddAnimationScriptsToMenu(vtkKWMenu *menu, 
                                         vtkPVAnimationInterfaceEntry *ai);

  // Description:
  // This class redefines SetBalloonHelpString since it
  // has to forward the call to a widget it contains.
  virtual void SetBalloonHelpString(const char *str);

  //BTX
  // Description:
  // Gets called when the accept button is pressed. The sub-classes
  // should first call this and then do their own thing.
  virtual void AcceptInternal(vtkClientServerID);
  //ETX

  // Description:
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

  // Description:
  // Update UI from Property object. This is an internal
  // method to be only used by the tracing interface. Use at
  // your own risk.
  void Update();
  
protected:
  vtkPVValueList();
  ~vtkPVValueList();

  static const int MAX_NUMBER_ENTRIES;

  vtkPVContourWidgetProperty *Property;
  
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
