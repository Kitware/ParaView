// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMColorMapEditorHelper
 * @brief   helper for color map editor handling
 *
 * It provides helper functions for controlling transfer functions,
 * scalar coloring, etc. Scalar bar controlling is still working for
 * RenderView only for now, as it's controlling the widget.
 */

#ifndef vtkSMColorMapEditorHelper_h
#define vtkSMColorMapEditorHelper_h

#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSmartPointer.h"        // For LastLUTProxy

class vtkPVArrayInformation;
class vtkPVProminentValuesInformation;
class vtkSMProxy;

class VTKREMOTINGVIEWS_EXPORT vtkSMColorMapEditorHelper
{
public:
  /**
   * Returns the lut proxy of this representation in the given view.
   * This method will return `nullptr` if no lut proxy exists in this view.
   */
  static vtkSMProxy* GetLUTProxy(vtkSMProxy* proxy, vtkSMProxy* view);

  /**
   * Returns true if scalar coloring is enabled. This checks whether a property
   * named "ColorArrayName" exists and has a non-empty string. This does not
   * check for the validity of the array.
   */
  static bool GetUsingScalarColoring(vtkSMProxy* proxy);

  /**
   * Given the input registered representation `proxy`, sets up a lookup table associated with the
   * representation if a scalar bar is being used for `proxy`.
   */
  static void SetupLookupTable(vtkSMProxy* proxy);

  /**
   * Updates the ranges shown in the scalar bar.
   * If deleteRange is true, then the range stored for current representation proxy is deleted.
   * This should be done when the scalar bar gets separated or becomes not visible.
   * If deleteRange is false, then the range stored for current representation proxy is updated
   * with the new range value.
   */
  static bool UpdateScalarBarRange(vtkSMProxy* proxy, vtkSMProxy* view, bool deleteRange);

  /**
   * Enable/disable scalar coloring using the specified array. This will set up a
   * color and opacity transfer functions using vtkSMTransferFunctionProxy
   * instance. If arrayname is nullptr, then scalar coloring is turned off.
   * \c attribute_type must be one of vtkDataObject::AttributeTypes.
   */
  static bool SetScalarColoring(vtkSMProxy* proxy, const char* arrayname, int attribute_type);

  /**
   * Enable/disable scalar coloring using the specified array. This will set up a
   * color and opacity transfer functions using vtkSMTransferFunctionProxy
   * instance. If arrayname is nullptr, then scalar coloring is turned off.
   * \c attribute_type must be one of vtkDataObject::AttributeTypes.
   * \c component enables choosing a component to color with,
   * -1 will change to Magnitude, >=0 will change to corresponding component.
   */
  static bool SetScalarColoring(
    vtkSMProxy* proxy, const char* arrayname, int attribute_type, int component);

  /**
   * Rescales the color transfer function and opacity transfer function using the
   * current data range. Returns true if rescale was successful.
   * If \c extend is true (false by default), the transfer function range will
   * only be extended as needed to fit the data range.
   * If \c force is false (true by default), then the transfer function range is
   * not changed if locked.
   */
  static bool RescaleTransferFunctionToDataRange(
    vtkSMProxy* proxy, bool extend = false, bool force = true);

  /**
   * Rescales the color transfer function and opacity transfer function using the
   * current data range for the chosen data-array. Returns true if rescale was
   * successful.
   * \c attribute_type must be one of vtkDataObject::AttributeTypes.
   * If \c extend is true (false by default), the transfer function range will
   * only be extended as needed to fit the data range.
   * If \c force is false (true by default), then the transfer function range is
   * not changed if locked.
   */
  static bool RescaleTransferFunctionToDataRange(vtkSMProxy* proxy, const char* arrayname,
    int attribute_type, bool extend = false, bool force = true);

  /**
   * Rescales the color transfer function and opacity transfer function using the
   * current data range over time. Returns true if rescale was successful.
   */
  static bool RescaleTransferFunctionToDataRangeOverTime(vtkSMProxy* proxy);

  /**
   * Rescales the color transfer function and opacity transfer function using the
   * current data range over time for the chosen data-array. Returns true if rescale
   * was successful.
   * \c attribute_type must be one of vtkDataObject::AttributeTypes,
   */
  static bool RescaleTransferFunctionToDataRangeOverTime(
    vtkSMProxy* proxy, const char* arrayname, int attribute_type);

