/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMinMax.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVMinMax -
// .SECTION Description

#ifndef __vtkPVMinMax_h
#define __vtkPVMinMax_h

#include "vtkPVWidget.h"

class vtkKWScale;
class vtkKWLabel;
class vtkPVArrayMenu;

class VTK_EXPORT vtkPVMinMax : public vtkPVWidget
{
public:
  static vtkPVMinMax* New();
  vtkTypeRevisionMacro(vtkPVMinMax, vtkPVWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void Create(vtkKWApplication *pvApp);

  // Description:
  // Called when the Accept button is pressed.
  virtual void Accept();

  // Description:
  // This calculates new range to display (using the array menu).
  virtual void Update();

  // Description:
  // This method allows scripts to modify the widgets value.
  void  SetMinValue(float val);
  float GetMinValue();
  void  SetMaxValue(float val);
  float GetMaxValue();
  void  SetResolution(float res);
  float GetResolution();
  void  SetRange(float min, float max);
  void GetRange(float range[2]);

  // Description:
  // Use the scalar range of the selected array to set the min max range.
  void SetArrayMenu(vtkPVArrayMenu* widget);

  // Description:
  // Callback for min scale
  void MinValueCallback();
  
  // Description:
  // Callback for max scale
  void MaxValueCallback();
  
  // Description:
  // Label for the minimum value scale.
  void SetMinimumLabel(const char* label);

  // Description:
  // Label for the maximum value scale.
  void SetMaximumLabel(const char* label);

  // Description:
  // Set the balloon help string for the minimum value scale.
  void SetMinimumHelp(const char* help);

  // Description:
  // Set the balloon help string for the maximum value scale.
  void SetMaximumHelp(const char* help);

  // Description:
  // The underlying scales.
  vtkGetMacro(PackVertically, int);
  vtkSetMacro(PackVertically, int);
  vtkBooleanMacro(PackVertically, int);

  // Description:
  // Should the label for the min. scale be displayed ?
  vtkGetMacro(ShowMinLabel, int);
  vtkSetMacro(ShowMinLabel, int);
  vtkBooleanMacro(ShowMinLabel, int);

  // Description:
  // Should the label for the max. scale be displayed ?
  vtkGetMacro(ShowMaxLabel, int);
  vtkSetMacro(ShowMaxLabel, int);
  vtkBooleanMacro(ShowMaxLabel, int);

  // Description:
  // What should the width of the min. label be ?
  vtkGetMacro(MinLabelWidth, int);
  vtkSetMacro(MinLabelWidth, int);


  // Description:
  // What should the width of the max. label be ?
  vtkGetMacro(MaxLabelWidth, int);
  vtkSetMacro(MaxLabelWidth, int);

  // Description:
  // The underlying scales.
  vtkGetObjectMacro(MinScale, vtkKWScale);
  vtkGetObjectMacro(MaxScale, vtkKWScale);

//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVMinMax* ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

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
  vtkPVMinMax();
  ~vtkPVMinMax();
  
  vtkPVArrayMenu* ArrayMenu;  

  vtkKWLabel *MinLabel;
  vtkKWLabel *MaxLabel;
  vtkKWScale *MinScale;
  vtkKWScale *MaxScale;
  vtkKWWidget *MinFrame;
  vtkKWWidget *MaxFrame;

  char* MinHelp;
  char* MaxHelp;
  vtkSetStringMacro(MinHelp);
  vtkSetStringMacro(MaxHelp);

  int PackVertically;

  int ShowMinLabel;
  int ShowMaxLabel;

  int MinLabelWidth;
  int MaxLabelWidth;

//BTX
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);
  
private:
  vtkPVMinMax(const vtkPVMinMax&); // Not implemented
  void operator=(const vtkPVMinMax&); // Not implemented
};

#endif
