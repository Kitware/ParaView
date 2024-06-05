// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMPVRepresentationProxy
 * @brief   representation for "Render View" like views in ParaView.
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
 * views. It provides helper functions redirecting to vtkSMColorMapEditorHelper
 * for controlling transfer functions, scalar coloring, etc.
 *
 * @sa vtkSMColorMapEditorHelper
 */

#ifndef vtkSMPVRepresentationProxy_h
#define vtkSMPVRepresentationProxy_h

#include "vtkParaViewDeprecation.h" // for PARAVIEW_DEPRECATED_IN_5_13_0
#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSMRepresentationProxy.h"
#include "vtkSmartPointer.h" // For LastLUTProxy

#include <set>           // needed for std::set
#include <unordered_map> // needed for std::unordered_map

class vtkPVArrayInformation;

class VTKREMOTINGVIEWS_EXPORT vtkSMPVRepresentationProxy : public vtkSMRepresentationProxy
{
public:
  static vtkSMPVRepresentationProxy* New();
  vtkTypeMacro(vtkSMPVRepresentationProxy, vtkSMRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/get last LUT proxy.
   * Used as a memory of what was the last LUT proxy linked to this representation.
   */
  PARAVIEW_DEPRECATED_IN_5_13_0("Use SetLastLookupTable instead")
  void SetLastLUTProxy(vtkSMProxy* proxy) { this->SetLastLookupTable(proxy); }
  void SetLastLookupTable(vtkSMProxy* proxy);
  PARAVIEW_DEPRECATED_IN_5_13_0("Use GetLastLookupTable instead")
  vtkSMProxy* GetLastLUTProxy() { return this->GetLastLookupTable(); }
  vtkSMProxy* GetLastLookupTable();
  void SetLastBlockLookupTable(const std::string& blockSelector, vtkSMProxy* proxy)
  {
    this->SetLastBlocksLookupTable({ blockSelector }, proxy);
  }
  void SetLastBlocksLookupTable(const std::vector<std::string>& blockSelectors, vtkSMProxy* proxy);
  vtkSMProxy* GetLastBlockLookupTable(const std::string& blockSelector)
  {
    return this->GetLastBlocksLookupTables({ blockSelector }).front();
  }
  std::vector<vtkSMProxy*> GetLastBlocksLookupTables(
    const std::vector<std::string>& blockSelectors);
  ///@}

  ///@{
  /**
   * Returns true if scalar coloring is enabled. This checks whether a property
   * named "ColorArrayName" exists and has a non-empty string. This does not
   * check for the validity of the array.
   */
  virtual bool GetUsingScalarColoring();
  virtual bool GetBlockUsingScalarColoring(const std::string& blockSelector)
  {
    return this->GetBlocksUsingScalarColoring({ blockSelector }).front();
  }
  virtual std::vector<vtkTypeBool> GetBlocksUsingScalarColoring(
    const std::vector<std::string>& blockSelectors);
  virtual bool GetAnyBlockUsingScalarColoring();
  ///@}

  ///@{
  /**
   * Returns the lut proxy of this representation in the given view.
   * This method will return `nullptr` if no lut proxy exists in this view.
   */
  PARAVIEW_DEPRECATED_IN_5_13_0("Use GetLookupTable instead")
  vtkSMProxy* GetLUTProxy(vtkSMProxy* view) { return this->GetLookupTable(view); }
  vtkSMProxy* GetLookupTable(vtkSMProxy* view);
  vtkSMProxy* GetBlockLookupTable(vtkSMProxy* view, const std::string& blockSelector)
  {
    return this->GetBlocksLookupTables(view, { blockSelector }).front();
  }
  std::vector<vtkSMProxy*> GetBlocksLookupTables(
    vtkSMProxy* view, const std::vector<std::string>& blockSelectors);
  ///@}

  ///@{
  /**
   * Updates the ranges shown in the scalar bar.
   * If deleteRange is true, then the range stored for current representation proxy is deleted.
   * This should be done when the scalar bar gets separated or becomes not visible.
   * If deleteRange is false, then the range stored for current representation proxy is updated
   * with the new range value.
   */
  bool UpdateScalarBarRange(vtkSMProxy* view, bool deleteRange);
  std::vector<vtkTypeBool> UpdateBlocksScalarBarRange(vtkSMProxy* view, bool deleteRange);
  ///@}

  ///@{
  /**
   * Enable/disable scalar coloring using the specified array. This will set up a
   * color and opacity transfer functions using vtkSMTransferFunctionProxy
   * instance. If arrayName is nullptr, then scalar coloring is turned off.
   * \c attributeType must be one of vtkDataObject::AttributeTypes.
   */
  virtual bool SetScalarColoring(const char* arrayName, int attributeType);
  virtual bool SetBlockScalarColoring(
    const std::string& blockSelector, const char* arrayName, int attributeType)
  {
    return this->SetBlocksScalarColoring({ blockSelector }, arrayName, attributeType).front();
  }
  virtual std::vector<vtkTypeBool> SetBlocksScalarColoring(
    const std::vector<std::string>& blockSelectors, const char* arrayName, int attributeType);
  ///@}

  ///@{
  /**
   * Enable/disable scalar coloring using the specified array. This will set up a
   * color and opacity transfer functions using vtkSMTransferFunctionProxy
   * instance. If arrayName is nullptr, then scalar coloring is turned off. if
   * component name is -1, then the magnitude of the array is used. If component
   * name is >=0, then the corresponding component is used.
   */
  virtual bool SetScalarColoring(const char* arrayName, int attributeType, int component);
  virtual bool SetBlockScalarColoring(
    const std::string& blockSelector, const char* arrayName, int attributeType, int component)
  {
    return this->SetBlocksScalarColoring({ blockSelector }, arrayName, attributeType, component)
      .front();
  }
  virtual std::vector<vtkTypeBool> SetBlocksScalarColoring(
    const std::vector<std::string>& blockSelectors, const char* arrayName, int attributeType,
    int component);
  ///@}

  ///@{
  /**
   * Rescales the color transfer function and opacity transfer function using the
   * current data range. Returns true if rescale was successful.
   * If \c extend is true (false by default), the transfer function range will
   * only be extended as needed to fit the data range.
   * If \c force is false (true by default), then the transfer function range is
   * not changed if locked.
   *
   * blockSelector the block selector.
   * extend Extend existing range instead of clamping to the new
   * range (default: false).
   * force Update transfer function even if the range is locked
   * (default: true).
   */
  virtual bool RescaleTransferFunctionToDataRange(bool extend = false, bool force = true);
  virtual bool RescaleBlockTransferFunctionToDataRange(
    const std::string& blockSelector, bool extend = false, bool force = true)
  {
    return this->RescaleBlocksTransferFunctionToDataRange({ blockSelector }, extend, force).front();
  }
  virtual std::vector<vtkTypeBool> RescaleBlocksTransferFunctionToDataRange(
    const std::vector<std::string>& blockSelectors, bool extend = false, bool force = true);
  ///@}

  ///@{
  /**
   * Rescales the color transfer function and opacity transfer function using the
   * current data range for the chosen data-array. Returns true if rescale was
   * successful.
   * If \c extend is true (false by default), the transfer function range will
   * only be extended as needed to fit the data range.
   * If \c force is false (true by default), then the transfer function range is
   * not changed if locked.
   *
   * blockSelector the block selector.
   * arrayName the name of the array.
   * attributeType must be one of vtkDataObject::AttributeTypes.
   * extend Extend existing range instead of clamping to the new
   * range (default: false).
   * force Update transfer function even if the range is locked
   * (default: true).
   */
  virtual bool RescaleTransferFunctionToDataRange(
    const char* arrayName, int attributeType, bool extend = false, bool force = true);
  virtual bool RescaleBlockTransferFunctionToDataRange(const std::string& blockSelector,
    const char* arrayName, int attributeType, bool extend = false, bool force = true)
  {
    return this
      ->RescaleBlocksTransferFunctionToDataRange(
        { blockSelector }, arrayName, attributeType, extend, force)
      .front();
  }
  virtual std::vector<vtkTypeBool> RescaleBlocksTransferFunctionToDataRange(
    const std::vector<std::string>& blockSelectors, const char* arrayName, int attributeType,
    bool extend = false, bool force = true);
  ///@}

  ///@{
  /**
   * Rescales the color transfer function and opacity transfer function using the
   * current data range over time. Returns true if rescale was successful.
   */
  virtual bool RescaleTransferFunctionToDataRangeOverTime();
  virtual bool RescaleBlockTransferFunctionToDataRangeOverTime(const std::string& blockSelector)
  {
    return this->RescaleBlocksTransferFunctionToDataRangeOverTime({ blockSelector }).front();
  }
  virtual std::vector<vtkTypeBool> RescaleBlocksTransferFunctionToDataRangeOverTime(
    const std::vector<std::string>& blockSelectors);
  ///@}

  ///@{
  /**
   * Rescales the color transfer function and opacity transfer function using the
   * current data range over time for the chosen data-array. Returns true if rescale was
   * successful. \c field_association must be one of
   * vtkDataObject::AttributeTypes,
   */
  virtual bool RescaleTransferFunctionToDataRangeOverTime(const char* arrayName, int attributeType);
  virtual bool RescaleBlockTransferFunctionToDataRangeOverTime(
    const std::string& blockSelector, const char* arrayName, int attributeType)
  {
    return this
      ->RescaleBlocksTransferFunctionToDataRangeOverTime(
        { blockSelector }, arrayName, attributeType)
      .front();
  }
  virtual std::vector<vtkTypeBool> RescaleBlocksTransferFunctionToDataRangeOverTime(
    const std::vector<std::string>& blockSelectors, const char* arrayName, int attributeType);
  ///@}

  ///@{
  /**
   * Rescales the color transfer function and the opacity transfer function
   * using the current data range, limited to the currernt visible elements.
   */
  virtual bool RescaleTransferFunctionToVisibleRange(vtkSMProxy* view);
  virtual bool RescaleTransferFunctionToVisibleRange(
    vtkSMProxy* view, const char* arrayName, int attributeType);
  ///@}

  ///@{
  /**
   * Set the scalar bar visibility. This will create a new scalar bar as needed.
   * Scalar bar is only shown if scalar coloring is indeed being used.
   */
  virtual bool SetScalarBarVisibility(vtkSMProxy* view, bool visible);
  virtual bool SetBlockScalarBarVisibility(
    vtkSMProxy* view, const std::string& blockSelector, bool visible)
  {
    return this->SetBlocksScalarBarVisibility(view, { blockSelector }, visible).front();
  }
  virtual std::vector<vtkTypeBool> SetBlocksScalarBarVisibility(
    vtkSMProxy* view, const std::vector<std::string>& blockSelectors, bool visible);
  ///@}

  ///@{
  /**
   * While SetScalarBarVisibility can be used to hide a scalar bar, it will
   * always simply hide the scalar bar even if its being used by some other
   * representation. Use this method instead to only hide the scalar/color bar
   * if no other visible representation in the view is mapping data using the
   * scalar bar.
   */
  virtual bool HideScalarBarIfNotNeeded(vtkSMProxy* view);
  virtual bool HideBlocksScalarBarIfNotNeeded(vtkSMProxy* view);
  ///@}

  ///@{
  /**
   * Check scalar bar visibility.  Return true if the scalar bar for this
   * representation and view is visible, return false otherwise.
   */
  virtual bool IsScalarBarVisible(vtkSMProxy* view);
  virtual bool IsBlockScalarBarVisible(vtkSMProxy* view, const std::string& blockSelector)
  {
    return this->IsBlocksScalarBarVisible(view, { blockSelector }).front();
  }
  virtual std::vector<vtkTypeBool> IsBlocksScalarBarVisible(
    vtkSMProxy* view, const std::vector<std::string>& blockSelectors);
  ///@}

  ///@{
  /**
   * Returns the array information for the data array used for scalar coloring, from input data.
   * If checkRepresentedData is true, it will also check in the represented data. Default is true.
   * If none is found, returns nullptr.
   */
  virtual vtkPVArrayInformation* GetArrayInformationForColorArray(bool checkRepresentedData = true);
  virtual vtkPVArrayInformation* GetBlockArrayInformationForColorArray(
    const std::string& blockSelector)
  {
    return this->GetBlocksArrayInformationForColorArray({ blockSelector }).front();
  }
  virtual std::vector<vtkPVArrayInformation*> GetBlocksArrayInformationForColorArray(
    const std::vector<std::string>& blockSelectors);
  ///@}

  ///@{
  /**
   * Call vtkSMRepresentationProxy::GetProminentValuesInformation() for the
   * array used for scalar color, if any. Otherwise returns nullptr.
   */
  virtual vtkPVProminentValuesInformation* GetProminentValuesInformationForColorArray(
    double uncertaintyAllowed = 1e-6, double fraction = 1e-3, bool force = false);
  virtual vtkPVProminentValuesInformation* GetBlockProminentValuesInformationForColorArray(
    const std::string& blockSelector, double uncertaintyAllowed = 1e-6, double fraction = 1e-3,
    bool force = false)
  {
    return this
      ->GetBlocksProminentValuesInformationForColorArray(
        { blockSelector }, uncertaintyAllowed, fraction, force)
      .front();
  }
  virtual std::vector<vtkPVProminentValuesInformation*>
  GetBlocksProminentValuesInformationForColorArray(const std::vector<std::string>& blockSelectors,
    double uncertaintyAllowed = 1e-6, double fraction = 1e-3, bool force = false);
  ///@}

  ///@{
  /**
   * Get an estimated number of annotation shown on this representation scalar bar
   */
  int GetEstimatedNumberOfAnnotationsOnScalarBar(vtkSMProxy* view);
  int GetBlockEstimatedNumberOfAnnotationsOnScalarBar(
    vtkSMProxy* view, const std::string& blockSelector)
  {
    return this->GetBlocksEstimatedNumberOfAnnotationsOnScalarBars(view, { blockSelector }).front();
  }
  std::vector<int> GetBlocksEstimatedNumberOfAnnotationsOnScalarBars(
    vtkSMProxy* view, const std::vector<std::string>& blockSelectors);
  ///@}

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

  ///@{
  /**
   * Checks if the scalar bar of this representation in view
   * is sticky visible, i.e. should be visible whenever this representation
   * is also visible.
   * It returns 1 if the scalar bar is sticky visible, 0 other wise.
   * If any problem is encountered, for example if view == nullptr,
   * or if the scalar bar representation is not instanciated / found,
   * it returns -1.
   */
  int IsScalarBarStickyVisible(vtkSMProxy* view);
  int IsBlockScalarBarStickyVisible(vtkSMProxy* view, const std::string& blockSelector)
  {
    return this->IsBlocksScalarBarStickyVisible(view, { blockSelector }).front();
  }
  std::vector<int> IsBlocksScalarBarStickyVisible(
    vtkSMProxy* view, const std::vector<std::string>& blockSelectors);
  ///@}

  /**
   * Called after the view updates.
   */
  void ViewUpdated(vtkSMProxy* view) override;

  ///@{
  /**
   * Set/Get the block color array name.
   */
  std::pair<int, std::string> GetBlockColorArray(const std::string& blockSelector)
  {
    return this->GetBlocksColorArrays({ blockSelector }).front();
  }
  std::vector<std::pair<int, std::string>> GetBlocksColorArrays(
    const std::vector<std::string>& blockSelectors);
  int GetBlockColorArrayAssociation(const std::string& blockSelector)
  {
    return this->GetBlockColorArray(blockSelector).first;
  }
  std::string GetBlockColorArrayName(const std::string& blockSelector)
  {
    return this->GetBlockColorArray(blockSelector).second;
  }
  ///@}

  ///@{
  /**
   * Set/Get if we should use a separate color map for this block.
   */
  void SetBlockUseSeparateColorMap(const std::string& blockSelector, bool use)
  {
    this->SetBlocksUseSeparateColorMap({ blockSelector }, use);
  }
  void SetBlocksUseSeparateColorMap(const std::vector<std::string>& blockSelectors, bool use);
  bool GetBlockUseSeparateColorMap(const std::string& blockSelector)
  {
    return this->GetBlocksUseSeparateColorMaps({ blockSelector }).front();
  }
  std::vector<vtkTypeBool> GetBlocksUseSeparateColorMaps(
    const std::vector<std::string>& blockSelectors);
  ///@}

protected:
  vtkSMPVRepresentationProxy();
  ~vtkSMPVRepresentationProxy() override;

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
   * Used as a memory of what was the last LUT proxy linked to this representation.
   * This is used in `UpdateScalarBarRange` to update the scalar bar range when
   * turning off the coloring for this representation.
   */
  vtkSmartPointer<vtkSMProxy> LastLookupTable;
  std::unordered_map<std::string, vtkSmartPointer<vtkSMProxy>> LastBlocksLookupTables;

private:
  vtkSMPVRepresentationProxy(const vtkSMPVRepresentationProxy&) = delete;
  void operator=(const vtkSMPVRepresentationProxy&) = delete;

  bool InReadXMLAttributes;
  std::set<std::string> RepresentationSubProxies;
};

#endif
