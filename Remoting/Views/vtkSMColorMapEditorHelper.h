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

#include "vtkObject.h"              // Superclass
#include "vtkParaViewDeprecation.h" // For PARAVIEW_DEPRECATED_IN_5_13_0
#include "vtkRemotingViewsModule.h" // needed for exports
#include "vtkSmartPointer.h"        // For LastLUTProxy

#include <array>   // For array
#include <cstdint> // For int
#include <map>     // For map
#include <string>  // For string
#include <vector>  // For vector

class vtkPVArrayInformation;
class vtkPVProminentValuesInformation;
class vtkSMProperty;
class vtkSMProxy;
class vtkSMPVRepresentationProxy;

// template <bool UseBlockProperties = false>
class VTKREMOTINGVIEWS_EXPORT vtkSMColorMapEditorHelper : public vtkObject
{
public:
  static vtkSMColorMapEditorHelper* New();
  vtkTypeMacro(vtkSMColorMapEditorHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum SelectedPropertiesTypes
  {
    Representation = 0,
    Blocks = 1
  };

  enum BlockPropertyState : std::uint8_t
  {
    // Property is disabled because it can not be edited
    Disabled = 0x0,
    // Property is inherited from the representation
    RepresentationInherited = 0x1,
    // Property is inherited from a block
    BlockInherited = 0x2,
    // Property is inherited from a block and the representation
    MixedInherited = 0x3,
    // Property is set in all blocks
    Set = 0x4,
    // Property is set in some blocks and representation inherited
    SetAndRepresentationInherited = 0x5,
    // Property is set in some blocks and block inherited
    SetAndBlockInherited = 0x6,
    // Property is set in some blocks and mixed inherited
    SetAndMixedInherited = 0x7,
    // Number of possible states
    NumberOfStates = 0x8
  };

  ///@{
  /**
   * Set/Get the selected properties type.
   *
   * This variable is used in all instance functions with the Selected key-word in the name.
   *
   * It decides whether to use a function that uses the representation's or blocks' properties.
   *
   * Default is false.
   */
  vtkSetClampMacro(SelectedPropertiesType, int, SelectedPropertiesTypes::Representation,
    SelectedPropertiesTypes::Blocks);
  void SetSelectedPropertiesTypeToRepresentation()
  {
    this->SetSelectedPropertiesType(SelectedPropertiesTypes::Representation);
  }
  void SetSelectedPropertiesTypeToBlocks()
  {
    this->SetSelectedPropertiesType(SelectedPropertiesTypes::Blocks);
  }
  vtkGetMacro(SelectedPropertiesType, int);
  ///@}

  /**
   * Determine if a property is a block property.
   */
  static SelectedPropertiesTypes GetPropertyType(vtkSMProperty* property);

  ///@{
  /**
   * Determine if a propert(y/ies) of a selector(s) is set, inherited from a block, or from the
   * representation, or mixed.
   *
   * if it's RepresentationInherited, the blockSelector will be empty.
   * if it's BlockInherited, the blockSelector will be the inherited block.
   * if it's MixedInherited, the blockSelector will be the inherited block.
   * if it's Set, the block selector will be the block where the property is set.
   * if it's SetAndRepresentationInherited, be the block where the property is set.
   * if it's SetAndBlockInherited, be the block where the property is set.
   * if it's SetAndMixedInherited, be the block where the property is set.
   */
  static std::pair<std::string, BlockPropertyState> HasBlockProperty(
    vtkSMProxy* proxy, const std::string& blockSelector, const std::string& propertyName);
  static std::pair<std::string, BlockPropertyState> GetBlockPropertyStateFromBlockPropertyStates(
    const std::vector<std::pair<std::string, BlockPropertyState>>& states);
  static std::pair<std::string, BlockPropertyState> HasBlockProperty(vtkSMProxy* proxy,
    const std::vector<std::string>& blockSelectors, const std::string& propertyName);
  static std::pair<std::string, BlockPropertyState> HasBlockProperties(vtkSMProxy* proxy,
    const std::string& blockSelector, const std::vector<std::string>& propertyNames);
  static std::pair<std::string, BlockPropertyState> HasBlocksProperties(vtkSMProxy* proxy,
    const std::vector<std::string>& blockSelectors, const std::vector<std::string>& propertyNames);
  ///@}

  /**
   * Returns the selected block selectors for this representation.
   */
  static std::vector<std::string> GetSelectedBlockSelectors(vtkSMProxy* proxy);

  /**
   * Returns the selectors that use a color array for this representation.
   */
  static std::vector<std::string> GetColorArraysBlockSelectors(vtkSMProxy* proxy);

  ///@{
  /**
   * Returns the lut proxy of this representation in the given view.
   *
   * The methods with a `view` parameter will return `nullptr` if the view is not a render view.
   */
  PARAVIEW_DEPRECATED_IN_5_13_0("Use GetLookupTable instead")
  static vtkSMProxy* GetLUTProxy(vtkSMProxy* proxy, vtkSMProxy* view)
  {
    return vtkSMColorMapEditorHelper::GetLookupTable(proxy, view);
  }
  static vtkSMProxy* GetLookupTable(vtkSMProxy* proxy);
  static vtkSMProxy* GetLookupTable(vtkSMProxy* proxy, vtkSMProxy* view);
  static vtkSMProxy* GetBlockLookupTable(vtkSMProxy* proxy, const std::string& blockSelector)
  {
    return vtkSMColorMapEditorHelper::GetBlocksLookupTables(proxy, { blockSelector }).front();
  }
  static std::vector<vtkSMProxy*> GetBlocksLookupTables(
    vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors);
  static vtkSMProxy* GetBlockLookupTable(
    vtkSMProxy* proxy, vtkSMProxy* view, const std::string& blockSelector)
  {
    return vtkSMColorMapEditorHelper::GetBlocksLookupTables(proxy, view, { blockSelector }).front();
  }
  static std::vector<vtkSMProxy*> GetBlocksLookupTables(
    vtkSMProxy* proxy, vtkSMProxy* view, const std::vector<std::string>& blockSelectors);
  std::vector<vtkSMProxy*> GetSelectedLookupTables(vtkSMProxy* proxy);
  std::vector<vtkSMProxy*> GetSelectedLookupTables(vtkSMProxy* proxy, vtkSMProxy* view);
  ///@}

  ///@{
  /**
   * Returns true if scalar coloring is enabled. This checks whether a property
   * named "ColorArrayName" exists and has a non-empty string. This does not
   * check for the validity of the array.
   */
  static bool GetUsingScalarColoring(vtkSMProxy* proxy);
  static bool GetBlockUsingScalarColoring(vtkSMProxy* proxy, const std::string& blockSelector)
  {
    return vtkSMColorMapEditorHelper::GetBlocksUsingScalarColoring(proxy, { blockSelector })
      .front();
  }
  static std::vector<vtkTypeBool> GetBlocksUsingScalarColoring(
    vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors);
  static bool GetAnyBlockUsingScalarColoring(vtkSMProxy* proxy);
  std::vector<vtkTypeBool> GetSelectedUsingScalarColorings(vtkSMProxy* proxy);
  bool GetAnySelectedUsingScalarColoring(vtkSMProxy* proxy);
  ///@}

  ///@{
  /**
   * Given the input registered representation `proxy`, sets up a lookup table associated with the
   * representation if a scalar bar is being used for `proxy`.
   */
  static void SetupLookupTable(vtkSMProxy* proxy);
  static void SetupBlocksLookupTables(vtkSMProxy* proxy);
  ///@}

  ///@{
  /**
   * Updates the ranges shown in the scalar bar.
   * If deleteRange is true, then the range stored for current representation proxy is deleted.
   * This should be done when the scalar bar gets separated or becomes not visible.
   * If deleteRange is false, then the range stored for current representation proxy is updated
   * with the new range value.
   */
  static bool UpdateScalarBarRange(vtkSMProxy* proxy, vtkSMProxy* view, bool deleteRange);
  static std::vector<vtkTypeBool> UpdateBlocksScalarBarRange(
    vtkSMProxy* proxy, vtkSMProxy* view, bool deleteRange);
  ///@}

  ///@{
  /**
   * Enable/disable scalar coloring using the specified array. This will set up a
   * color and opacity transfer functions using vtkSMTransferFunctionProxy
   * instance. If arrayName is nullptr, then scalar coloring is turned off.
   * \c attributeType must be one of vtkDataObject::AttributeTypes.
   */
  static bool SetScalarColoring(vtkSMProxy* proxy, const char* arrayName, int attributeType);
  static bool SetBlockScalarColoring(
    vtkSMProxy* proxy, const std::string& blockSelector, const char* arrayName, int attributeType)
  {
    return vtkSMColorMapEditorHelper::SetBlocksScalarColoring(
      proxy, { blockSelector }, arrayName, attributeType)
      .front();
  }
  static std::vector<vtkTypeBool> SetBlocksScalarColoring(vtkSMProxy* proxy,
    const std::vector<std::string>& blockSelectors, const char* arrayName, int attributeType);
  std::vector<vtkTypeBool> SetSelectedScalarColoring(
    vtkSMProxy* proxy, const char* arrayName, int attributeType);
  ///@}

  ///@{
  /**
   * Enable/disable scalar coloring using the specified array. This will set up a
   * color and opacity transfer functions using vtkSMTransferFunctionProxy
   * instance. If arrayName is nullptr, then scalar coloring is turned off.
   * \c attributeType must be one of vtkDataObject::AttributeTypes.
   * \c component enables choosing a component to color with,
   * -1 will change to Magnitude, >=0 will change to corresponding component.
   */
  static bool SetScalarColoring(
    vtkSMProxy* proxy, const char* arrayName, int attributeType, int component);
  static bool SetBlockScalarColoring(vtkSMProxy* proxy, const std::string& blockSelector,
    const char* arrayName, int attributeType, int component)
  {
    return vtkSMColorMapEditorHelper::SetBlocksScalarColoring(
      proxy, { blockSelector }, arrayName, attributeType, component)
      .front();
  }
  static std::vector<vtkTypeBool> SetBlocksScalarColoring(vtkSMProxy* proxy,
    const std::vector<std::string>& blockSelectors, const char* arrayName, int attributeType,
    int component);
  std::vector<vtkTypeBool> SetSelectedScalarColoring(
    vtkSMProxy* proxy, const char* arrayName, int attributeType, int component);
  ///@}

  ///@{
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
  static bool RescaleBlockTransferFunctionToDataRange(
    vtkSMProxy* proxy, const std::string& blockSelector, bool extend = false, bool force = true)
  {
    return vtkSMColorMapEditorHelper::RescaleBlocksTransferFunctionToDataRange(
      proxy, { blockSelector }, extend, force)
      .front();
  }
  static std::vector<vtkTypeBool> RescaleBlocksTransferFunctionToDataRange(vtkSMProxy* proxy,
    const std::vector<std::string>& blockSelectors, bool extend = false, bool force = true);
  std::vector<vtkTypeBool> RescaleSelectedTransferFunctionToDataRange(
    vtkSMProxy* proxy, bool extend = false, bool force = true);
  ///@}

  ///@{
  /**
   * Rescales the color transfer function and opacity transfer function using the
   * current data range for the chosen data-array. Returns true if rescale was
   * successful.
   * \c attributeType must be one of vtkDataObject::AttributeTypes.
   * If \c extend is true (false by default), the transfer function range will
   * only be extended as needed to fit the data range.
   * If \c force is false (true by default), then the transfer function range is
   * not changed if locked.
   */
  static bool RescaleTransferFunctionToDataRange(vtkSMProxy* proxy, const char* arrayName,
    int attributeType, bool extend = false, bool force = true);
  static bool RescaleBlockTransferFunctionToDataRange(vtkSMProxy* proxy,
    const std::string& blockSelector, const char* arrayName, int attributeType, bool extend = false,
    bool force = true)
  {
    return vtkSMColorMapEditorHelper::RescaleBlocksTransferFunctionToDataRange(
      proxy, { blockSelector }, arrayName, attributeType, extend, force)
      .front();
  }
  static std::vector<vtkTypeBool> RescaleBlocksTransferFunctionToDataRange(vtkSMProxy* proxy,
    const std::vector<std::string>& blockSelectors, const char* arrayName, int attributeType,
    bool extend = false, bool force = true);
  std::vector<vtkTypeBool> RescaleSelectedTransferFunctionToDataRange(vtkSMProxy* proxy,
    const char* arrayName, int attributeType, bool extend = false, bool force = true);
  ///@}

  ///@{
  /**
   * Rescales the color transfer function and opacity transfer function using the
   * current data range over time. Returns true if rescale was successful.
   */
  static bool RescaleTransferFunctionToDataRangeOverTime(vtkSMProxy* proxy);
  static bool RescaleBlockTransferFunctionToDataRangeOverTime(
    vtkSMProxy* proxy, const std::string& blockSelector)
  {
    return vtkSMColorMapEditorHelper::RescaleBlocksTransferFunctionToDataRangeOverTime(
      proxy, { blockSelector })
      .front();
  }
  static std::vector<vtkTypeBool> RescaleBlocksTransferFunctionToDataRangeOverTime(
    vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors);
  std::vector<vtkTypeBool> RescaleSelectedTransferFunctionToDataRangeOverTime(vtkSMProxy* proxy);
  ///@}

  ///@{
  /**
   * Rescales the color transfer function and opacity transfer function using the
   * current data range over time for the chosen data-array. Returns true if rescale
   * was successful.
   * \c attributeType must be one of vtkDataObject::AttributeTypes,
   */
  static bool RescaleTransferFunctionToDataRangeOverTime(
    vtkSMProxy* proxy, const char* arrayName, int attributeType);
  static bool RescaleBlockTransferFunctionToDataRangeOverTime(
    vtkSMProxy* proxy, const std::string& blockSelector, const char* arrayName, int attributeType)
  {
    return vtkSMColorMapEditorHelper::RescaleBlocksTransferFunctionToDataRangeOverTime(
      proxy, { blockSelector }, arrayName, attributeType)
      .front();
  }
  static std::vector<vtkTypeBool> RescaleBlocksTransferFunctionToDataRangeOverTime(
    vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors, const char* arrayName,
    int attributeType);
  std::vector<vtkTypeBool> RescaleSelectedTransferFunctionToDataRangeOverTime(
    vtkSMProxy* proxy, const char* arrayName, int attributeType);
  ///@}

  ///@{
  /**
   * Rescales the color transfer function and the opacity transfer function
   * using the current data range, limited to the current visible elements.
   */
  static bool RescaleTransferFunctionToVisibleRange(vtkSMProxy* proxy, vtkSMProxy* view);
  static bool RescaleTransferFunctionToVisibleRange(
    vtkSMProxy* proxy, vtkSMProxy* view, const char* arrayName, int attributeType);
  ///@}

  ///@{
  /**
   * Set the scalar bar visibility. This will create a new scalar bar as needed.
   * Scalar bar is only shown if scalar coloring is indeed being used.
   */
  static bool SetScalarBarVisibility(vtkSMProxy* proxy, vtkSMProxy* view, bool visible);
  static bool SetBlockScalarBarVisibility(
    vtkSMProxy* proxy, vtkSMProxy* view, const std::string& blockSelector, bool visible)
  {
    return vtkSMColorMapEditorHelper::SetBlocksScalarBarVisibility(
      proxy, view, { blockSelector }, visible)
      .front();
  }
  static std::vector<vtkTypeBool> SetBlocksScalarBarVisibility(vtkSMProxy* proxy, vtkSMProxy* view,
    const std::vector<std::string>& blockSelectors, bool visible);
  std::vector<vtkTypeBool> SetSelectedScalarBarVisibility(
    vtkSMProxy* proxy, vtkSMProxy* view, bool visible);
  ///@}

  ///@{
  /**
   * While SetScalarBarVisibility can be used to hide a scalar bar, it will
   * always simply hide the scalar bar even if its being used by some other
   * representation. Use this method instead to only hide the scalar/color bar
   * if no other visible representation in the view is mapping data using the
   * scalar bar.
   */
  static bool HideScalarBarIfNotNeeded(vtkSMProxy* repr, vtkSMProxy* view);
  static bool HideBlocksScalarBarIfNotNeeded(vtkSMProxy* repr, vtkSMProxy* view);
  ///@}

  ///@{
  /**
   * Check scalar bar visibility.  Return true if the scalar bar for this
   * representation and view is visible, return false otherwise.
   */
  static bool IsScalarBarVisible(vtkSMProxy* repr, vtkSMProxy* view);
  static bool IsBlockScalarBarVisible(
    vtkSMProxy* repr, vtkSMProxy* view, const std::string& blockSelector)
  {
    return vtkSMColorMapEditorHelper::IsBlocksScalarBarVisible(repr, view, { blockSelector })
      .front();
  }
  static std::vector<vtkTypeBool> IsBlocksScalarBarVisible(
    vtkSMProxy* repr, vtkSMProxy* view, const std::vector<std::string>& blockSelectors);
  ///@}

  ///@{
  /**
   * Returns the array information for the data array used for scalar coloring, from input data.
   * If checkRepresentedData is true, it will also check in the represented data. Default is true.
   * If none is found, returns nullptr.
   */
  static vtkPVArrayInformation* GetArrayInformationForColorArray(
    vtkSMProxy* proxy, bool checkRepresentedData = true);
  static vtkPVArrayInformation* GetBlockArrayInformationForColorArray(
    vtkSMProxy* proxy, const std::string& blockSelector)
  {
    return vtkSMColorMapEditorHelper::GetBlocksArrayInformationForColorArray(
      proxy, { blockSelector })
      .front();
  }
  static std::vector<vtkPVArrayInformation*> GetBlocksArrayInformationForColorArray(
    vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors);
  ///@}

  ///@{
  /**
   * In case of UseSeparateColorMap enabled, this function prefix the given
   * arrayName with unique identifier, otherwise it acts as a passthrough.
   */
  static std::string GetDecoratedArrayName(vtkSMProxy* proxy, const std::string& arrayName);
  static std::string GetBlockDecoratedArrayName(
    vtkSMProxy* proxy, const std::string& blockSelector, const std::string& arrayName)
  {
    return vtkSMColorMapEditorHelper::GetBlocksDecoratedArrayNames(
      proxy, { blockSelector }, arrayName)
      .front();
  }
  static std::vector<std::string> GetBlocksDecoratedArrayNames(vtkSMProxy* proxy,
    const std::vector<std::string>& blockSelectors, const std::string& arrayName);
  ///@}

  ///@{
  /**
   * Call vtkSMRepresentationProxy::GetProminentValuesInformation() for the
   * array used for scalar color, if any. Otherwise returns nullptr.
   */
  static vtkPVProminentValuesInformation* GetProminentValuesInformationForColorArray(
    vtkSMProxy* proxy, double uncertaintyAllowed = 1e-6, double fraction = 1e-3,
    bool force = false);
  static vtkPVProminentValuesInformation* GetBlockProminentValuesInformationForColorArray(
    vtkSMProxy* proxy, const std::string& blockSelector, double uncertaintyAllowed = 1e-6,
    double fraction = 1e-3, bool force = false)
  {
    return vtkSMColorMapEditorHelper::GetBlocksProminentValuesInformationForColorArray(
      proxy, { blockSelector }, uncertaintyAllowed, fraction, force)
      .front();
  }
  static std::vector<vtkPVProminentValuesInformation*>
  GetBlocksProminentValuesInformationForColorArray(vtkSMProxy* proxy,
    const std::vector<std::string>& blockSelectors, double uncertaintyAllowed = 1e-6,
    double fraction = 1e-3, bool force = false);
  ///@}

  ///@{
  /**
   * Get an estimated number of annotation shown on this representation scalar bar
   *
   * If the return value is -1, then the value is not set.
   */
  static int GetEstimatedNumberOfAnnotationsOnScalarBar(vtkSMProxy* proxy, vtkSMProxy* view);
  static int GetBlockEstimatedNumberOfAnnotationsOnScalarBar(
    vtkSMProxy* proxy, vtkSMProxy* view, const std::string& blockSelector)
  {
    return vtkSMColorMapEditorHelper::GetBlocksEstimatedNumberOfAnnotationsOnScalarBars(
      proxy, view, { blockSelector })
      .front();
  }
  static std::vector<int> GetBlocksEstimatedNumberOfAnnotationsOnScalarBars(
    vtkSMProxy* proxy, vtkSMProxy* view, const std::vector<std::string>& blockSelectors);
  std::vector<int> GetSelectedEstimatedNumberOfAnnotationsOnScalarBars(
    vtkSMProxy* proxy, vtkSMProxy* view);
  int GetAnySelectedEstimatedNumberOfAnnotationsOnScalarBar(vtkSMProxy* proxy, vtkSMProxy* view);
  ///@}

  ///@{
  /**
   * Checks if the scalar bar of this representation in view
   * is sticky visible, i.e. should be visible whenever this representation
   * is also visible.
   * It returns 1 if the scalar bar is sticky visible, 0 other wise.
   * If any problem is encountered, for example if view == nullptr,
   * or if the scalar bar representation is not instantiated / found,
   * it returns -1.
   */
  static int IsScalarBarStickyVisible(vtkSMProxy* proxy, vtkSMProxy* view);
  static int IsBlockScalarBarStickyVisible(
    vtkSMProxy* proxy, vtkSMProxy* view, const std::string& blockSelector)
  {
    return vtkSMColorMapEditorHelper::IsBlocksScalarBarStickyVisible(proxy, view, { blockSelector })
      .front();
  }
  static std::vector<int> IsBlocksScalarBarStickyVisible(
    vtkSMProxy* proxy, vtkSMProxy* view, const std::vector<std::string>& blockSelectors);
  ///@}

  using Color = std::array<double, 3>;
  ///@{
  /**
   * Set/Get the color of the representation.
   *
   * @note Use `IsColorValid` to check if the returned value is valid/set.
   */
  static bool IsColorValid(Color color);
  static void SetColor(vtkSMProxy* proxy, Color color);
  static void SetBlockColor(vtkSMProxy* proxy, const std::string& blockSelector, Color color)
  {
    vtkSMColorMapEditorHelper::SetBlocksColor(proxy, { blockSelector }, color);
  }
  static void SetBlocksColor(
    vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors, Color color);
  static void RemoveBlockColor(vtkSMProxy* proxy, const std::string& blockSelector)
  {
    vtkSMColorMapEditorHelper::RemoveBlocksColors(proxy, { blockSelector });
  }
  static void RemoveBlocksColors(vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors);
  void SetSelectedColor(vtkSMProxy* proxy, Color color);
  static Color GetColor(vtkSMProxy* proxy);
  static Color GetBlockColor(vtkSMProxy* proxy, const std::string& blockSelector)
  {
    return vtkSMColorMapEditorHelper::GetBlocksColors(proxy, { blockSelector }).front();
  }
  static std::vector<Color> GetBlocksColors(
    vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors);
  std::vector<Color> GetSelectedColors(vtkSMProxy* proxy);
  ///@}

  ///@{
  /**
   * Get Color array property
   */
  static vtkSMProperty* GetColorArrayProperty(vtkSMProxy* proxy);
  static vtkSMProperty* GetBlockColorArrayProperty(vtkSMProxy* proxy);
  vtkSMProperty* GetSelectedColorArrayProperty(vtkSMProxy* proxy);
  ///@}

  using ColorArray = std::pair<int, std::string>;
  ///@{
  /**
   * Get the color array name.
   *
   * @note Use `IsColorArrayValid` to check if the returned value is valid/set.
   */
  static bool IsColorArrayValid(const ColorArray& array);
  static std::vector<ColorArray> GetBlocksColorArrays(
    vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors);
  static int GetBlockColorArrayAssociation(vtkSMProxy* proxy, const std::string& blockSelector)
  {
    return vtkSMColorMapEditorHelper::GetBlockColorArray(proxy, blockSelector).first;
  }
  static std::string GetBlockColorArrayName(vtkSMProxy* proxy, const std::string& blockSelector)
  {
    return vtkSMColorMapEditorHelper::GetBlockColorArray(proxy, blockSelector).second;
  }
  static std::map<ColorArray, std::vector<std::string>> GetCommonColorArraysBlockSelectors(
    vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors);
  std::vector<ColorArray> GetSelectedColorArrays(vtkSMProxy* proxy);
  ///@}

  ///@{
  /**
   * Get use separate color map property
   */
  static vtkSMProperty* GetUseSeparateColorMapProperty(vtkSMProxy* proxy);
  static vtkSMProperty* GetBlockUseSeparateColorMapProperty(vtkSMProxy* proxy);
  vtkSMProperty* GetSelectedUseSeparateColorMapProperty(vtkSMProxy* proxy);
  ///@}

  ///@{
  /**
   * Set/Get if we should use a separate color map.
   *
   * @note Use `IsUseSeparateColorMapValid` to check if the returned value is valid/set.
   */
  static bool IsUseSeparateColorMapValid(int useSeparateColorMap);
  static void SetUseSeparateColorMap(vtkSMProxy* proxy, bool use);
  static void SetBlockUseSeparateColorMap(
    vtkSMProxy* proxy, const std::string& blockSelector, bool use)
  {
    vtkSMColorMapEditorHelper::SetBlocksUseSeparateColorMap(proxy, { blockSelector }, use);
  }
  static void SetBlocksUseSeparateColorMap(
    vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors, bool use);
  static void RemoveBlockUseSeparateColorMap(vtkSMProxy* proxy, const std::string& blockSelector)
  {
    vtkSMColorMapEditorHelper::RemoveBlocksUseSeparateColorMaps(proxy, { blockSelector });
  }
  static void RemoveBlocksUseSeparateColorMaps(
    vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors);
  void SetSelectedUseSeparateColorMap(vtkSMProxy* proxy, bool use);
  static bool GetUseSeparateColorMap(vtkSMProxy* proxy);
  static int GetBlockUseSeparateColorMap(vtkSMProxy* proxy, const std::string& blockSelector)
  {
    return vtkSMColorMapEditorHelper::GetBlocksUseSeparateColorMaps(proxy, { blockSelector })
      .front();
  }
  static std::vector<int> GetBlocksUseSeparateColorMaps(
    vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors);
  std::vector<int> GetSelectedUseSeparateColorMaps(vtkSMProxy* proxy);
  bool GetAnySelectedUseSeparateColorMap(vtkSMProxy* proxy);
  ///@}

  ///@{
  /**
   * Set/Get the map scalars of the representation.
   *
   * @note Use `IsMapScalarsValid` to check if the returned value is valid/set.
   */
  static bool IsMapScalarsValid(int mapScalars);
  static void SetMapScalars(vtkSMProxy* proxy, bool mapScalars);
  static void SetBlockMapScalars(
    vtkSMProxy* proxy, const std::string& blockSelector, bool mapScalars)
  {
    vtkSMColorMapEditorHelper::SetBlocksMapScalars(proxy, { blockSelector }, mapScalars);
  }
  static void SetBlocksMapScalars(
    vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors, bool mapScalars);
  static void RemoveBlockMapScalars(vtkSMProxy* proxy, const std::string& blockSelector)
  {
    vtkSMColorMapEditorHelper::RemoveBlocksMapScalars(proxy, { blockSelector });
  }
  static void RemoveBlocksMapScalars(
    vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors);
  void SetSelectedMapScalars(vtkSMProxy* proxy, bool mapScalars);
  static bool GetMapScalars(vtkSMProxy* proxy);
  static int GetBlockMapScalars(vtkSMProxy* proxy, const std::string& blockSelector)
  {
    return vtkSMColorMapEditorHelper::GetBlocksMapScalars(proxy, { blockSelector }).front();
  }
  static std::vector<int> GetBlocksMapScalars(
    vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors);
  std::vector<int> GetSelectedMapScalars(vtkSMProxy* proxy);
  bool GetAnySelectedMapScalars(vtkSMProxy* proxy);
  ///@}

  ///@{
  /**
   * Set/Get the map scalars of the representation.
   *
   * @note Use `IsInterpolateScalarsBeforeMappingValid` to check if the returned value is valid/set.
   */
  static bool IsInterpolateScalarsBeforeMappingValid(int interpolate);
  static void SetInterpolateScalarsBeforeMapping(vtkSMProxy* proxy, bool interpolate);
  static void SetBlockInterpolateScalarsBeforeMapping(
    vtkSMProxy* proxy, const std::string& blockSelector, bool interpolate)
  {
    vtkSMColorMapEditorHelper::SetBlocksInterpolateScalarsBeforeMapping(
      proxy, { blockSelector }, interpolate);
  }
  static void SetBlocksInterpolateScalarsBeforeMapping(
    vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors, bool interpolate);
  static void RemoveBlockInterpolateScalarsBeforeMapping(
    vtkSMProxy* proxy, const std::string& blockSelector)
  {
    vtkSMColorMapEditorHelper::RemoveBlocksInterpolateScalarsBeforeMappings(
      proxy, { blockSelector });
  }
  static void RemoveBlocksInterpolateScalarsBeforeMappings(
    vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors);
  void SetSelectedInterpolateScalarsBeforeMapping(vtkSMProxy* proxy, bool interpolate);
  static bool GetInterpolateScalarsBeforeMapping(vtkSMProxy* proxy);
  static int GetBlockInterpolateScalarsBeforeMapping(
    vtkSMProxy* proxy, const std::string& blockSelector)
  {
    return vtkSMColorMapEditorHelper::GetBlocksInterpolateScalarsBeforeMappings(
      proxy, { blockSelector })
      .front();
  }
  static std::vector<int> GetBlocksInterpolateScalarsBeforeMappings(
    vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors);
  std::vector<int> GetSelectedInterpolateScalarsBeforeMappings(vtkSMProxy* proxy);
  bool GetAnySelectedInterpolateScalarsBeforeMapping(vtkSMProxy* proxy);
  ///@}

  ///@{
  /**
   * Set/Get the opacity of the representation.
   *
   * @note Use `IsOpacityValid` to check if the returned value is valid/set.
   */
  static bool IsOpacityValid(double opacity);
  static void SetOpacity(vtkSMProxy* proxy, double opacity);
  static void SetBlockOpacity(vtkSMProxy* proxy, const std::string& blockSelector, double opacity)
  {
    vtkSMColorMapEditorHelper::SetBlocksOpacity(proxy, { blockSelector }, opacity);
  }
  static void SetBlocksOpacity(
    vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors, double opacity);
  static void RemoveBlockOpacity(vtkSMProxy* proxy, const std::string& blockSelector)
  {
    vtkSMColorMapEditorHelper::RemoveBlocksOpacities(proxy, { blockSelector });
  }
  static void RemoveBlocksOpacities(
    vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors);
  void SetSelectedOpacity(vtkSMProxy* proxy, double opacity);
  static double GetOpacity(vtkSMProxy* proxy);
  static double GetBlockOpacity(vtkSMProxy* proxy, const std::string& blockSelector)
  {
    return vtkSMColorMapEditorHelper::GetBlocksOpacities(proxy, { blockSelector }).front();
  }
  static std::vector<double> GetBlocksOpacities(
    vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors);
  std::vector<double> GetSelectedOpacities(vtkSMProxy* proxy);
  ///@}

  ///@{
  /**
   * Reset a block property.
   */
  static void ResetBlockProperty(
    vtkSMProxy* proxy, const std::string& blockSelector, const std::string& propertyName)
  {
    vtkSMColorMapEditorHelper::ResetBlocksProperty(proxy, { blockSelector }, propertyName);
  }
  static void RemoveBlockProperties(vtkSMProxy* proxy, const std::string& blockSelector,
    const std::vector<std::string>& propertyNames)
  {
    vtkSMColorMapEditorHelper::ResetBlocksProperties(proxy, { blockSelector }, propertyNames);
  }
  static void ResetBlocksProperty(vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors,
    const std::string& propertyName)
  {
    vtkSMColorMapEditorHelper::ResetBlocksProperties(proxy, blockSelectors, { propertyName });
  }
  static void ResetBlocksProperties(vtkSMProxy* proxy,
    const std::vector<std::string>& blockSelectors, const std::vector<std::string>& propertyNames);
  ///@}

protected:
  vtkSMColorMapEditorHelper();
  ~vtkSMColorMapEditorHelper() override;

  ///@{
  /**
   * Rescales transfer function ranges using the array information provided.
   */
  // Add proxy parameter?
  static bool RescaleTransferFunctionToDataRange(
    vtkSMProxy* proxy, vtkPVArrayInformation* info, bool extend = false, bool force = true);
  static bool RescaleBlockTransferFunctionToDataRange(vtkSMProxy* proxy,
    const std::string& blockSelector, vtkPVArrayInformation* info, bool extend = false,
    bool force = true)
  {
    return vtkSMColorMapEditorHelper::RescaleBlocksTransferFunctionToDataRange(
      proxy, { blockSelector }, { info }, extend, force)
      .front();
  }
  static std::vector<vtkTypeBool> RescaleBlocksTransferFunctionToDataRange(vtkSMProxy* proxy,
    const std::vector<std::string>& blockSelectors, std::vector<vtkPVArrayInformation*> infos,
    bool extend = false, bool force = true);
  ///@}

  ///@{
  /**
   * Set the color array name.
   */
  static void SetColorArray(vtkSMProxy* proxy, int attributeType, std::string arrayName);
  static void SetBlockColorArray(
    vtkSMProxy* proxy, const std::string& blockSelector, int attributeType, std::string arrayName)
  {
    vtkSMColorMapEditorHelper::SetBlocksColorArray(
      proxy, { blockSelector }, attributeType, arrayName);
  }
  static void SetBlocksColorArray(vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors,
    int attributeType, std::string arrayName);
  static void RemoveBlockColorArray(vtkSMProxy* proxy, const std::string& blockSelector)
  {
    vtkSMColorMapEditorHelper::RemoveBlocksColorArrays(proxy, { blockSelector });
  }
  static void RemoveBlocksColorArrays(
    vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors);
  static ColorArray GetColorArray(vtkSMProxy* proxy);
  static ColorArray GetBlockColorArray(vtkSMProxy* proxy, const std::string& blockSelector)
  {
    return vtkSMColorMapEditorHelper::GetBlocksColorArrays(proxy, { blockSelector }).front();
  }
  ///@}

  ///@{
  /**
   * Set the block lookup table proxy
   */
  static void SetBlockLookupTable(
    vtkSMProxy* proxy, const std::string& blockSelector, vtkSMProxy* lutProxy)
  {
    vtkSMColorMapEditorHelper::SetBlocksLookupTable(proxy, { blockSelector }, lutProxy);
  }
  static void SetBlocksLookupTable(
    vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors, vtkSMProxy* lutProxy);
  static void RemoveBlockLookupTable(vtkSMProxy* proxy, const std::string& blockSelector)
  {
    vtkSMColorMapEditorHelper::RemoveBlocksLookupTables(proxy, { blockSelector });
  }
  static void RemoveBlocksLookupTables(
    vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors);
  ///@}

  /**
   * Internal method to set scalar coloring, do not use directly.
   */
  static bool SetScalarColoringInternal(
    vtkSMProxy* proxy, const char* arrayName, int attributeType, bool useComponent, int component);
  static std::vector<vtkTypeBool> SetBlocksScalarColoringInternal(vtkSMProxy* proxy,
    const std::vector<std::string>& blockSelectors, const char* arrayName, int attributeType,
    bool useComponent, int component);
  std::vector<vtkTypeBool> SetSelectedScalarColoringInternal(
    vtkSMProxy* proxy, const char* arrayName, int attributeType, bool useComponent, int component);
  ///@}

  ///@{
  /**
   * Used as a memory of what was the last LUT proxy linked to this representation.
   * This is used in `UpdateScalarBarRange` to update the scalar bar range when
   * turning off the coloring for this representation.
   */
  static vtkSMProxy* GetLastLookupTable(vtkSMProxy* proxy);
  static void SetLastLookupTable(vtkSMProxy* proxy, vtkSMProxy* lutProxy);
  static vtkSMProxy* GetLastBlockLookupTable(vtkSMProxy* proxy, const std::string& blockSelector)
  {
    return vtkSMColorMapEditorHelper::GetLastBlocksLookupTables(proxy, { blockSelector }).front();
  }
  static std::vector<vtkSMProxy*> GetLastBlocksLookupTables(
    vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors);
  static void SetLastBlockLookupTable(
    vtkSMProxy* proxy, const std::string& blockSelector, vtkSMProxy* lutProxy)
  {
    vtkSMColorMapEditorHelper::SetLastBlocksLookupTable(proxy, { blockSelector }, lutProxy);
  }
  static void SetLastBlocksLookupTable(
    vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors, vtkSMProxy* lutProxy);
  ///@}

  friend class vtkSMPVRepresentationProxy;

private:
  vtkSMColorMapEditorHelper(const vtkSMColorMapEditorHelper&) = delete;
  void operator=(const vtkSMColorMapEditorHelper&) = delete;

  int SelectedPropertiesType = SelectedPropertiesTypes::Representation;
};

#endif
