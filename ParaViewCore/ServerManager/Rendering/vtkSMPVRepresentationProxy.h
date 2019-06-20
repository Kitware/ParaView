/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPVRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMPVRepresentationProxy
 * @brief   representation for "Render View" like
 * views in ParaView.
 *
 * vtkSMPVRepresentationProxy combines surface representation and volume
 * representation proxies typically used for displaying data.
 * This class also takes over the selection obligations for all the internal
 * representations, i.e. is disables showing of selection in all the internal
 * representations, and manages it. This avoids duplicate execution of extract
 * selection filter for each of the internal representations.
 *
 * vtkSMPVRepresentationProxy is used for pretty much all of the
 * data-representations (i.e. representations showing input data) in the render
 * views. It provides helper functions for controlling transfer functions,
 * scalar coloring, etc.
*/

#ifndef vtkSMPVRepresentationProxy_h
#define vtkSMPVRepresentationProxy_h

#include "vtkPVServerManagerRenderingModule.h" //needed for exports
#include "vtkSMRepresentationProxy.h"

class vtkPVArrayInformation;

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMPVRepresentationProxy
  : public vtkSMRepresentationProxy
{
public:
  static vtkSMPVRepresentationProxy* New();
  vtkTypeMacro(vtkSMPVRepresentationProxy, vtkSMRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns true if scalar coloring is enabled. This checks whether a property
   * named "ColorArrayName" exists and has a non-empty string. This does not
   * check for the validity of the array.
   */
  virtual bool GetUsingScalarColoring();

  //@{
  /**
   * Safely call GetUsingScalarColoring() after casting the proxy to appropriate
   * type.
   */
  static bool GetUsingScalarColoring(vtkSMProxy* proxy)
  {
    vtkSMPVRepresentationProxy* self = vtkSMPVRepresentationProxy::SafeDownCast(proxy);
    return self ? self->GetUsingScalarColoring() : false;
  }
  //@}

  /**
   * Enable/disable scalar coloring using the specified array. This will set up a
   * color and opacity transfer functions using vtkSMTransferFunctionProxy
   * instance. If arrayname is NULL, then scalar coloring is turned off.
   * \c attribute_type must be one of vtkDataObject::AttributeTypes.
   */
  virtual bool SetScalarColoring(const char* arrayname, int attribute_type);

  /**
   * Enable/disable scalar coloring using the specified array. This will set up a
   * color and opacity transfer functions using vtkSMTransferFunctionProxy
   * instance. If arrayname is NULL, then scalar coloring is turned off.
   * \param arrayname the name of the array.
   * \param attribute_type must be one of vtkDataObject::AttributeTypes.
   * \param component enables choosing a component to color with,
   * -1 will change to Magnitude, >=0 will change to corresponding component.
   */
  virtual bool SetScalarColoring(const char* arrayname, int attribute_type, int component);

  //@{
  /**
   * Safely call SetScalarColoring() after casting the proxy to the appropriate
   * type.
   */
  static bool SetScalarColoring(vtkSMProxy* proxy, const char* arrayname, int attribute_type)
  {
    vtkSMPVRepresentationProxy* self = vtkSMPVRepresentationProxy::SafeDownCast(proxy);
    return self ? self->SetScalarColoring(arrayname, attribute_type) : false;
  }
  //@}

  //@{
  /**
   * Safely call SetScalarColoring() after casting the proxy to the appropriate
   * type, component version
   */
  static bool SetScalarColoring(
    vtkSMProxy* proxy, const char* arrayname, int attribute_type, int component)
  {
    vtkSMPVRepresentationProxy* self = vtkSMPVRepresentationProxy::SafeDownCast(proxy);
    return self ? self->SetScalarColoring(arrayname, attribute_type, component) : false;
  }
  //@}

  /**
   * Rescales the color transfer function and opacity transfer function using the
   * current data range. Returns true if rescale was successful.
   * If \c extend is true (false by default), the transfer function range will
   * only be extended as needed to fit the data range.
   * If \c force is false (true by default), then the transfer function range is
   * not changed if locked.
   * @param[in] extend Extend existing range instead of clamping to the new
   * range (default: false).
   * @param[in] force Update transfer function even if the range is locked
   * (default: true).
   */
  virtual bool RescaleTransferFunctionToDataRange(bool extend = false, bool force = true);

  /**
   * Rescales the color transfer function and opacity transfer function using the
   * current data range for the chosen data-array. Returns true if rescale was
   * successful.
   * If \c extend is true (false by default), the transfer function range will
   * only be extended as needed to fit the data range.
   * If \c force is false (true by default), then the transfer function range is
   * not changed if locked.
   * @param arrayname the name of the array.
   * @param attribute_type must be one of vtkDataObject::AttributeTypes.
   * @param[in] extend Extend existing range instead of clamping to the new
   * range (default: false).
   * @param[in] force Update transfer function even if the range is locked
   * (default: true).
   */
  virtual bool RescaleTransferFunctionToDataRange(
    const char* arrayname, int attribute_type, bool extend = false, bool force = true);

  //@{
  /**
   * Safely call RescaleTransferFunctionToDataRange() after casting the proxy to
   * appropriate type.
   */
  static bool RescaleTransferFunctionToDataRange(
    vtkSMProxy* proxy, bool extend = false, bool force = true)
  {
    vtkSMPVRepresentationProxy* self = vtkSMPVRepresentationProxy::SafeDownCast(proxy);
    return self ? self->RescaleTransferFunctionToDataRange(extend, force) : false;
  }
  //@}

  //@{
  /**
   * Safely call RescaleTransferFunctionToDataRange() after casting the proxy to
   * appropriate type.
   */
  static bool RescaleTransferFunctionToDataRange(vtkSMProxy* proxy, const char* arrayname,
    int attribute_type, bool extend = false, bool force = true)
  {
    vtkSMPVRepresentationProxy* self = vtkSMPVRepresentationProxy::SafeDownCast(proxy);
    return self ? self->RescaleTransferFunctionToDataRange(arrayname, attribute_type, extend, force)
                : false;
  }
  //@}

  /**
   * Rescales the color transfer function and opacity transfer function using the
   * current data range over time. Returns true if rescale was successful.
   */
  virtual bool RescaleTransferFunctionToDataRangeOverTime();

  /**
   * Rescales the color transfer function and opacity transfer function using the
   * current data range over time for the chosen data-array. Returns true if rescale was
   * successful. \c field_association must be one of
   * vtkDataObject::AttributeTypes,
   */
  virtual bool RescaleTransferFunctionToDataRangeOverTime(
    const char* arrayname, int attribute_type);

  //@{
  /**
   * Safely call RescaleTransferFunctionToDataRangeOverTime() after casting the proxy to
   * appropriate type.
   */
  static bool RescaleTransferFunctionToDataRangeOverTime(vtkSMProxy* proxy)
  {
    vtkSMPVRepresentationProxy* self = vtkSMPVRepresentationProxy::SafeDownCast(proxy);
    return self ? self->RescaleTransferFunctionToDataRangeOverTime() : false;
  }
  //@}

  //@{
  /**
   * Safely call RescaleTransferFunctionToDataRangeOverTime() after casting the proxy to
   * appropriate type.
   */
  static bool RescaleTransferFunctionToDataRangeOverTime(
    vtkSMProxy* proxy, const char* arrayname, int attribute_type)
  {
    vtkSMPVRepresentationProxy* self = vtkSMPVRepresentationProxy::SafeDownCast(proxy);
    return self ? self->RescaleTransferFunctionToDataRangeOverTime(arrayname, attribute_type)
                : false;
  }
  //@}

  //@{
  /**
   * Rescales the color transfer function and the opacity transfer function
   * using the current data range, limited to the currernt visible elements.
   */
  virtual bool RescaleTransferFunctionToVisibleRange(vtkSMProxy* view);
  virtual bool RescaleTransferFunctionToVisibleRange(
    vtkSMProxy* view, const char* arrayname, int attribute_type);
  //@}

  //@{
  /**
   * Safely call RescaleTransferFunctionToVisibleRange() after casting the proxy
   * to the appropriate type.
   */
  static bool RescaleTransferFunctionToVisibleRange(vtkSMProxy* proxy, vtkSMProxy* view)
  {
    vtkSMPVRepresentationProxy* self = vtkSMPVRepresentationProxy::SafeDownCast(proxy);
    return self ? self->RescaleTransferFunctionToVisibleRange(view) : false;
  }
  static bool RescaleTransferFunctionToVisibleRange(
    vtkSMProxy* proxy, vtkSMProxy* view, const char* arrayname, int attribute_type)
  {
    vtkSMPVRepresentationProxy* self = vtkSMPVRepresentationProxy::SafeDownCast(proxy);
    return self ? self->RescaleTransferFunctionToVisibleRange(view, arrayname, attribute_type)
                : false;
  }
  //@}

  //@{
  /**
   * Set the scalar bar visibility. This will create a new scalar bar as needed.
   * Scalar bar is only shown if scalar coloring is indeed being used.
   */
  virtual bool SetScalarBarVisibility(vtkSMProxy* view, bool visible);
  static bool SetScalarBarVisibility(vtkSMProxy* proxy, vtkSMProxy* view, bool visible)
  {
    vtkSMPVRepresentationProxy* self = vtkSMPVRepresentationProxy::SafeDownCast(proxy);
    return self ? self->SetScalarBarVisibility(view, visible) : false;
  }
  //@}

  //@{
  /**
   * While SetScalarBarVisibility can be used to hide a scalar bar, it will
   * always simply hide the scalar bar even if its being used by some other
   * representation. Use this method instead to only hide the scalar/color bar
   * if no other visible representation in the view is mapping data using the
   * scalar bar.
   */
  virtual bool HideScalarBarIfNotNeeded(vtkSMProxy* view);
  static bool HideScalarBarIfNotNeeded(vtkSMProxy* repr, vtkSMProxy* view)
  {
    vtkSMPVRepresentationProxy* self = vtkSMPVRepresentationProxy::SafeDownCast(repr);
    return self ? self->HideScalarBarIfNotNeeded(view) : false;
  }
  //@}

  //@{
  /**
   * Check scalar bar visibility.  Return true if the scalar bar for this
   * representation and view is visible, return false otherwise.
   */
  virtual bool IsScalarBarVisible(vtkSMProxy* view);
  static bool IsScalarBarVisible(vtkSMProxy* repr, vtkSMProxy* view)
  {
    vtkSMPVRepresentationProxy* self = vtkSMPVRepresentationProxy::SafeDownCast(repr);
    return self ? self->IsScalarBarVisible(view) : false;
  }
  //@}

  //@{
  /**
   * Returns the array information for the data array used for scalar coloring, from input data.
   * If checkRepresentedData is true, it will also check in the represented data. Default is true.
   * If none is found, returns NULL.
   */
  virtual vtkPVArrayInformation* GetArrayInformationForColorArray(bool checkRepresentedData = true);
  static vtkPVArrayInformation* GetArrayInformationForColorArray(
    vtkSMProxy* proxy, bool checkRepresentedData = true)
  {
    vtkSMPVRepresentationProxy* self = vtkSMPVRepresentationProxy::SafeDownCast(proxy);
    return self ? self->GetArrayInformationForColorArray(checkRepresentedData) : nullptr;
  }
  //@}

  //@{
  /**
   * Call vtkSMRepresentationProxy::GetProminentValuesInformation() for the
   * array used for scalar color, if any. Otherwise returns NULL.
   */
  virtual vtkPVProminentValuesInformation* GetProminentValuesInformationForColorArray(
    double uncertaintyAllowed = 1e-6, double fraction = 1e-3, bool force = false);
  static vtkPVProminentValuesInformation* GetProminentValuesInformationForColorArray(
    vtkSMProxy* proxy, double uncertaintyAllowed = 1e-6, double fraction = 1e-3, bool force = false)
  {
    vtkSMPVRepresentationProxy* self = vtkSMPVRepresentationProxy::SafeDownCast(proxy);
    return self
      ? self->GetProminentValuesInformationForColorArray(uncertaintyAllowed, fraction, force)
      : NULL;
  }
  //@}

  /**
   * Get an estimated number of annotation shown on this representation scalar bar
   */
  int GetEstimatedNumberOfAnnotationsOnScalarBar(vtkSMProxy* view);
  static int GetEstimatedNumberOfAnnotationsOnScalarBar(vtkSMProxy* proxy, vtkSMProxy* view)
  {
    vtkSMPVRepresentationProxy* self = vtkSMPVRepresentationProxy::SafeDownCast(proxy);
    return self ? self->GetEstimatedNumberOfAnnotationsOnScalarBar(view) : -1;
  }

  /**
   * Overridden to ensure when picking representation types that require scalar
   * colors, scalar coloring it setup properly. Currently this is hard-coded for
   * Volume and Slice representation types.
   */
  bool SetRepresentationType(const char* type) override;

  /**
   * True if ranges have to be computed independently on component 0 for the color
   * and 1 for the opacity on the Volume representation.
   */
  bool GetVolumeIndependentRanges();

protected:
  vtkSMPVRepresentationProxy();
  ~vtkSMPVRepresentationProxy() override;

  /**
   * Rescales transfer function ranges using the array information provided.
   */
  virtual bool RescaleTransferFunctionToDataRange(
    vtkPVArrayInformation* info, bool extend = false, bool force = true);

  /**
   * Overridden to ensure that the RepresentationTypesInfo and
   * Representations's domain are up-to-date.
   */
  void CreateVTKObjects() override;

  // Whenever the "Representation" property is modified, we ensure that the
  // this->InvalidateDataInformation() is called.
  void OnPropertyUpdated(vtkObject*, unsigned long, void* calldata);

  /**
   * Overridden to ensure that whenever "Input" property changes, we update the
   * "Input" properties for all internal representations (including setting up
   * of the link to the extract-selection representation).
   */
  void SetPropertyModifiedFlag(const char* name, int flag) override;

  /**
   * Overridden to process "RepresentationType" elements.
   */
  int ReadXMLAttributes(vtkSMSessionProxyManager* pm, vtkPVXMLElement* element) override;

  /**
   * In case of UseSeparateColorMap enabled, this function prefix the given
   * arrayname with unique identifier, otherwise it acts as a passthrough.
   */
  std::string GetDecoratedArrayName(const std::string& arrayname);

  /**
   * Internal method to set scalar coloring, do not use directly.
   */
  virtual bool SetScalarColoringInternal(
    const char* arrayname, int attribute_type, bool useComponent, int component);

private:
  vtkSMPVRepresentationProxy(const vtkSMPVRepresentationProxy&) = delete;
  void operator=(const vtkSMPVRepresentationProxy&) = delete;

  bool InReadXMLAttributes;
  class vtkStringSet;
  vtkStringSet* RepresentationSubProxies;
};

#endif
