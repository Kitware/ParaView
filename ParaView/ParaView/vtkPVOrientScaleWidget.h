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
class vtkPVInputMenu;
class vtkPVStringAndScalarListWidgetProperty;

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
  virtual void AcceptInternal(vtkClientServerID);
  virtual void ResetInternal();
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
  void ScalarsMenuEntryCallback();
  void VectorsMenuEntryCallback();
  
  // Description:
  // This input menu supplies the data set.
  virtual void SetInputMenu(vtkPVInputMenu*);
  vtkGetObjectMacro(InputMenu, vtkPVInputMenu);
  
  // Description:
  // This is called to update the widget is something (InputMenu) changes.
  virtual void Update();
 
  // Description:
  // Set/get the property to use with this widget.
  virtual void SetProperty(vtkPVWidgetProperty* prop);
  virtual vtkPVWidgetProperty* GetProperty();
  
  // Description:
  // Create the right property for use with this widget.
  virtual vtkPVWidgetProperty* CreateAppropriateProperty();

  // Description:
  // Set the VTK commands.
  vtkSetStringMacro(ScalarsCommand);
  vtkSetStringMacro(VectorsCommand);
  vtkSetStringMacro(OrientCommand);
  vtkSetStringMacro(ScaleModeCommand);
  vtkSetStringMacro(ScaleFactorCommand);

  // Description:
  // Set default widget values.
  vtkSetMacro(DefaultOrientMode, int);
  vtkSetMacro(DefaultScaleMode, int);
  
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

  vtkPVInputMenu *InputMenu;
  char *ScalarArrayName;
  char *VectorArrayName;
  vtkSetStringMacro(ScalarArrayName);
  vtkSetStringMacro(VectorArrayName);

  vtkPVStringAndScalarListWidgetProperty *Property;

  char *ScalarsCommand;
  char *VectorsCommand;
  char *OrientCommand;
  char *ScaleModeCommand;
  char *ScaleFactorCommand;

  int DefaultOrientMode;
  int DefaultScaleMode;
  
//BTX
  virtual void CopyProperties(vtkPVWidget *clone, vtkPVSource *pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX
  
  int ReadXMLAttributes(vtkPVXMLElement *element,
                        vtkPVXMLPackageParser *parser);
  
  vtkPVDataSetAttributesInformation* GetPointDataInformation();
  void UpdateArrayMenus();
  void UpdateModeMenus();
  void UpdateScaleFactor();
  
private:
  vtkPVOrientScaleWidget(const vtkPVOrientScaleWidget&); // Not implemented
  void operator=(const vtkPVOrientScaleWidget&); // Not implemented
};

#endif
