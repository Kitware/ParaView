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
// .NAME vtkSMPVRepresentationProxy - representation for "Render View" like
// views in ParaView.
// .SECTION Description
// vtkSMPVRepresentationProxy combines surface representation and volume
// representation proxies typically used for displaying data.
// This class also takes over the selection obligations for all the internal
// representations, i.e. is disables showing of selection in all the internal
// representations, and manages it. This avoids duplicate execution of extract
// selection filter for each of the internal representations.
//
// vtkSMPVRepresentationProxy is used for pretty much all of the
// data-representations (i.e. representations showing input data) in the render
// views. It provides helper functions for controlling transfer functions,
// scalar coloring, etc.

#ifndef __vtkSMPVRepresentationProxy_h
#define __vtkSMPVRepresentationProxy_h

#include "vtkPVServerManagerRenderingModule.h" //needed for exports
#include "vtkSMRepresentationProxy.h"

class vtkPVArrayInformation;

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMPVRepresentationProxy : public vtkSMRepresentationProxy
{
public:
  static vtkSMPVRepresentationProxy* New();
  vtkTypeMacro(vtkSMPVRepresentationProxy, vtkSMRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns true if scalar coloring is enabled. This checks whether a property
  // named "ColorArrayName" exists and has a non-empty string. This does not
  // check for the validity of the array.
  virtual bool GetUsingScalarColoring();
  
  // Description:
  // Safely call GetUsingScalarColoring() after casting the proxy to appropriate
  // type.
  static bool GetUsingScalarColoring(vtkSMProxy* proxy)
    {
    vtkSMPVRepresentationProxy* self =
      vtkSMPVRepresentationProxy::SafeDownCast(proxy);
    return self? self->GetUsingScalarColoring() : false;
    }

  // Description:
  // Enable/disable scalar coloring using the specified array. This will setup a
  // color and opacity transfer functions using vtkSMTransferFunctionProxy
  // instance. If arrayname is NULL, then scalar coloring is turned off.
  // \c field_association must be one of vtkDataObject::AttributeTypes.
  virtual bool SetScalarColoring(const char* arrayname, int attribute_type);

  // Description:
  // Safely call SetScalarColoring() after casting the proxy to the appropriate
  // type.
  static bool SetScalarColoring(vtkSMProxy* proxy, const char* arrayname, int attribute_type)
    {
    vtkSMPVRepresentationProxy* self =
      vtkSMPVRepresentationProxy::SafeDownCast(proxy);
    return self? self->SetScalarColoring(arrayname, attribute_type) : false;
    }

  // Description:
  // Rescales the color transfer function and opacity transfer function using the
  // current data range. Returns true if rescale was successful.
  // If \c extend is true (false by default), the transfer function range will
  // only be extended as needed to fit the data range.
  virtual bool RescaleTransferFunctionToDataRange(bool extend=false);

  // Description:
  // Rescales the color transfer function and opacity transfer function using the
  // current data range for the chosen data-array. Returns true if rescale was
  // successful. \c field_association must be one of
  // vtkDataObject::AttributeTypes.
  // If \c extend is true (false by default), the transfer function range will
  // only be extended as needed to fit the data range.
  virtual bool RescaleTransferFunctionToDataRange(
    const char* arrayname, int attribute_type, bool extend=false);

  // Description:
  // Safely call RescaleTransferFunctionToDataRange() after casting the proxy to
  // appropriate type.
  // If \c extend is true, the transfer function range will
  // only be extended as needed to fit the data range.
  static bool RescaleTransferFunctionToDataRange(vtkSMProxy* proxy, bool extend=false)
    {
    vtkSMPVRepresentationProxy* self =
      vtkSMPVRepresentationProxy::SafeDownCast(proxy);
    return self? self->RescaleTransferFunctionToDataRange(extend) : false;
    }

  // Description:
  // Safely call RescaleTransferFunctionToDataRange() after casting the proxy to
  // appropriate type.
  static bool RescaleTransferFunctionToDataRange(vtkSMProxy* proxy,
    const char* arrayname, int attribute_type, bool extend=false)
    {
    vtkSMPVRepresentationProxy* self =
      vtkSMPVRepresentationProxy::SafeDownCast(proxy);
    return self?
      self->RescaleTransferFunctionToDataRange(arrayname, attribute_type, extend) : false;
    }

  // Description:
  // Rescales the color transfer function and opacity transfer function using the
  // current data range over time. Returns true if rescale was successful.
  virtual bool RescaleTransferFunctionToDataRangeOverTime();

  // Description:
  // Rescales the color transfer function and opacity transfer function using the
  // current data range over time for the chosen data-array. Returns true if rescale was
  // successful. \c field_association must be one of
  // vtkDataObject::AttributeTypes,
  virtual bool RescaleTransferFunctionToDataRangeOverTime(
    const char* arrayname, int attribute_type);

  // Description:
  // Safely call RescaleTransferFunctionToDataRangeOverTime() after casting the proxy to
  // appropriate type.
  static bool RescaleTransferFunctionToDataRangeOverTime(vtkSMProxy* proxy)
    {
    vtkSMPVRepresentationProxy* self =
      vtkSMPVRepresentationProxy::SafeDownCast(proxy);
    return self? self->RescaleTransferFunctionToDataRangeOverTime() : false;
    }

  // Description:
  // Safely call RescaleTransferFunctionToDataRangeOverTime() after casting the proxy to
  // appropriate type.
  static bool RescaleTransferFunctionToDataRangeOverTime(vtkSMProxy* proxy,
    const char* arrayname, int attribute_type)
    {
    vtkSMPVRepresentationProxy* self =
      vtkSMPVRepresentationProxy::SafeDownCast(proxy);
    return self?
      self->RescaleTransferFunctionToDataRangeOverTime(arrayname, attribute_type) : false;
    }

  // Description:
  // Set the scalar bar visibility. This will create a new scalar bar as needed.
  // Scalar bar is only shown if scalar coloring is indeed being used.
  virtual bool SetScalarBarVisibility(vtkSMProxy* view, bool visibile);
  static bool SetScalarBarVisibility(vtkSMProxy* proxy, vtkSMProxy* view, bool visibile)
    {
    vtkSMPVRepresentationProxy* self =
      vtkSMPVRepresentationProxy::SafeDownCast(proxy);
    return self? self->SetScalarBarVisibility(view, visibile) : false;
    }

  // Description:
  // While SetScalarBarVisibility can be used to hide a scalar bar, it will
  // always simply hide the scalar bar even if its being used by some other
  // representation. Use this method instead to only hide the scalar/color bar
  // if no other visible representation in the view is mapping data using the
  // scalar bar.
  virtual bool HideScalarBarIfNotNeeded(vtkSMProxy* view);
  static bool HideScalarBarIfNotNeeded(vtkSMProxy* repr, vtkSMProxy* view)
    {
    vtkSMPVRepresentationProxy* self =
      vtkSMPVRepresentationProxy::SafeDownCast(repr);
    return self? self->HideScalarBarIfNotNeeded(view) : false;
    }

  // Description:
  // Returns the array information for the data array used for scalar coloring,
  // if any. Otherwise returns NULL.
  virtual vtkPVArrayInformation* GetArrayInformationForColorArray();
  static vtkPVArrayInformation* GetArrayInformationForColorArray(vtkSMProxy* proxy)
    {
    vtkSMPVRepresentationProxy* self =
      vtkSMPVRepresentationProxy::SafeDownCast(proxy);
    return self? self->GetArrayInformationForColorArray() : NULL;
    }

  // Description:
  // Call vtkSMRepresentationProxy::GetProminentValuesInformation() for the
  // array used for scalar color, if any. Otherwise returns NULL.
  virtual vtkPVProminentValuesInformation* GetProminentValuesInformationForColorArray(
    double uncertaintyAllowed = 1e-6, double fraction = 1e-3);
  static vtkPVProminentValuesInformation* GetProminentValuesInformationForColorArray(
    vtkSMProxy* proxy, double uncertaintyAllowed = 1e-6, double fraction = 1e-3)
    {
    vtkSMPVRepresentationProxy* self =
      vtkSMPVRepresentationProxy::SafeDownCast(proxy);
    return self? self->GetProminentValuesInformationForColorArray(
      uncertaintyAllowed, fraction) : NULL;
    }

  // Description:
  // Overridden to ensure when picking representation types that require scalar
  // colors, scalar coloring it setup properly. Currently this is hard-coded for
  // Volume and Slice representation types.
  virtual bool SetRepresentationType(const char* type);

protected:
  vtkSMPVRepresentationProxy();
  ~vtkSMPVRepresentationProxy();

  // Description:
  // Rescales transfer function ranges using the array information provided.
  virtual bool RescaleTransferFunctionToDataRange(
    vtkPVArrayInformation* info, bool extend=false);

  // Description:
  // Overridden to ensure that the RepresentationTypesInfo and
  // Representations's domain are up-to-date.
  virtual void CreateVTKObjects();

  // Whenever the "Representation" property is modified, we ensure that the
  // this->InvalidateDataInformation() is called.
  void OnPropertyUpdated(vtkObject*, unsigned long, void* calldata);

  // Description:
  // Overridden to ensure that whenever "Input" property changes, we update the
  // "Input" properties for all internal representations (including setting up
  // of the link to the extract-selection representation).
  virtual void SetPropertyModifiedFlag(const char* name, int flag);

  // Description:
  // Overridden to process "RepresentationType" elements.
  int ReadXMLAttributes(vtkSMSessionProxyManager* pm, vtkPVXMLElement* element);

private:
  vtkSMPVRepresentationProxy(const vtkSMPVRepresentationProxy&); // Not implemented
  void operator=(const vtkSMPVRepresentationProxy&); // Not implemented

  bool InReadXMLAttributes;
  class vtkStringSet;
  vtkStringSet* RepresentationSubProxies;
//ETX
};

#endif

