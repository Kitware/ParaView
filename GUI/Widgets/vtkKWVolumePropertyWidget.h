/*=========================================================================

  Module:    vtkKWVolumePropertyWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWVolumePropertyWidget - a transfer function widget
// .SECTION Description
// This class contains the UI components and methods to edit a 
// ColorTransferFunction in concert with a PiecewiseFunction for opacity.
// New control points can be added by clicking with the left mouse button
// and they can be removed by dragging them out of the window.

#ifndef __vtkKWVolumePropertyWidget_h
#define __vtkKWVolumePropertyWidget_h

#include "vtkKWWidget.h"

class vtkDataSet;
class vtkKWCheckButton;
class vtkKWColorTransferFunctionEditor;
class vtkKWHSVColorSelector;
class vtkKWHistogramSet;
class vtkKWFrameLabeled;
class vtkKWOptionMenuLabeled;
class vtkKWScaleSetLabeled;
class vtkKWOptionMenu;
class vtkKWPiecewiseFunctionEditor;
class vtkKWScalarComponentSelectionWidget;
class vtkKWScale;
class vtkKWVolumeMaterialPropertyWidget;
class vtkVolumeProperty;

class VTK_EXPORT vtkKWVolumePropertyWidget : public vtkKWWidget
{
public:
  static vtkKWVolumePropertyWidget* New();
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkTypeRevisionMacro(vtkKWVolumePropertyWidget,vtkKWWidget);

  // Description:
  // Create the widget
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Get/Set the transfer function mapping scalar value to color
//BTX
  vtkGetObjectMacro(VolumeProperty, vtkVolumeProperty);
  virtual void SetVolumeProperty(vtkVolumeProperty*);
//ETX

  // Description:
  // The data this volume property is used for. 
  // Will be used to get the scalar range of the transfer functions for
  // example.
//BTX
  vtkGetObjectMacro(DataSet, vtkDataSet);
  virtual void SetDataSet(vtkDataSet*);
//ETX

  // Description:
  // Set/Get the histograms for the data this volume property is used for.
  // Obviously those histograms must have been computed using the same
  // data as the DataSet Ivar above.
//BTX
  vtkGetObjectMacro(HistogramSet, vtkKWHistogramSet);
  virtual void SetHistogramSet(vtkKWHistogramSet*);
//ETX

  // Description:
  // Set/get the current component controlled by the widget
  virtual void SetSelectedComponent(int);
  vtkGetMacro(SelectedComponent, int);

  // Description:
  // Set/Get the window/level for those transfer functions that support
  // this mode (SetInteractiveWindowLevel will trigger interactive events)
  // IsInWindowLevelMode return true if part of this widget is in
  // window/level mode.
  virtual void SetWindowLevel(float window, float level);
  virtual void SetInteractiveWindowLevel(float window, float level);
  virtual int IsInWindowLevelMode();

  // Description:
  // Show/Hide the component selection widget
  vtkBooleanMacro(ShowComponentSelection, int);
  virtual void SetShowComponentSelection(int);
  vtkGetMacro(ShowComponentSelection, int);

  // Description:
  // Show/Hide the interpolation type widget
  vtkBooleanMacro(ShowInterpolationType, int);
  virtual void SetShowInterpolationType(int);
  vtkGetMacro(ShowInterpolationType, int);

  // Description:
  // Show/Hide the material widget + enable shading
  vtkBooleanMacro(ShowMaterialProperty, int);
  virtual void SetShowMaterialProperty(int);
  vtkGetMacro(ShowMaterialProperty, int);

  // Description:
  // Show/Hide the gradient opacity function
  vtkBooleanMacro(ShowGradientOpacityFunction, int);
  virtual void SetShowGradientOpacityFunction(int);
  vtkGetMacro(ShowGradientOpacityFunction, int);

  // Description:
  // Show/Hide the component weight
  vtkBooleanMacro(ShowComponentWeights, int);
  virtual void SetShowComponentWeights(int);
  vtkGetMacro(ShowComponentWeights, int);

  // Description:
  // If true, an "Enable Shading" checkbox will be displayed and will
  // control the shading flag of all components at once 
  // (based on the first one). If false, the shading flag will be available
  // on a per-component basis in the shading dialog.
  vtkBooleanMacro(EnableShadingForAllComponents, int);
  virtual void SetEnableShadingForAllComponents(int);
  vtkGetMacro(EnableShadingForAllComponents, int);

  // Description:
  // Update the whole UI depending on the value of the Ivars
  virtual void Update();

  // Description:
  // Set/get whether the above commands should be called or not.
  vtkSetMacro(DisableCommands, int);
  vtkGetMacro(DisableCommands, int);
  vtkBooleanMacro(DisableCommands, int);

  // Description:
  // Set commands.
  virtual void SetVolumePropertyChangedCommand(
    vtkKWObject* object,const char *method);
  virtual void SetVolumePropertyChangingCommand(
    vtkKWObject* object,const char *method);
  virtual void InvokeVolumePropertyChangedCommand();
  virtual void InvokeVolumePropertyChangingCommand();

  // Description:
  // Callbacks
  virtual void SelectedComponentCallback(int);
  virtual void InterpolationTypeCallback(int type);
  virtual void EnableShadingCallback();
  virtual void MaterialPropertyChangedCallback();
  virtual void MaterialPropertyChangingCallback();
  virtual void ScalarOpacityFunctionChangedCallback();
  virtual void ScalarOpacityFunctionChangingCallback();
  virtual void WindowLevelModeCallback();
  virtual void LockOpacityAndColorCallback();
  virtual void ScalarOpacityUnitDistanceChangedCallback();
  virtual void ScalarOpacityUnitDistanceChangingCallback();
  virtual void RGBTransferFunctionChangedCallback();
  virtual void RGBTransferFunctionChangingCallback();
  virtual void RGBTransferFunctionSelectionChangedCallback();
  virtual void EnableGradientOpacityCallback(int val);
  virtual void GradientOpacityFunctionChangedCallback();
  virtual void GradientOpacityFunctionChangingCallback();
  virtual void HSVColorSelectionChangedCallback();
  virtual void HSVColorSelectionChangingCallback();
  virtual void ComponentWeightChangedCallback(int index);
  virtual void ComponentWeightChangingCallback(int index);

  // Description:
  // Access to the editors
  vtkGetObjectMacro(ScalarOpacityFunctionEditor, vtkKWPiecewiseFunctionEditor);
  vtkGetObjectMacro(ScalarColorFunctionEditor, vtkKWColorTransferFunctionEditor);
  vtkGetObjectMacro(GradientOpacityFunctionEditor, vtkKWPiecewiseFunctionEditor);
  vtkGetObjectMacro(ScalarOpacityUnitDistanceScale, vtkKWScale);
 
  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWVolumePropertyWidget();
  ~vtkKWVolumePropertyWidget();

  vtkVolumeProperty *VolumeProperty;
  vtkDataSet        *DataSet;
  vtkKWHistogramSet *HistogramSet;

  int   SelectedComponent;
  int   DisableCommands;
  int   EnableShadingForAllComponents;

  int   ShowComponentSelection;
  int   ShowInterpolationType;
  int   ShowMaterialProperty;
  int   ShowGradientOpacityFunction;
  int   ShowComponentWeights;

  // Commands

  char  *VolumePropertyChangedCommand;
  char  *VolumePropertyChangingCommand;

  virtual void InvokeCommand(const char *command);
  
  // GUI

  vtkKWFrameLabeled                   *EditorFrame;
  vtkKWHSVColorSelector               *HSVColorSelector;
  vtkKWScalarComponentSelectionWidget *ComponentSelectionWidget;
  vtkKWOptionMenuLabeled              *InterpolationTypeOptionMenu;
  vtkKWVolumeMaterialPropertyWidget   *MaterialPropertyWidget;
  vtkKWCheckButton                    *EnableShadingCheckButton;
  vtkKWCheckButton                    *InteractiveApplyCheckButton;
  vtkKWPiecewiseFunctionEditor        *ScalarOpacityFunctionEditor;
  vtkKWScale                          *ScalarOpacityUnitDistanceScale;
  vtkKWColorTransferFunctionEditor    *ScalarColorFunctionEditor;
  vtkKWCheckButton                    *LockOpacityAndColorCheckButton;
  vtkKWPiecewiseFunctionEditor        *GradientOpacityFunctionEditor;
  vtkKWOptionMenu                     *EnableGradientOpacityOptionMenu;
  vtkKWScaleSetLabeled                *ComponentWeightScaleSet;

  int                                 LockOpacityAndColor[VTK_MAX_VRCOMP];
  int                                 WindowLevelMode[VTK_MAX_VRCOMP];

  // Pack

  virtual void Pack();

  // Are the components independent of each other?

  virtual int GetIndependentComponents();
  
  // Update HSV selector

  virtual void UpdateHSVColorSelectorFromScalarColorFunctionEditor();

  // Get the dataset information
  // This methods will be overriden in subclasses so that something
  // different than the DataSet ivar will be used to compute the
  // corresponding items
  virtual int GetDataSetNumberOfComponents();
  virtual int GetDataSetScalarRange(int comp, double range[2]);
  virtual int GetDataSetAdjustedScalarRange(int comp, double range[2]);
  virtual const char* GetDataSetScalarName();
  virtual int GetDataSetScalarOpacityUnitDistanceRangeAndResolution(
    double range[2], double *resolution);

private:
  vtkKWVolumePropertyWidget(const vtkKWVolumePropertyWidget&); // Not implemented
  void operator=(const vtkKWVolumePropertyWidget&); // Not implemented
};

#endif
