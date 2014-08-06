/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGeneralSettings.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVGeneralSettings - object for general options.
// .SECTION Description
// vtkPVGeneralSettings keeps track of general options in a ParaView
// application.
// This is a singleton. All calls to vtkPVGeneralSettings::New() return a
// pointer to the same global instance (with reference count incremented as
// expected).
#ifndef __vtkPVGeneralSettings_h
#define __vtkPVGeneralSettings_h

#include "vtkObject.h"
#include "vtkPVServerManagerDefaultModule.h" //needed for exports
#include "vtkSmartPointer.h" // needed for vtkSmartPointer.

class VTKPVSERVERMANAGERDEFAULT_EXPORT vtkPVGeneralSettings : public vtkObject
{
public:
  static vtkPVGeneralSettings* New();
  vtkTypeMacro(vtkPVGeneralSettings, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Access the singleton.
  static vtkPVGeneralSettings* GetInstance();

  // Description:
  // Automatically convert data arrays as needed by filters including converting
  // cell arrays to point arrays, or vice versa, and extracting single components
  // from multi-component arrays.
  // Forwards the call to vtkSMInputArrayDomain::SetAutomaticPropertyConversion.
  void SetAutoConvertProperties(bool val);
  bool GetAutoConvertProperties();

  // Description:
  // Automatically apply changes in the 'Properties' panel.
  vtkGetMacro(AutoApply, bool);
  vtkSetMacro(AutoApply, bool);

  // Description:
  // Automatically apply changes in the 'Properties' panel.
  vtkGetMacro(AutoApplyActiveOnly, bool);
  vtkSetMacro(AutoApplyActiveOnly, bool);

  // Description:
  // Enable auto-mpi. Forwarded to vtkProcessModuleAutoMPI.
  void SetEnableAutoMPI(bool);
  bool GetEnableAutoMPI();

  // Description:
  // Set the core limit for auto-mpi.
  void SetAutoMPILimit(int val);
  int GetAutoMPILimit();

  // Description:
  // Get/Set the default view type.
  vtkGetStringMacro(DefaultViewType);
  vtkSetStringMacro(DefaultViewType);

  // Description:
  // Enum for TransferFunctionResetMode
  enum
    {
    GROW_ON_APPLY=0,
    GROW_ON_APPLY_AND_TIMESTEP=1,
    RESET_ON_APPLY=2,
    RESET_ON_APPLY_AND_TIMESTEP=3
    };

  // Description:
  // Get/Set the transfer function reset mode.
  vtkGetMacro(TransferFunctionResetMode, int);
  vtkSetMacro(TransferFunctionResetMode, int);

  // Description:
  // Enum for ScalarBarMode.
  enum
    {
    AUTOMATICALLY_SHOW_AND_HIDE_SCALAR_BARS=0,
    AUTOMATICALLY_HIDE_SCALAR_BARS=1,
    MANUAL_SCALAR_BARS=2
    };
  vtkGetMacro(ScalarBarMode, int);
  void SetScalarBarMode(int);

  // Description:
  // Set when animation geometry caching is enabled.
  void SetCacheGeometryForAnimation(bool val);
  vtkGetMacro(CacheGeometryForAnimation, bool);

  // Description:
  // Set the animation cache limit in KBs.
  void SetAnimationGeometryCacheLimit(unsigned long val);
  vtkGetMacro(AnimationGeometryCacheLimit, unsigned long);

  // Description:
  // Forwarded for vtkSMParaViewPipelineControllerWithRendering.
  void SetInheritRepresentationProperties(bool val);

  enum
    {
    ALL_IN_ONE=0,
    SEPARATE_DISPLAY_PROPERTIES=1,
    SEPARATE_VIEW_PROPERTIES=2,
    ALL_SEPARATE=3
    };
  // Description:
  // Properties panel configuration.
  vtkSetMacro(PropertiesPanelMode, int);
  vtkGetMacro(PropertiesPanelMode, int);

  // Description:
  // Forwarded to vtkSMViewLayoutProxy.
  void SetMultiViewImageBorderColor(double r, double g, double b);
  void SetMultiViewImageBorderWidth(int width);

  // Description:
  // Forwarded to vtkSMViewProxy.
  void SetTransparentBackground(bool val);

//BTX
protected:
  vtkPVGeneralSettings();
  ~vtkPVGeneralSettings();

  bool AutoApply;
  bool AutoApplyActiveOnly;
  char* DefaultViewType;
  int TransferFunctionResetMode;
  int ScalarBarMode;
  bool CacheGeometryForAnimation;
  unsigned long AnimationGeometryCacheLimit;
  int PropertiesPanelMode;

private:
  vtkPVGeneralSettings(const vtkPVGeneralSettings&); // Not implemented
  void operator=(const vtkPVGeneralSettings&); // Not implemented

  static vtkSmartPointer<vtkPVGeneralSettings> Instance;
//ETX
};

#endif
