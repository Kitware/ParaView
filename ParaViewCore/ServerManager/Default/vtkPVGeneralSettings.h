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
/**
 * @class   vtkPVGeneralSettings
 * @brief   object for general options.
 *
 * vtkPVGeneralSettings keeps track of general options in a ParaView
 * application.
 * This is a singleton. All calls to vtkPVGeneralSettings::New() return a
 * pointer to the same global instance (with reference count incremented as
 * expected).
*/

#ifndef vtkPVGeneralSettings_h
#define vtkPVGeneralSettings_h

#include "vtkObject.h"
#include "vtkPVServerManagerDefaultModule.h" //needed for exports
#include "vtkSmartPointer.h"                 // needed for vtkSmartPointer.

class VTKPVSERVERMANAGERDEFAULT_EXPORT vtkPVGeneralSettings : public vtkObject
{
public:
  static vtkPVGeneralSettings* New();
  vtkTypeMacro(vtkPVGeneralSettings, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Access the singleton.
   */
  static vtkPVGeneralSettings* GetInstance();

  //@{
  /**
   * Automatically convert data arrays as needed by filters including converting
   * cell arrays to point arrays, or vice versa, and extracting single components
   * from multi-component arrays.
   * Forwards the call to vtkSMInputArrayDomain::SetAutomaticPropertyConversion.
   */
  void SetAutoConvertProperties(bool val);
  bool GetAutoConvertProperties();
  //@}

  //@{
  /**
   * Determines the number of distinct values in
   * vtkBlockColors. This array is added to each block if
   * the dataset is a composite dataset. The array has one value
   * set to (blockIndex % BlockColorsDistinctValues)
   */
  vtkGetMacro(BlockColorsDistinctValues, int);
  vtkSetMacro(BlockColorsDistinctValues, int);
  //@}

  //@{
  /**
   * Automatically apply changes in the 'Properties' panel.
   */
  vtkGetMacro(AutoApply, bool);
  vtkSetMacro(AutoApply, bool);
  //@}

  //@{
  /**
   * Automatically apply changes in the 'Properties' panel.
   */
  vtkGetMacro(AutoApplyActiveOnly, bool);
  vtkSetMacro(AutoApplyActiveOnly, bool);
  //@}

  //@{
  /**
   * Enable auto-mpi. Forwarded to vtkProcessModuleAutoMPI.
   */
  void SetEnableAutoMPI(bool);
  bool GetEnableAutoMPI();
  //@}

  //@{
  /**
   * Set the core limit for auto-mpi.
   */
  void SetAutoMPILimit(int val);
  int GetAutoMPILimit();
  //@}

  //@{
  /**
   * Get/Set the default view type.
   */
  vtkGetStringMacro(DefaultViewType);
  vtkSetStringMacro(DefaultViewType);
  //@}

  /**
   * Enum for DefaultTimeStep
   */
  enum
  {
    DEFAULT_TIME_STEP_UNCHANGED,
    DEFAULT_TIME_STEP_FIRST,
    DEFAULT_TIME_STEP_LAST
  };

  /**
   * Enum for TransferFunctionResetMode
   */
  enum
  {
    GROW_ON_APPLY = 0,
    GROW_ON_APPLY_AND_TIMESTEP = 1,
    RESET_ON_APPLY = 2,
    RESET_ON_APPLY_AND_TIMESTEP = 3
  };

  //@{
  /**
   * Get/Set the transfer function reset mode.
   */
  vtkGetMacro(TransferFunctionResetMode, int);
  vtkSetMacro(TransferFunctionResetMode, int);
  //@}

  //@{
  /**
   * Enum for ScalarBarMode.
   */
  enum
  {
    AUTOMATICALLY_SHOW_AND_HIDE_SCALAR_BARS = 0,
    AUTOMATICALLY_HIDE_SCALAR_BARS = 1,
    MANUAL_SCALAR_BARS = 2
  };
  vtkGetMacro(ScalarBarMode, int);
  void SetScalarBarMode(int);
  //@}

  //@{
  /**
   * Set when animation geometry caching is enabled.
   */
  void SetCacheGeometryForAnimation(bool val);
  vtkGetMacro(CacheGeometryForAnimation, bool);
  //@}

  //@{
  /**
   * Set the animation cache limit in KBs.
   */
  void SetAnimationGeometryCacheLimit(unsigned long val);
  vtkGetMacro(AnimationGeometryCacheLimit, unsigned long);
  //@}

  //@{
  /**
   * Set the precision of the animation time toolbar.
   */
  vtkSetMacro(AnimationTimePrecision, int);
  vtkGetMacro(AnimationTimePrecision, int);
  //@}

  /**
   * Forwarded for vtkSMParaViewPipelineControllerWithRendering.
   */
  void SetInheritRepresentationProperties(bool val);

  enum
  {
    ALL_IN_ONE = 0,
    SEPARATE_DISPLAY_PROPERTIES = 1,
    SEPARATE_VIEW_PROPERTIES = 2,
    ALL_SEPARATE = 3
  };
  //@{
  /**
   * Properties panel configuration.
   */
  vtkSetMacro(PropertiesPanelMode, int);
  vtkGetMacro(PropertiesPanelMode, int);
  //@}

  //@{
  /**
   * Set whether to dock widgets into place.
   */
  vtkSetMacro(LockPanels, bool);
  vtkGetMacro(LockPanels, bool);
  //@}

  //@{
  /**
   * Load all variables when loading a data set.
   */
  void SetLoadAllVariables(bool val);
  bool GetLoadAllVariables();
  //@}

  //@{
  /**
   * Load no variables when showing a 2D chart.
   */
  void SetLoadNoChartVariables(bool val);
  bool GetLoadNoChartVariables();
  //@}

  //@{
  /**
   * Get/Set the GUI font size. This is used only if GUIOverrideFont is true.
   */
  vtkSetClampMacro(GUIFontSize, int, 8, VTK_INT_MAX);
  vtkGetMacro(GUIFontSize, int);
  //@}

  //@{
  /**
   * Get/Set whether the GUIFontSize should be used.
   */
  vtkSetMacro(GUIOverrideFont, bool);
  vtkGetMacro(GUIOverrideFont, bool);
  //@}

protected:
  vtkPVGeneralSettings();
  ~vtkPVGeneralSettings();

  int BlockColorsDistinctValues;
  bool AutoApply;
  bool AutoApplyActiveOnly;
  char* DefaultViewType;
  int TransferFunctionResetMode;
  int ScalarBarMode;
  bool CacheGeometryForAnimation;
  unsigned long AnimationGeometryCacheLimit;
  int AnimationTimePrecision;
  int PropertiesPanelMode;
  bool LockPanels;
  int GUIFontSize;
  bool GUIOverrideFont;

private:
  vtkPVGeneralSettings(const vtkPVGeneralSettings&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVGeneralSettings&) VTK_DELETE_FUNCTION;

  static vtkSmartPointer<vtkPVGeneralSettings> Instance;
};

#endif