  ///@{
  /**
   * Rescales the color transfer function and the opacity transfer function
   * using the current data range, limited to the currernt visible elements.
   */
  static bool RescaleTransferFunctionToVisibleRange(vtkSMProxy* proxy, vtkSMProxy* view);
  static bool RescaleTransferFunctionToVisibleRange(
    vtkSMProxy* proxy, vtkSMProxy* view, const char* arrayname, int attribute_type);
  ///@}

  /**
   * Set the scalar bar visibility. This will create a new scalar bar as needed.
   * Scalar bar is only shown if scalar coloring is indeed being used.
   */
  static bool SetScalarBarVisibility(vtkSMProxy* proxy, vtkSMProxy* view, bool visible);

  /**
   * While SetScalarBarVisibility can be used to hide a scalar bar, it will
   * always simply hide the scalar bar even if its being used by some other
   * representation. Use this method instead to only hide the scalar/color bar
   * if no other visible representation in the view is mapping data using the
   * scalar bar.
   */
  static bool HideScalarBarIfNotNeeded(vtkSMProxy* repr, vtkSMProxy* view);

  /**
   * Check scalar bar visibility.  Return true if the scalar bar for this
   * representation and view is visible, return false otherwise.
   */
  static bool IsScalarBarVisible(vtkSMProxy* repr, vtkSMProxy* view);

  /**
   * Returns the array information for the data array used for scalar coloring, from input data.
   * If checkRepresentedData is true, it will also check in the represented data. Default is true.
   * If none is found, returns nullptr.
   */
  static vtkPVArrayInformation* GetArrayInformationForColorArray(
    vtkSMProxy* proxy, bool checkRepresentedData = true);

  /**
   * In case of UseSeparateColorMap enabled, this function prefix the given
   * arrayname with unique identifier, otherwise it acts as a passthrough.
   */
  static std::string GetDecoratedArrayName(vtkSMProxy* proxy, const std::string& arrayname);

  /**
   * Call vtkSMRepresentationProxy::GetProminentValuesInformation() for the
   * array used for scalar color, if any. Otherwise returns nullptr.
   */
  static vtkPVProminentValuesInformation* GetProminentValuesInformationForColorArray(
    vtkSMProxy* proxy, double uncertaintyAllowed = 1e-6, double fraction = 1e-3,
    bool force = false);

  /**
   * Get an estimated number of annotation shown on this representation scalar bar
   */
  static int GetEstimatedNumberOfAnnotationsOnScalarBar(vtkSMProxy* proxy, vtkSMProxy* view);

  /**
   * Checks if the scalar bar of this representation in view
   * is sticky visible, i.e. should be visible whenever this representation
   * is also visible.
   * It returns 1 if the scalar bar is sticky visible, 0 other wise.
   * If any problem is encountered, for example if view == nullptr,
   * or if the scalar bar representation is not instanciated / found,
   * it returns -1.
   */
  static int IsScalarBarStickyVisible(vtkSMProxy* proxy, vtkSMProxy* view);

protected:
  /**
   * Rescales transfer function ranges using the array information provided.
   */
  // Add proxy parameter?
  static bool RescaleTransferFunctionToDataRange(
    vtkSMProxy* proxy, vtkPVArrayInformation* info, bool extend = false, bool force = true);

  /**
   * Internal method to set scalar coloring, do not use directly.
   */
  static bool SetScalarColoringInternal(
    vtkSMProxy* proxy, const char* arrayname, int attribute_type, bool useComponent, int component);

  /**
   * Used as a memory of what was the last LUT proxy linked to this representation.
   * This is used in `UpdateScalarBarRange` to update the scalar bar range when
   * turning off the coloring for this representation.
   */
  static vtkSMProxy* GetLastLUTProxy(vtkSMProxy* proxy);
  static void SetLastLUTProxy(vtkSMProxy* proxy, vtkSMProxy* lutProxy);

private:
  vtkSMColorMapEditorHelper() = delete;
  vtkSMColorMapEditorHelper(const vtkSMColorMapEditorHelper&) = delete;
  ~vtkSMColorMapEditorHelper() = delete;
  void operator=(const vtkSMColorMapEditorHelper&) = delete;
};

#endif
