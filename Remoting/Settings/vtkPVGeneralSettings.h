// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
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
#include "vtkParaViewDeprecation.h"    // for PARAVIEW_DEPRECATED_IN_5_12_0
#include "vtkRemotingSettingsModule.h" //needed for exports
#include "vtkSmartPointer.h"           // needed for vtkSmartPointer.

#include <string>

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

  ///@{
  /**
   * Automatically convert data arrays as needed by filters including converting
   * cell arrays to point arrays, or vice versa, and extracting single components
   * from multi-component arrays.
   * Forwards the call to vtkSMInputArrayDomain::SetAutomaticPropertyConversion.
   */
  void SetAutoConvertProperties(bool val);
  bool GetAutoConvertProperties();
  ///@}

  ///@{
  /**
   * Determines the number of distinct values in
   * vtkBlockColors. This array is added to each block if
   * the dataset is a composite dataset. The array has one value
   * set to (blockIndex % BlockColorsDistinctValues)
   */
  vtkGetMacro(BlockColorsDistinctValues, int);
  vtkSetMacro(BlockColorsDistinctValues, int);
  ///@}

  ///@{
  /**
   * Automatically apply changes in the 'Properties' panel.
   * Default is false.
   */
  vtkGetMacro(AutoApply, bool);
  vtkSetMacro(AutoApply, bool);
  ///@}

  ///@{
  /**
   * Get/Set delay for auto apply.
   * Not exposed in the UI.
   * Default is 0.
   */
  vtkGetMacro(AutoApplyDelay, int);
  vtkSetMacro(AutoApplyDelay, int);
  ///@}

  ///@{
  /**
   * Automatically apply changes in the 'Properties' panel.
   * Default is false.
   */
  vtkGetMacro(AutoApplyActiveOnly, bool);
  vtkSetMacro(AutoApplyActiveOnly, bool);
  ///@}

  ///@{
  /**
   * Get/Set the default view type.
   */
  vtkGetMacro(DefaultViewType, std::string);
  vtkSetMacro(DefaultViewType, std::string);
  ///@}

  ///@{
  /**
   * Get/Set the default interface language.
   * Default is en
   */
  vtkGetMacro(InterfaceLanguage, std::string);
  vtkSetMacro(InterfaceLanguage, std::string);
  ///@}

  /**
   * Enum for DefaultTimeStep
   */
  enum
  {
    DEFAULT_TIME_STEP_UNCHANGED,
    DEFAULT_TIME_STEP_FIRST,
    DEFAULT_TIME_STEP_LAST
  };

  ///@{
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
  ///@}

  ///@{
  /**
   * Set when animation geometry caching is enabled.
   */
  void SetCacheGeometryForAnimation(bool val);
  bool GetCacheGeometryForAnimation();
  ///@}

  ///@{
  /**
   * Set the animation cache limit in KBs.
   */
  void SetAnimationGeometryCacheLimit(unsigned long val);
  vtkGetMacro(AnimationGeometryCacheLimit, unsigned long);
  ///@}

  enum RealNumberNotation
  {
    MIXED = 0,
    SCIENTIFIC,
    FIXED,
    FULL
  };

  ///@{
  /**
   * Set the notation for the animation time toolbar.
   * Accepted values are MIXED, SCIENTIFIC, and FIXED.
   */
  vtkSetMacro(AnimationTimeNotation, int);
  vtkGetMacro(AnimationTimeNotation, int);
  ///@}

  ///@{
  /**
   * Get/Set the usage of shortest accurate precision instead of actual precision for animation time
   */
  vtkSetMacro(AnimationTimeShortestAccuratePrecision, bool);
  vtkGetMacro(AnimationTimeShortestAccuratePrecision, bool);
  ///@}

  ///@{
  /**
   * Set the precision of the animation time toolbar.
   */
  vtkSetMacro(AnimationTimePrecision, int);
  vtkGetMacro(AnimationTimePrecision, int);
  ///@}

  ///@{
  /**
   * Set when animation shortcuts are shown.
   */
  vtkSetMacro(ShowAnimationShortcuts, bool);
  vtkGetMacro(ShowAnimationShortcuts, bool);
  vtkBooleanMacro(ShowAnimationShortcuts, bool);
  ///@}

  ///@{
  /**
   * Set whether to reset display when showing
   * a representation in an empty view.
   */
  vtkSetMacro(ResetDisplayEmptyViews, bool);
  vtkGetMacro(ResetDisplayEmptyViews, bool);
  vtkBooleanMacro(ResetDisplayEmptyViews, bool);
  ///@}

  ///@{
  /**
   * Get/Set the notation of real number displayed in widgets or views.
   */
  vtkSetMacro(RealNumberDisplayedNotation, int);
  vtkGetMacro(RealNumberDisplayedNotation, int);
  ///@}

  ///@{
  /**
   * Get/Set the usage of shortest accurate precision instead of actual precision for real numbers
   */
  vtkSetMacro(RealNumberDisplayedShortestAccuratePrecision, bool);
  vtkGetMacro(RealNumberDisplayedShortestAccuratePrecision, bool);
  ///@}

  ///@{
  /**
   * Get/Set the precision of real number displayed in widgets or views.
   */
  vtkSetMacro(RealNumberDisplayedPrecision, int);
  vtkGetMacro(RealNumberDisplayedPrecision, int);
  ///@}

  ///@{
  /**
   * Get/Set the low exponent used with full notation
   */
  vtkSetMacro(FullNotationLowExponent, int);
  vtkGetMacro(FullNotationLowExponent, int);
  ///@}

  ///@{
  /**
   * Get/Set the high exponent used with full notation
   */
  vtkSetMacro(FullNotationHighExponent, int);
  vtkGetMacro(FullNotationHighExponent, int);
  ///@}

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
  ///@{
  /**
   * Properties panel configuration.
   */
  vtkSetMacro(PropertiesPanelMode, int);
  vtkGetMacro(PropertiesPanelMode, int);
  ///@}

  ///@{
  /**
   * Set whether to dock widgets into place.
   */
  vtkSetMacro(LockPanels, bool);
  vtkGetMacro(LockPanels, bool);
  ///@}

  ///@{
  /**
   * Load all variables when loading a data set.
   */
  void SetLoadAllVariables(bool val);
  bool GetLoadAllVariables();
  ///@}

  ///@{
  /**
   * Load no variables when showing a 2D chart.
   */
  void SetLoadNoChartVariables(bool val);
  bool GetLoadNoChartVariables();
  ///@}

  ///@{
  /**
   * Get/Set the GUI font size. This is used only if GUIOverrideFont is true.
   */
  vtkSetClampMacro(GUIFontSize, int, 8, VTK_INT_MAX);
  vtkGetMacro(GUIFontSize, int);
  ///@}

  ///@{
  /**
   * Get/Set whether the GUIFontSize should be used.
   */
  vtkSetMacro(GUIOverrideFont, bool);
  vtkGetMacro(GUIOverrideFont, bool);
  ///@}

  ///@{
  /**
   * This method has no effect and should not be used.
   */
  PARAVIEW_DEPRECATED_IN_5_12_0("SetConsoleFontSize has no effect, do not use it.")
  void SetConsoleFontSize(int vtkNotUsed(val)){};
  PARAVIEW_DEPRECATED_IN_5_12_0("GetConsoleFontSize has no effect, do not use it.")
  int GetConsoleFontSize() { return 0; };
  ///@}

  ///@{
  /**
   *  Automatically color by **vtkBlockColors** if array is present on `Apply`.
   */
  vtkSetMacro(ColorByBlockColorsOnApply, bool);
  vtkGetMacro(ColorByBlockColorsOnApply, bool);
  ///@}

  ///@{
  /**
   * Turn on streamed rendering.
   */
  void SetEnableStreaming(bool);
  vtkGetMacro(EnableStreaming, bool);
  vtkBooleanMacro(EnableStreaming, bool);
  ///@}

  ///@{
  /**
   * Enable use of accelerated filters where available.
   */
  void SetUseAcceleratedFilters(bool);
  bool GetUseAcceleratedFilters();
  vtkBooleanMacro(UseAcceleratedFilters, bool);
  ///@}

  ///@{
  /**
   * ActiveSelection is hooked up in the MultiBlock Inspector such that a click on a/multiple
   * block(s) selects it/them. Default is true.
   */
  vtkGetMacro(SelectOnClickMultiBlockInspector, bool);
  vtkSetMacro(SelectOnClickMultiBlockInspector, bool);
  ///@}

  ///@{
  /**
   * Sets the number of threads that are used for `vtkPVSession::ThreadedCallbackQueue`.
   */
  static int GetNumberOfCallbackThreads();
  static void SetNumberOfCallbackThreads(int);
  ///@}

