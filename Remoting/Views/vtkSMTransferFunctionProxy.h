/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMTransferFunctionProxy
 *
 * vtkSMTransferFunctionProxy is the proxy used for "PVLookupTable",
 * "ColorTransferFunction" and "PiecewiseFunction".
 * It provides utility API to update lookup-table ranges, invert transfer
 * function, etc. that can be used from C++ as well as Python layers.
*/

#ifndef vtkSMTransferFunctionProxy_h
#define vtkSMTransferFunctionProxy_h

#include "vtkRemotingViewsModule.h" // needed for export macro
#include "vtkSMProxy.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

#include <vtk_jsoncpp_fwd.h> // for forward declarations

class vtkPVArrayInformation;
class VTKREMOTINGVIEWS_EXPORT vtkSMTransferFunctionProxy : public vtkSMProxy
{
public:
  static vtkSMTransferFunctionProxy* New();
  vtkTypeMacro(vtkSMTransferFunctionProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Rescale the "RGBPoints" for the transfer function to match the new range.
   * Returns true if rescale was successful.
   * If \c extend is true (false by default), the transfer function range will
   * only be extended as needed to fit the data range.
   */
  virtual bool RescaleTransferFunction(const double range[2], bool extend = false)
  {
    return this->RescaleTransferFunction(range[0], range[1], extend);
  }
  virtual bool RescaleTransferFunction(double rangeMin, double rangeMax, bool extend = false);

  //@{
  /**
   * Safely call RescaleTransferFunction() after casting the proxy to
   * appropriate type.
   */
  static bool RescaleTransferFunction(
    vtkSMProxy* proxy, double rangeMin, double rangeMax, bool extend = false);
  static bool RescaleTransferFunction(vtkSMProxy* proxy, const double range[2], bool extend = false)
  {
    return vtkSMTransferFunctionProxy::RescaleTransferFunction(proxy, range[0], range[1], extend);
  }
  //@}

  //@{
  /**
   * Locates all representations that are currently using this transfer function
   * and then rescales the transfer function scalar range to exactly match the
   * combined valid scalar ranges obtained from them all.
   */
  virtual bool RescaleTransferFunctionToDataRange(bool extend = false);
  static bool RescaleTransferFunctionToDataRange(vtkSMProxy* proxy, bool extend = false)
  {
    vtkSMTransferFunctionProxy* self = vtkSMTransferFunctionProxy::SafeDownCast(proxy);
    return self ? self->RescaleTransferFunctionToDataRange(extend) : false;
  }
  //@}

  /**
   * Invert the transfer function. Returns true if successful.
   */
  virtual bool InvertTransferFunction();

  /**
   * Safely call InvertTransferFunction() after casting the proxy to the
   * appropriate type.
   */
  static bool InvertTransferFunction(vtkSMProxy*);

  /**
   * Remaps control points by normalizing in linear-space and then interpolating
   * in log-space. This is useful when converting the transfer function from
   * linear- to log-mode. If \c inverse is true, the operation is reversed i.e.
   * the control points are normalized in log-space and interpolated in
   * linear-space, useful when converting from log- to linear-mode.
   */
  virtual bool MapControlPointsToLogSpace(bool inverse = false);
  virtual bool MapControlPointsToLinearSpace() { return this->MapControlPointsToLogSpace(true); }

  //@{
  /**
   * Safely call MapControlPointsToLogSpace() after casting the proxy to the
   * appropriate type.
   */
  static bool MapControlPointsToLogSpace(vtkSMProxy* proxy, bool inverse = false)
  {
    vtkSMTransferFunctionProxy* self = vtkSMTransferFunctionProxy::SafeDownCast(proxy);
    return self ? self->MapControlPointsToLogSpace(inverse) : false;
  }
  //@}

  /**
   * Safely call MapControlPointsToLinearSpace() after casting the proxy to the
   * appropriate type.
   */
  static bool MapControlPointsToLinearSpace(vtkSMProxy* proxy)
  {
    return vtkSMTransferFunctionProxy::MapControlPointsToLogSpace(proxy, true);
  }

  //@{
  /**
   * Apply a preset. If \c rescale is true (default), then apply loading the
   * preset, the transfer function range will be preserved (if originally
   * valid). If not valid, or \c rescale is false, then the range provided by
   * the preset is used. When using indexed color maps, the \c rescale implies
   * loading of annotations since the "range" for indexed color maps is
   * described by the annotations.
   */
  virtual bool ApplyPreset(const Json::Value& value, bool rescale = true);
  static bool ApplyPreset(vtkSMProxy* proxy, const Json::Value& value, bool rescale = true)
  {
    vtkSMTransferFunctionProxy* self = vtkSMTransferFunctionProxy::SafeDownCast(proxy);
    return self ? self->ApplyPreset(value, rescale) : false;
  }
  //@}

  virtual bool ApplyPreset(const char* presetname, bool rescale = true);
  static bool ApplyPreset(vtkSMProxy* proxy, const char* presetname, bool rescale = true)
  {
    vtkSMTransferFunctionProxy* self = vtkSMTransferFunctionProxy::SafeDownCast(proxy);
    return self ? self->ApplyPreset(presetname, rescale) : false;
  }

  //@{
  /**
   * Saves the transfer function state as a preset. This is simply a subset of the
   * state of the transfer function proxy.
   */
  virtual Json::Value GetStateAsPreset();
  static Json::Value GetStateAsPreset(vtkSMProxy* proxy);
  //@}

  //@{
  /**
   * Load a ColorMap XML. This will update a transfer function using the
   * ColorMap XML. Currently, this is only supported for color transfer
   * functions. Returns true on success.
   */
  virtual bool ApplyColorMap(const char* text);
  virtual bool ApplyColorMap(vtkPVXMLElement* xml);
  //@}

  //@{
  /**
   * Safely call ApplyColorMap(..) after casting the proxy to the appropriate
   * type.
   */
  static bool ApplyColorMap(vtkSMProxy* proxy, const char* text)
  {
    vtkSMTransferFunctionProxy* self = vtkSMTransferFunctionProxy::SafeDownCast(proxy);
    return self ? self->ApplyColorMap(text) : false;
  }
  //@}

  //@{
  /**
   * Safely call ApplyColorMap(..) after casting the proxy to the appropriate
   * type.
   */
  static bool ApplyColorMap(vtkSMProxy* proxy, vtkPVXMLElement* xml)
  {
    vtkSMTransferFunctionProxy* self = vtkSMTransferFunctionProxy::SafeDownCast(proxy);
    return self ? self->ApplyColorMap(xml) : false;
  }
  //@}

  /**
   * Save to ColorMap XML. Currently, this is only supported for color transfer
   * functions. Returns true on success.
   */
  virtual bool SaveColorMap(vtkPVXMLElement* xml);

  //@{
  /**
   * Safely call ApplyColorMap(..) after casting the proxy to the appropriate
   * type.
   */
  static bool SaveColorMap(vtkSMProxy* proxy, vtkPVXMLElement* xml)
  {
    vtkSMTransferFunctionProxy* self = vtkSMTransferFunctionProxy::SafeDownCast(proxy);
    return self ? self->SaveColorMap(xml) : false;
  }
  //@}

  /**
   * Return true if the representation corresponding to the transfer function
   * for the view is showing a Scalar-Bar (Color Legend).  Otherwise return
   * false.
   */
  virtual bool IsScalarBarVisible(vtkSMProxy* view);

  //@{
  /**
   * Safely call IsScalarBarVisible(..) after casting the proxy to the
   * appropriate type.
   */
  static bool IsScalarBarVisible(vtkSMProxy* proxy, vtkSMProxy* view)
  {
    vtkSMTransferFunctionProxy* self = vtkSMTransferFunctionProxy::SafeDownCast(proxy);
    return self ? self->IsScalarBarVisible(view) : false;
  }
  //@}

  /**
   * Find and return the Scalar-Bar (Color Legend) representation corresponding
   * to the transfer function for the view, if any. This returns the proxy if
   * one exists, it won't create a new one.
   */
  virtual vtkSMProxy* FindScalarBarRepresentation(vtkSMProxy* view);

  //@{
  /**
   * Safely call FindScalarBarRepresentation(..) after casting the proxy to the
   * appropriate type.
   */
  static vtkSMProxy* FindScalarBarRepresentation(vtkSMProxy* proxy, vtkSMProxy* view)
  {
    vtkSMTransferFunctionProxy* self = vtkSMTransferFunctionProxy::SafeDownCast(proxy);
    return self ? self->FindScalarBarRepresentation(view) : nullptr;
  }
  //@}

  //@{
  /**
   * Update component titles for all scalar bars connected to this transfer
   * function proxy. The arrayInfo is used to determine component names, if
   * possible.
   */
  virtual bool UpdateScalarBarsComponentTitle(vtkPVArrayInformation* arrayInfo);
  static bool UpdateScalarBarsComponentTitle(vtkSMProxy* proxy, vtkPVArrayInformation* arrayInfo)
  {
    vtkSMTransferFunctionProxy* self = vtkSMTransferFunctionProxy::SafeDownCast(proxy);
    return self ? self->UpdateScalarBarsComponentTitle(arrayInfo) : false;
  }
  //@}

  //@{
  /**
   * Helper method used by RescaleTransferFunctionToDataRange() to compute range
   * from all visible representations using the transfer function.
   * Returns true if a valid range was determined.
   */
  virtual bool ComputeDataRange(double range[2]);
  static bool ComputeDataRange(vtkSMProxy* proxy, double range[2])
  {
    vtkSMTransferFunctionProxy* self = vtkSMTransferFunctionProxy::SafeDownCast(proxy);
    return self ? self->ComputeDataRange(range) : false;
  }
  //@}

  //@{
  /**
   * Helper method used to compute a histogram with provided number of bins based on the data
   * from all the visible representations using the transfer function.
   * If successful, returns the histogram as a vtkTable containing two columns of double,
   * the first one being the indexes, the second one the number of values.
   * If not, returns nullptr.
   */
  virtual vtkTable* ComputeDataHistogramTable(int numberOfBins);
  static vtkTable* ComputeDataHistogramTable(vtkSMProxy* proxy, int numberOfBins)
  {
    vtkSMTransferFunctionProxy* self = vtkSMTransferFunctionProxy::SafeDownCast(proxy);
    return self ? self->ComputeDataHistogramTable(numberOfBins) : nullptr;
  }
  //@}

  //@{
  /**
   * Helper method used to recover the last histogram computed by ComputeDataHistogram
   * Returns the histogram as a vtkTable if available, nullptr otherwise.
   */
  virtual vtkTable* GetHistogramTableCache() { return this->HistogramTableCache; }
  static vtkTable* GetHistogramTableCache(vtkSMProxy* proxy)
  {
    vtkSMTransferFunctionProxy* self = vtkSMTransferFunctionProxy::SafeDownCast(proxy);
    return self ? self->GetHistogramTableCache() : nullptr;
  }
  //@}

  // Helper method to compute the active annotated values in visible
  // representations that use the transfer function.
  virtual bool ComputeAvailableAnnotations(bool extend = false);
  static bool ComputeAvailableAnnotations(vtkSMProxy* proxy, bool extend = false)
  {
    vtkSMTransferFunctionProxy* self = vtkSMTransferFunctionProxy::SafeDownCast(proxy);
    return self ? self->ComputeAvailableAnnotations(extend) : false;
  }

  //@{
  /**
   * Helper method to reset a transfer function proxy to its defaults. By
   * passing in preserve_range, you can make this method preserve the current
   * transfer function range.
   */
  virtual void ResetPropertiesToDefaults(const char* arrayName, bool preserve_range);
  static void ResetPropertiesToDefaults(
    vtkSMProxy* proxy, const char* arrayName, bool preserve_range = false)
  {
    vtkSMTransferFunctionProxy* self = vtkSMTransferFunctionProxy::SafeDownCast(proxy);
    if (self)
    {
      self->ResetPropertiesToDefaults(arrayName, preserve_range);
    }
  }
  using Superclass::ResetPropertiesToXMLDefaults;
  //@}

  /**
   * Reset the transfer function's AutomaticRescaleResetMode to the global
   * TransferFunctionRresetMode setting.
   */
  void ResetRescaleModeToGlobalSetting();

  //@{
  /**
   * Method to convert legacy color map preset XML to JSON. Use this to convert
   * a single ColorMap xml. To convert a collection of color maps, use
   * ConvertMultipleLegacyColorMapXMLToJSON().
   */
  static Json::Value ConvertLegacyColorMapXMLToJSON(vtkPVXMLElement* xml);
  static Json::Value ConvertLegacyColorMapXMLToJSON(const char* xmlcontents);
  //@}

  //@{
  /**
   * Method to convert legacy "ColorMaps" preset XML to JSON. This converts all
   * colormaps in the XML.
   */
  static Json::Value ConvertMultipleLegacyColorMapXMLToJSON(vtkPVXMLElement* xml);
  static Json::Value ConvertMultipleLegacyColorMapXMLToJSON(const char* xmlcontents);
  //@}

  /**
   * Converts legacy xml file to json.
   */
  static bool ConvertLegacyColorMapsToJSON(const char* inxmlfile, const char* outjsonfile);

  //@{
  /**
   * Returns current transfer function data range. Returns false is a valid
   * range could not be determined.
   */
  virtual bool GetRange(double range[2]);
  static bool GetRange(vtkSMProxy* proxy, double range[2])
  {
    vtkSMTransferFunctionProxy* self = vtkSMTransferFunctionProxy::SafeDownCast(proxy);
    return self ? self->GetRange(range) : false;
  }
  //@}

  /**
   * Export a transfer function to a json file. opacityTransferFunction can be nullptr but
   * colorTransferFunction must not be nullptr. The tfname will be the preset name
   * upon importing the transfer function back into ParaView.
   */
  static bool ExportTransferFunction(vtkSMTransferFunctionProxy* colorTransferFunction,
    vtkSMTransferFunctionProxy* opacityTransferFunction, const char* tfname, const char* filename);

protected:
  vtkSMTransferFunctionProxy() = default;
  ~vtkSMTransferFunctionProxy() override = default;

  /**
   * Attempt to reset transfer function to site settings. If site settings are not
   * available, then the application XML defaults are used.
   */
  void RestoreFromSiteSettingsOrXML(const char* arrayName);

  /*
   * Stores the last range used to rescale to transfer function
   * Used by ComputeDataHistogram
   */
  double LastRange[2] = { 0, 1 };

  /*
   * Cache for the histogram table
   */
  vtkSmartPointer<vtkTable> HistogramTableCache;

private:
  vtkSMTransferFunctionProxy(const vtkSMTransferFunctionProxy&) = delete;
  void operator=(const vtkSMTransferFunctionProxy&) = delete;
};

#endif
