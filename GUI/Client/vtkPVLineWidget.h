/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLineWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVLineWidget -
// .SECTION Description

// Todo:
// Cleanup GUI:
//       * Visibility
//       * Resolution
//       *

#ifndef __vtkPVLineWidget_h
#define __vtkPVLineWidget_h

#include "vtkPV3DWidget.h"

class vtkKWEntry;
class vtkKWLabel;
class vtkPickLineWidget;

class VTK_EXPORT vtkPVLineWidget : public vtkPV3DWidget
{
public:
  static vtkPVLineWidget* New();
  vtkTypeRevisionMacro(vtkPVLineWidget, vtkPV3DWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Callbacks to set the points of the 3D widget from the
  // entry values. Bound to <KeyPress-Return>.
  void SetPoint1();
  void SetPoint2();
  void SetPoint1(double x, double y, double z);
  void SetPoint2(double x, double y, double z);
  void GetPoint1(double pt[3]);
  void GetPoint2(double pt[3]);

  // Description:
  // Set the resolution of the line widget.
  void SetResolution();
  void SetResolution(int f);
  int GetResolution();

  // Description:
  // Set the tcl variables that are modified when accept is called.
  void SetPoint1VariableName(const char* varname);
  void SetPoint2VariableName(const char* varname);
  void SetResolutionVariableName(const char* varname);

  vtkGetStringMacro(Point1Variable);
  vtkGetStringMacro(Point2Variable);
  vtkGetStringMacro(ResolutionVariable);

  // Description:
  // Set the tcl labels that are modified when accept is called.
  void SetPoint1LabelTextName(const char* varname);
  void SetPoint2LabelTextName(const char* varname);
  void SetResolutionLabelTextName(const char* varname);

  // Description:
  // Labels for entries
  vtkGetStringMacro(Point1LabelText);
  vtkGetStringMacro(Point2LabelText);
  vtkGetStringMacro(ResolutionLabelText);

  // Description:
  // Determines whether the Resolution entry is shown.
  vtkGetMacro(ShowResolution, int);
  vtkSetMacro(ShowResolution, int);
  vtkBooleanMacro(ShowResolution, int);

  // Description:
  // This method does the actual placing. It sets the first point at 
  // (bounds[0]+bounds[1])/2, bounds[2], (bounds[4]+bounds[5])/2
  // and the second point at
  // (bounds[0]+bounds[1])/2, bounds[3], (bounds[4]+bounds[5])/2
  virtual void ActualPlaceWidget();
    
//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVLineWidget* ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  // Description:
  // This class redefines SetBalloonHelpString since it
  // has to forward the call to a widget it contains.
  virtual void SetBalloonHelpString(const char *str);

  //BTX
  // Description:
  // Called when accept button is pushed.
  // Sets objects variable to the widgets value.
  // Side effect is to turn modified flag off.
  virtual void Accept();
  //ETX

  // Description:
  // Initialize widget after creation
  virtual void Initialize();

  // Description:
  // Called when the reset button is pushed.
  // Sets widget's value to the object-variable's value.
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
  vtkPVLineWidget();
  ~vtkPVLineWidget();
  

//BTX
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);

  friend class vtkLineWidgetObserver;
//ETX
  
  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);

  // Description:
  // Call creation on the child.
  virtual void ChildCreate(vtkPVApplication*);

  // Description:
  // Execute event of the RM3DWidget.
  virtual void ExecuteEvent(vtkObject*, unsigned long, void*);

  //void UpdateVTKObject(vtkClientServerID);

  void SetPoint1Internal(double x, double y, double z);
  void SetPoint2Internal(double x, double y, double z);

  // Description:
  // These methods assume that the Property has been
  // updated before calling them; i.e. Property->UpdateInformation
  // has been invoked.
  void GetPoint1Internal(double pt[3]);
  void GetPoint2Internal(double pt[3]);
  int GetResolutionInternal();

  void DisplayLength(double len);

  vtkKWEntry* Point1[3];
  vtkKWEntry* Point2[3];
  vtkKWLabel* Labels[2];
  vtkKWLabel* CoordinateLabel[3];
  vtkKWLabel* ResolutionLabel;
  vtkKWEntry* ResolutionEntry;
  vtkKWLabel* LengthLabel;
  vtkKWLabel* LengthValue;

  vtkSetStringMacro(Point1Variable);
  vtkSetStringMacro(Point2Variable);
  vtkSetStringMacro(ResolutionVariable);

  char *Point1Variable;
  char *Point2Variable;
  char *ResolutionVariable;

  vtkSetStringMacro(Point1LabelText);
  vtkSetStringMacro(Point2LabelText);
  vtkSetStringMacro(ResolutionLabelText);

  char *Point1LabelText;
  char *Point2LabelText;
  char *ResolutionLabelText;

  int ShowResolution;
  
private:  
  vtkPVLineWidget(const vtkPVLineWidget&); // Not implemented
  void operator=(const vtkPVLineWidget&); // Not implemented
};

#endif