protected:
  vtkPVGeneralSettings() = default;
  ~vtkPVGeneralSettings() override = default;

  int BlockColorsDistinctValues = 7;
  bool AutoApply = false;
  int AutoApplyDelay = 0;
  bool AutoApplyActiveOnly = false;
  std::string DefaultViewType = "RenderView";
  std::string InterfaceLanguage = "en";
  int ScalarBarMode = AUTOMATICALLY_HIDE_SCALAR_BARS;
  bool CacheGeometryForAnimation = false;
  unsigned long AnimationGeometryCacheLimit = 0;
  int AnimationTimeNotation = MIXED;
  bool AnimationTimeShortestAccuratePrecision = false;
  int AnimationTimePrecision = 6;
  bool ShowAnimationShortcuts = false;
  int RealNumberDisplayedNotation = MIXED;
  bool RealNumberDisplayedShortestAccuratePrecision = false;
  int RealNumberDisplayedPrecision = 6;
  int FullNotationLowExponent = -6;
  int FullNotationHighExponent = 20;
  bool ResetDisplayEmptyViews = false;
  int PropertiesPanelMode = ALL_IN_ONE;
  bool LockPanels = false;
  int GUIFontSize = 0;
  bool GUIOverrideFont = false;
  bool ColorByBlockColorsOnApply = true;
  bool EnableStreaming = false;
  bool SelectOnClickMultiBlockInspector = true;

private:
  vtkPVGeneralSettings(const vtkPVGeneralSettings&) = delete;
  void operator=(const vtkPVGeneralSettings&) = delete;

  static vtkSmartPointer<vtkPVGeneralSettings> Instance;
};

#endif
