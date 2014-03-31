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
// .NAME vtkSMTransferFunctionProxy
// .SECTION Description
// vtkSMTransferFunctionProxy is the proxy used for "PVLookupTable",
// "ColorTransferFunction" and "PiecewiseFunction".
// It provides utility API to update lookup-table ranges, invert transfer
// function, etc. that can be used from C++ as well as Python layers.

#ifndef __vtkSMTransferFunctionProxy_h
#define __vtkSMTransferFunctionProxy_h

#include "vtkSMProxy.h"
#include "vtkPVServerManagerRenderingModule.h" // needed for export macro

class vtkPVArrayInformation;

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMTransferFunctionProxy : public vtkSMProxy
{
public:
  static vtkSMTransferFunctionProxy* New();
  vtkTypeMacro(vtkSMTransferFunctionProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Rescale the "RGBPoints" for the transfer function to match the new range.
  // Returns true if rescale was successful.
  // If \c extend is true (false by default), the transfer function range will
  // only be extended as needed to fit the data range.
  virtual bool RescaleTransferFunction(const double range[2], bool extend=false)
    { return this->RescaleTransferFunction(range[0], range[1], extend); }
  virtual bool RescaleTransferFunction(double rangeMin, double rangeMax, bool extend=false);

  // Description:
  // Safely call RescaleTransferFunction() after casting the proxy to
  // appropriate type.
  static bool RescaleTransferFunction(vtkSMProxy* proxy,
    double rangeMin, double rangeMax, bool extend=false);
  static bool RescaleTransferFunction(vtkSMProxy* proxy,
    const double range[2], bool extend=false)
    {
    return vtkSMTransferFunctionProxy::RescaleTransferFunction(
      proxy, range[0], range[1], extend);
    }

  // Description:
  // Locates all representations that are currently using this transfer function
  // and then rescales the transfer function scalar range to exactly match the
  // combined valid scalar ranges obtained from them all.
  virtual bool RescaleTransferFunctionToDataRange(bool extend=false);
  static bool RescaleTransferFunctionToDataRange(vtkSMProxy* proxy, bool extend=false)
    {
    vtkSMTransferFunctionProxy* self =
      vtkSMTransferFunctionProxy::SafeDownCast(proxy);
    return self? self->RescaleTransferFunctionToDataRange(extend) : false;
    }

  // Description:
  // Invert the transfer function. Returns true if successful.
  virtual bool InvertTransferFunction();

  // Description:
  // Safely call InvertTransferFunction() after casting the proxy to the
  // appropriate type.
  static bool InvertTransferFunction(vtkSMProxy*);

  // Description:
  // Remaps control points by normalizing in linear-space and then interpolating
  // in log-space. This is useful when converting the transfer function from
  // linear- to log-mode. If \c inverse is true, the operation is reversed i.e.
  // the control points are normalized in log-space and interpolated in
  // linear-space, useful when converting from log- to linear-mode.
  virtual bool MapControlPointsToLogSpace(bool inverse=false);
  virtual bool MapControlPointsToLinearSpace()
    { return this->MapControlPointsToLogSpace(true); }

  // Description:
  // Safely call MapControlPointsToLogSpace() after casting the proxy to the
  // appropriate type.
  static bool MapControlPointsToLogSpace(vtkSMProxy* proxy, bool inverse=false)
    {
    vtkSMTransferFunctionProxy* self =
      vtkSMTransferFunctionProxy::SafeDownCast(proxy);
    return self? self->MapControlPointsToLogSpace(inverse) : false;
    }

  // Description:
  // Safely call MapControlPointsToLinearSpace() after casting the proxy to the
  // appropriate type.
  static bool MapControlPointsToLinearSpace(vtkSMProxy* proxy)
    {
    return vtkSMTransferFunctionProxy::MapControlPointsToLogSpace(proxy, true);
    }

  // Description:
  // Load a ColorMap XML. This will update a transfer function using the
  // ColorMap XML. Currently, this is only supported for color transfer
  // functions. Returns true on success.
  virtual bool ApplyColorMap(const char* text);
  virtual bool ApplyColorMap(vtkPVXMLElement* xml);

  // Description:
  // Safely call ApplyColorMap(..) after casting the proxy to the appropriate
  // type.
  static bool ApplyColorMap(vtkSMProxy* proxy, const char* text)
    {
    vtkSMTransferFunctionProxy* self =
      vtkSMTransferFunctionProxy::SafeDownCast(proxy);
    return self? self->ApplyColorMap(text) : false;
    }

  // Description:
  // Safely call ApplyColorMap(..) after casting the proxy to the appropriate
  // type.
  static bool ApplyColorMap(vtkSMProxy* proxy, vtkPVXMLElement* xml)
    {
    vtkSMTransferFunctionProxy* self =
      vtkSMTransferFunctionProxy::SafeDownCast(proxy);
    return self? self->ApplyColorMap(xml) : false;
    }

  // Description:
  // Save to ColorMap XML. Currently, this is only supported for color transfer
  // functions. Returns true on success.
  virtual bool SaveColorMap(vtkPVXMLElement* xml);

  // Description:
  // Safely call ApplyColorMap(..) after casting the proxy to the appropriate
  // type.
  static bool SaveColorMap(vtkSMProxy* proxy, vtkPVXMLElement* xml)
    {
    vtkSMTransferFunctionProxy* self =
      vtkSMTransferFunctionProxy::SafeDownCast(proxy);
    return self? self->SaveColorMap(xml) : false;
    }

  // Description:
  // Find and return the Scalar-Bar (Color Legend) representation corresponding
  // to the transfer function for the view, if any. This returns the proxy if
  // one exists, it won't create a new one.
  virtual vtkSMProxy* FindScalarBarRepresentation(vtkSMProxy* view);

  // Description:
  // Safely call FindScalarBarRepresentation(..) after casting the proxy to the
  // appropriate type.
  static vtkSMProxy* FindScalarBarRepresentation(
    vtkSMProxy* proxy, vtkSMProxy* view)
    {
    vtkSMTransferFunctionProxy* self =
      vtkSMTransferFunctionProxy::SafeDownCast(proxy);
    return self? self->FindScalarBarRepresentation(view) : NULL;
    }

  // Description:
  // Update component titles for all scalar bars connected to this transfer
  // function proxy. The arrayInfo is used to determine component names, if
  // possible.
  virtual bool UpdateScalarBarsComponentTitle(vtkPVArrayInformation* arrayInfo);
  static bool UpdateScalarBarsComponentTitle(vtkSMProxy* proxy,
    vtkPVArrayInformation* arrayInfo)
    {
    vtkSMTransferFunctionProxy* self =
      vtkSMTransferFunctionProxy::SafeDownCast(proxy);
    return self? self->UpdateScalarBarsComponentTitle(arrayInfo) : false;
    }

//BTX
protected:
  vtkSMTransferFunctionProxy();
  ~vtkSMTransferFunctionProxy();

private:
  vtkSMTransferFunctionProxy(const vtkSMTransferFunctionProxy&); // Not implemented
  void operator=(const vtkSMTransferFunctionProxy&); // Not implemented
//ETX
};

#endif
