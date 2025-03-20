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
   */
  vtkSetMacro(AutoConvertProperties, bool);
  vtkGetMacro(AutoConvertProperties, bool);
  vtkBooleanMacro(AutoConvertProperties, bool);
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
   * Preserve applied properties values for next use.
   * This is intended to be used for pipeline sources,
   * and acts as "save current values as default" on each Apply.
   * Default is false.
   */
  vtkGetMacro(PreservePropertyValues, bool);
  vtkSetMacro(PreservePropertyValues, bool);
  /// }

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
  vtkSetMacro(ScalarBarMode, int);
  vtkGetMacro(ScalarBarMode, int);
  ///@}

  ///@{
  /**
   * Set when animation geometry caching is enabled.
   */
  vtkSetMacro(CacheGeometryForAnimation, bool);
  vtkGetMacro(CacheGeometryForAnimation, bool);
  vtkBooleanMacro(CacheGeometryForAnimation, bool);
  ///@}

  ///@{
  /**
   * Set the animation cache limit in KBs.
   */
  vtkSetMacro(AnimationGeometryCacheLimit, unsigned long);
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

  ///@{
  /**
   * Forwarded for vtkSMParaViewPipelineControllerWithRendering.
   */
  vtkSetMacro(InheritRepresentationProperties, bool);
  vtkGetMacro(InheritRepresentationProperties, bool);
  vtkBooleanMacro(InheritRepresentationProperties, bool);
  ///@}

  ///@{
  /**
   * Description:
   * When plotting data with nonpositive values, ignore the standard warning
   * and draw only the data with positive values.
   */
  vtkSetMacro(IgnoreNegativeLogAxisWarning, bool);
  vtkGetMacro(IgnoreNegativeLogAxisWarning, bool);
  vtkBooleanMacro(IgnoreNegativeLogAxisWarning, bool);
  ///@}

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
  vtkSetMacro(LoadAllVariables, bool);
  vtkGetMacro(LoadAllVariables, bool);
  vtkBooleanMacro(LoadAllVariables, bool);
  ///@}

  ///@{
  /**
   * Load no variables when showing a 2D chart.
   */
  vtkSetMacro(LoadNoChartVariables, bool);
  vtkGetMacro(LoadNoChartVariables, bool);
  vtkBooleanMacro(LoadNoChartVariables, bool);
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
   *  Automatically color by **vtkBlockColors** if array is present on `Apply`.
   */
  vtkSetMacro(ColorByBlockColorsOnApply, bool);
  vtkGetMacro(ColorByBlockColorsOnApply, bool);
  ///@}

  ///@{
  /**
   * Turn on streamed rendering.
   */
  vtkSetMacro(EnableStreaming, bool);
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

  ///@{
  /**
   * Sets the number of threads that are used by `vtkSMPTools`.
   */
  static int GetNumberOfSMPThreads();
  static void SetNumberOfSMPThreads(int);
  ///@}

protected:
  vtkPVGeneralSettings() = default;
  ~vtkPVGeneralSettings() override = default;

private:
  vtkPVGeneralSettings(const vtkPVGeneralSettings&) = delete;
  void operator=(const vtkPVGeneralSettings&) = delete;

  static vtkSmartPointer<vtkPVGeneralSettings> Instance;

  int BlockColorsDistinctValues = 7;
  bool AutoApply = false;
  int AutoApplyDelay = 0;
  bool AutoApplyActiveOnly = false;
  bool PreservePropertyValues = false;
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
  bool AutoConvertProperties = false;
  bool LoadAllVariables = false;
  bool IgnoreNegativeLogAxisWarning = false;
  bool InheritRepresentationProperties = false;
  bool LoadNoChartVariables = false;
};

#endif
