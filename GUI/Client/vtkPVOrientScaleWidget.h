/*=========================================================================

  Program:   ParaView
  Module:    vtkPVOrientScaleWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVOrientScaleWidget - a widget for scaling and orientation
// .SECTION Description
// vtkPVOrientScaleWidget is used by the glyph filter to handle scaling and
// orienting the glyphs.  The scale factor depends on the scale mode and the
// selected scalars and vectors.

#ifndef __vtkPVOrientScaleWidget_h
#define __vtkPVOrientScaleWidget_h

#include "vtkPVWidget.h"

class vtkKWEntry;
class vtkKWLabel;
class vtkKWLabeledFrame;
class vtkKWOptionMenu;
class vtkPVDataSetAttributesInformation;
class vtkSMProperty;

class VTK_EXPORT vtkPVOrientScaleWidget : public vtkPVWidget
{
public:
  static vtkPVOrientScaleWidget* New();
  vtkTypeRevisionMacro(vtkPVOrientScaleWidget, vtkPVWidget);
  void PrintSelf(ostream &os, vtkIndent indent);
  
  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);

  //BTX
  // Description:
  // Move widget state to vtk object or back.
  virtual void Accept();
  virtual void ResetInternal();
  virtual void Initialize();
  //ETX

  // Description:
  // Save this widget's state into a PVScript.  This method does not initialize
  // trace variable or check modified.
  virtual void Trace(ofstream *file);

  // Description:
  // Enable / disable the array menus depending on which orient and scale
  // modes have been selected.
  void UpdateActiveState();

  // Description:
  // Callbacks
  void ScaleModeMenuCallback();
  void OrientModeMenuCallback();
  void ScalarsMenuEntryCallback();
  void VectorsMenuEntryCallback();
  
  // Description:
  // This is called to update the widget is something (InputMenu) changes.
  virtual void Update();
 
  // Description:
  // Methods to set the widgets' values from a script.
  void SetOrientMode(char *mode);
  void SetScaleMode(char *mode);
  void SetScalars(char *scalars);
  void SetVectors(char *vectors);
  void SetScaleFactor(float factor);
  
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
  vtkPVOrientScaleWidget();
  ~vtkPVOrientScaleWidget();

  vtkKWLabeledFrame *LabeledFrame;
  vtkKWWidget *ScalarsFrame;
  vtkKWLabel *ScalarsLabel;
  vtkKWOptionMenu *ScalarsMenu;
  vtkKWWidget *VectorsFrame;
  vtkKWLabel *VectorsLabel;
  vtkKWOptionMenu *VectorsMenu;
  vtkKWWidget *OrientModeFrame;
  vtkKWLabel *OrientModeLabel;
  vtkKWOptionMenu *OrientModeMenu;
  vtkKWWidget *ScaleModeFrame;
  vtkKWLabel *ScaleModeLabel;
  vtkKWOptionMenu *ScaleModeMenu;
  vtkKWWidget *ScaleFactorFrame;
  vtkKWLabel *ScaleFactorLabel;
  vtkKWEntry *ScaleFactorEntry;

  char *ScalarArrayName;
  char *VectorArrayName;
  vtkSetStringMacro(ScalarArrayName);
  vtkSetStringMacro(VectorArrayName);

  char *SMScalarPropertyName;
  char *SMVectorPropertyName;
  char *SMOrientModePropertyName;
  char *SMScaleModePropertyName;
  char *SMScaleFactorPropertyName;

  void SetSMScalarProperty(vtkSMProperty *prop);
  vtkSMProperty* GetSMScalarProperty();
  void SetSMVectorProperty(vtkSMProperty *prop);
  vtkSMProperty* GetSMVectorProperty();
  void SetSMOrientModeProperty(vtkSMProperty *prop);
  vtkSMProperty* GetSMOrientModeProperty();
  void SetSMScaleModeProperty(vtkSMProperty *prop);
  vtkSMProperty* GetSMScaleModeProperty();
  void SetSMScaleFactorProperty(vtkSMProperty *prop);
  vtkSMProperty* GetSMScaleFactorProperty();

  vtkSetStringMacro(SMScalarPropertyName);
  vtkGetStringMacro(SMScalarPropertyName);
  vtkSetStringMacro(SMVectorPropertyName);
  vtkGetStringMacro(SMVectorPropertyName);
  vtkSetStringMacro(SMOrientModePropertyName);
  vtkGetStringMacro(SMOrientModePropertyName);
  vtkSetStringMacro(SMScaleModePropertyName);
  vtkGetStringMacro(SMScaleModePropertyName);
  vtkSetStringMacro(SMScaleFactorPropertyName);
  vtkGetStringMacro(SMScaleFactorPropertyName);

  char *CurrentScalars;
  char *CurrentVectors;
  char *CurrentOrientMode;
  char *CurrentScaleMode;
  vtkSetStringMacro(CurrentScalars);
  vtkSetStringMacro(CurrentVectors);
  vtkSetStringMacro(CurrentOrientMode);
  vtkSetStringMacro(CurrentScaleMode);
  
//BTX
  virtual void CopyProperties(vtkPVWidget *clone, vtkPVSource *pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX
  
  int ReadXMLAttributes(vtkPVXMLElement *element,
                        vtkPVXMLPackageParser *parser);
  
  void UpdateArrayMenus();
  void UpdateModeMenus();
  void UpdateScaleFactor();
  
private:
  vtkPVOrientScaleWidget(const vtkPVOrientScaleWidget&); // Not implemented
  void operator=(const vtkPVOrientScaleWidget&); // Not implemented
  
  vtkSMProperty *SMScalarProperty;
  vtkSMProperty *SMVectorProperty;
  vtkSMProperty *SMOrientModeProperty;
  vtkSMProperty *SMScaleModeProperty;
  vtkSMProperty *SMScaleFactorProperty;
};

#endif
