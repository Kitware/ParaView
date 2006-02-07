/*=========================================================================

  Program:   ParaView
  Module:    vtkPVIceTRenderModuleUI.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVIceTRenderModuleUI - UI for MPI and Client server.
// .SECTION Description
// This render module user interface controls ICE-T tile display compositing.


#ifndef __vtkPVIceTRenderModuleUI_h
#define __vtkPVIceTRenderModuleUI_h

#include "vtkPVMultiDisplayRenderModuleUI.h"

class vtkPVIceTRenderModule;

class VTK_EXPORT vtkPVIceTRenderModuleUI : public vtkPVMultiDisplayRenderModuleUI
{
public:
  static vtkPVIceTRenderModuleUI* New();
  vtkTypeRevisionMacro(vtkPVIceTRenderModuleUI,vtkPVMultiDisplayRenderModuleUI);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  void Create();

  // Description:
  // Threshold for collecting geometry to the client (vs. showing the outline
  // on the client).
  void CollectCheckCallback(int state);
  void CollectThresholdScaleCallback(double);
  void CollectThresholdLabelCallback(double);
  void SetCollectThreshold(float val);
  vtkGetMacro(CollectThreshold, float);

  // Description:
  // Tracing uses the method with the argument.
  // A reduction value of 1 is equivalent to having the feature
  // disabled.
  void StillReductionCheckCallback(int state);
  void StillReductionFactorScaleCallback(double value);
  void SetStillReductionFactor(int val);

  // Description:
  // Callback for the ordered composite check button.
  virtual void SetOrderedCompositingFlag(int state);

  // Description:
  // Resets all the settings to default values.
  virtual void ResetSettingsToDefault();
protected:
  vtkPVIceTRenderModuleUI();
  ~vtkPVIceTRenderModuleUI();

  vtkKWLabel       *CollectLabel;
  vtkKWCheckButton *CollectCheck;
  vtkKWScale       *CollectThresholdScale;
  vtkKWLabel       *CollectThresholdLabel;
  float             CollectThreshold;

  vtkKWLabel*       StillReductionLabel;
  vtkKWCheckButton* StillReductionCheck;
  vtkKWScale*       StillReductionFactorScale;
  vtkKWLabel*       StillReductionFactorLabel;
  int               StillReductionFactor;

  vtkKWCheckButton *OrderedCompositingCheck;
  int               OrderedCompositingFlag;

  vtkPVIceTRenderModuleUI(const vtkPVIceTRenderModuleUI&); // Not implemented
  void operator=(const vtkPVIceTRenderModuleUI&); // Not implemented
};


#endif
