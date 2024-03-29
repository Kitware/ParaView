// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkPVLookingGlassSettings
 * @brief Stores Looking Glass renderer settings
 *
 * Unlike other settings classes, this is not a singleton class. It stores
 * view-specific settings for the Looking Glass. The associated view is set
 * view SetView.
 */

#ifndef vtkPVLookingGlassSettings_h
#define vtkPVLookingGlassSettings_h

#include "vtkObject.h"

#include "vtkPVRenderView.h" // for vtkPVRenderView

#include "vtkLookingGlassSettingsModule.h" // for VTKLOOKINGGLASSSETTINGS_EXPORT

class VTKLOOKINGGLASSSETTINGS_EXPORT vtkPVLookingGlassSettings : public vtkObject
{
public:
  static vtkPVLookingGlassSettings* New();
  vtkTypeMacro(vtkPVLookingGlassSettings, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkSetObjectMacro(View, vtkPVRenderView);
  vtkGetObjectMacro(View, vtkPVRenderView);

  vtkSetMacro(FocalPlaneMovementFactor, double);
  vtkGetMacro(FocalPlaneMovementFactor, double);

  vtkSetMacro(DeviceIndex, int);
  vtkGetMacro(DeviceIndex, int);

  vtkSetMacro(RenderRate, int);
  vtkGetMacro(RenderRate, int);

  vtkSetVector2Macro(ClippingLimits, double);
  vtkGetVector2Macro(ClippingLimits, double);

protected:
  vtkPVLookingGlassSettings();
  ~vtkPVLookingGlassSettings() override;

  vtkPVRenderView* View = nullptr;
  double FocalPlaneMovementFactor;
  int DeviceIndex = 0;
  int RenderRate = 0;
  double ClippingLimits[2];

  vtkCamera* GetActiveCamera();

private:
  vtkPVLookingGlassSettings(const vtkPVLookingGlassSettings&) = delete;
  void operator=(const vtkPVLookingGlassSettings&) = delete;
};

#endif
