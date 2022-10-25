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
#include "vtkParaViewDeprecation.h"    // for PARAVIEW_DEPRECATED_IN_5_10_0
#include "vtkRemotingSettingsModule.h" //needed for exports
#include "vtkSmartPointer.h"           // needed for vtkSmartPointer.

class VTKREMOTINGSETTINGS_EXPORT vtkPVGeneralSettings : public vtkObject
{
public:
  static vtkPVGeneralSettings* New();
  vtkTypeMacro(vtkPVGeneralSettings, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

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
   * Default is false.
   */
  vtkGetMacro(AutoApply, bool);
  vtkSetMacro(AutoApply, bool);
  //@}

  //@{
  /**
   * Get/Set delay for auto apply.
   * Not exposed in the UI.
   * Default is 0.
   */
  vtkGetMacro(AutoApplyDelay, int);
  vtkSetMacro(AutoApplyDelay, int);
  //@}

  //@{
  /**
   * Automatically apply changes in the 'Properties' panel.
   * Default is false.
   */
  vtkGetMacro(AutoApplyActiveOnly, bool);
  vtkSetMacro(AutoApplyActiveOnly, bool);
  //@}

  //@{
  /**
   * Deprecated. AutoMPI is no longer supported.
   */
  PARAVIEW_DEPRECATED_IN_5_10_0("AutoMPI is no longer supported")
  void SetEnableAutoMPI(bool);
  PARAVIEW_DEPRECATED_IN_5_10_0("AutoMPI is no longer supported")
  bool GetEnableAutoMPI();
  PARAVIEW_DEPRECATED_IN_5_10_0("AutoMPI is no longer supported")
  void SetAutoMPILimit(int val);
  PARAVIEW_DEPRECATED_IN_5_10_0("AutoMPI is no longer supported")
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
  bool GetCacheGeometryForAnimation();
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
  vtkSetClampMacro(AnimationTimePrecision, int, 1, 17);
  vtkGetMacro(AnimationTimePrecision, int);
  //@}

  enum
  {
    MIXED = 0,
    SCIENTIFIC,
    FIXED
  };

  //@{
  /**
   * Set the notation for the animation time toolbar.
   * Accepted values are MIXED, SCIENTIFIC, and FIXED.
   */
  vtkSetMacro(AnimationTimeNotation, int);
  vtkGetMacro(AnimationTimeNotation, int);
  //@}

  //@{
  /**
   * Set when animation shortcuts are shown.
   */
  vtkSetMacro(ShowAnimationShortcuts, bool);
  vtkGetMacro(ShowAnimationShortcuts, bool);
  vtkBooleanMacro(ShowAnimationShortcuts, bool);
  //@}

  //@{
  /**
   * Set whether to reset display when showing
   * a representation in an empty view.
   */
  vtkSetMacro(ResetDisplayEmptyViews, bool);
  vtkGetMacro(ResetDisplayEmptyViews, bool);
  vtkBooleanMacro(ResetDisplayEmptyViews, bool);
  //@}

  //@{
  /**
   * This enum specifies which notations to use for displaying real number values.
   */
  enum
  {
    DISPLAY_REALNUMBERS_USING_SCIENTIFIC_NOTATION = 1,
    DISPLAY_REALNUMBERS_USING_FIXED_NOTATION,
  };
  /**
   * Get/Set the notation of real number displayed in widgets or views.
   */
  vtkSetMacro(RealNumberDisplayedNotation, int);
  vtkGetMacro(RealNumberDisplayedNotation, int);
  //@}

  //@{
  /**
   * Get/Set the precision of real number displayed in widgets or views.
   */
  vtkSetMacro(RealNumberDisplayedPrecision, int);
  vtkGetMacro(RealNumberDisplayedPrecision, int);
  //@}

  /**
   * Forwarded for vtkSMParaViewPipelineControllerWithRendering.
   */
  void SetInheritRepresentationProperties(bool val);

  // Description:
  // When plotting data with nonpositive values, ignore the standard warning
  // and draw only the data with positive values.
  void SetIgnoreNegativeLogAxisWarning(bool val);
  bool GetIgnoreNegativeLogAxisWarning();

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

  //@{
  /**
   * Console font size.
   */
  vtkSetClampMacro(ConsoleFontSize, int, 8, VTK_INT_MAX);
  vtkGetMacro(ConsoleFontSize, int);
  //@}

  //@{
  /**
   *  Automatically color by **vtkBlockColors** if array is present on `Apply`.
   */
  vtkSetMacro(ColorByBlockColorsOnApply, bool);
  vtkGetMacro(ColorByBlockColorsOnApply, bool);
  //@}

  //@{
  /**
   * Turn on streamed rendering.
   */
  void SetEnableStreaming(bool);
  vtkGetMacro(EnableStreaming, bool);
  vtkBooleanMacro(EnableStreaming, bool);
  //@}

  //@{
  /**
   * Enable use of accelerated filters where available.
   */
  void SetUseAcceleratedFilters(bool);
  bool GetUseAcceleratedFilters();
  vtkBooleanMacro(UseAcceleratedFilters, bool);
  //@}

  //@{
  /**
   * ActiveSelection is hooked up in the MultiBlock Inspector such that a click on a/multiple
   * block(s) selects it/them. Default is true.
   */
  vtkGetMacro(SelectOnClickMultiBlockInspector, bool);
  vtkSetMacro(SelectOnClickMultiBlockInspector, bool);
  //@}

protected:
  vtkPVGeneralSettings();
  ~vtkPVGeneralSettings() override;

  int BlockColorsDistinctValues;
  bool AutoApply;
  int AutoApplyDelay = 0;
  bool AutoApplyActiveOnly;
  char* DefaultViewType;
  int ScalarBarMode;
  bool CacheGeometryForAnimation;
  unsigned long AnimationGeometryCacheLimit;
  int AnimationTimePrecision;
  bool ShowAnimationShortcuts;
  int RealNumberDisplayedNotation;
  int RealNumberDisplayedPrecision;
  bool ResetDisplayEmptyViews;
  int PropertiesPanelMode;
  bool LockPanels;
  int GUIFontSize;
  bool GUIOverrideFont;
  int ConsoleFontSize;
  bool ColorByBlockColorsOnApply;
  int AnimationTimeNotation;
  bool EnableStreaming;
  bool SelectOnClickMultiBlockInspector;

private:
  vtkPVGeneralSettings(const vtkPVGeneralSettings&) = delete;
  void operator=(const vtkPVGeneralSettings&) = delete;

  static vtkSmartPointer<vtkPVGeneralSettings> Instance;
};

#endif
