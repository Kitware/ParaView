/*=========================================================================

  Program:   ParaView
  Module:    vtkSMTransferFunction2DProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMTransferFunction2DProxy
 * @brief vtkSMTransferFunction2DProxy is the proxy used for "TransferFunction2D".
 *
 * vtkSMTransferFunction2DProxy provides the utility API to update 2D transfer function range,
 * control boxes, etc.
 */

#ifndef vtkSMTransferFunction2DProxy_h
#define vtkSMTransferFunction2DProxy_h

// ParaView includes
#include "vtkRemotingViewsModule.h" // needed for export macro
#include "vtkSMProxy.h"

// VTK includes
#include <vtkImageData.h>    // needed for vtkImageData
#include <vtkSmartPointer.h> // for ivars

// Forward declarations

class VTKREMOTINGVIEWS_EXPORT vtkSMTransferFunction2DProxy : public vtkSMProxy
{
public:
  /**
   * Instantiate the class.
   */
  static vtkSMTransferFunction2DProxy* New();

  ///@{
  /**
   * Standard methods for the VTK class.
   */
  vtkTypeMacro(vtkSMTransferFunction2DProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Rescale the control boxes for the 2D transfer function to match the new range.
   * Returns true if rescale was successful.
   * If \c extend is true (false by default), the transfer function range will
   * only be extended as needed to fit the data range.
   */
  virtual bool RescaleTransferFunction(const double range[4], bool extend = false)
  {
    return this->RescaleTransferFunction(range[0], range[1], range[2], range[3], extend);
  }
  virtual bool RescaleTransferFunction(
    double rangeXMin, double rangeXMax, double rangeYMin, double rangeYMax, bool extend = false);

  //@{
  /**
   * Safely call RescaleTransferFunction() after casting the proxy to
   * appropriate type.
   */
  static bool RescaleTransferFunction(vtkSMProxy* proxy, double rangeXMin, double rangeXMax,
    double rangeYMin, double rangeYMax, bool extend = false);
  static bool RescaleTransferFunction(vtkSMProxy* proxy, const double range[4], bool extend = false)
  {
    return vtkSMTransferFunction2DProxy::RescaleTransferFunction(
      proxy, range[0], range[1], range[2], range[3], extend);
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
    vtkSMTransferFunction2DProxy* self = vtkSMTransferFunction2DProxy::SafeDownCast(proxy);
    return self ? self->RescaleTransferFunctionToDataRange(extend) : false;
  }
  //@}

  //@{
  /**
   * Helper method used by RescaleTransferFunctionToDataRange() to compute range
   * from all visible representations using the transfer function.
   * Returns true if a valid range was determined.
   */
  virtual bool ComputeDataRange(double range[4]);
  static bool ComputeDataRange(vtkSMProxy* proxy, double range[4])
  {
    vtkSMTransferFunction2DProxy* self = vtkSMTransferFunction2DProxy::SafeDownCast(proxy);
    return self ? self->ComputeDataRange(range) : false;
  }
  //@}

  //@(
  /**
   * Helper method used to compute a 2D histogram image with provided number of bins based on the
   * data from all the visible representations using the transfer function.
   * If successful, returns the histogram as a vtkImageData of type double.
   * If not, returns nullptr.
   */
  virtual vtkSmartPointer<vtkImageData> ComputeDataHistogram2D(int numberOfBins);
  static vtkSmartPointer<vtkImageData> ComputeDataHistogram2D(vtkSMProxy* proxy, int numberOfBins)
  {
    vtkSMTransferFunction2DProxy* self = vtkSMTransferFunction2DProxy::SafeDownCast(proxy);
    return self ? self->ComputeDataHistogram2D(numberOfBins) : nullptr;
  }
  //@}

  //@{
  /**
   * Helper method used to recover the last histogram computed by ComputeDataHistogram2D
   * Returns the histogram as a vtkImageData if available, nullptr otherwise.
   */
  virtual vtkImageData* GetHistogram2DCache() { return this->Histogram2DCache; }
  static vtkImageData* GetHistogram2DCache(vtkSMProxy* proxy)
  {
    vtkSMTransferFunction2DProxy* self = vtkSMTransferFunction2DProxy::SafeDownCast(proxy);
    return self ? self->GetHistogram2DCache() : nullptr;
  }
  //@}

  //@{
  /**
   * Returns current transfer function data range. Returns false is a valid
   * range could not be determined.
   */
  virtual bool GetRange(double range[4]);
  static bool GetRange(vtkSMProxy* proxy, double range[4])
  {
    vtkSMTransferFunction2DProxy* self = vtkSMTransferFunction2DProxy::SafeDownCast(proxy);
    return self ? self->GetRange(range) : false;
  }
  //@}

  /**
   * Expected tuple size of the boxes property of the proxy.
   * Internal use only.
   */
  static const int BOX_PROPERTY_SIZE = 8;

protected:
  vtkSMTransferFunction2DProxy() = default;
  ~vtkSMTransferFunction2DProxy() override = default;

  // Helper members
  /**
   * Cache for the 2D histogram image
   */
  vtkSmartPointer<vtkImageData> Histogram2DCache;

private:
  vtkSMTransferFunction2DProxy(const vtkSMTransferFunction2DProxy&) = delete;
  void operator=(const vtkSMTransferFunction2DProxy) = delete;
};

#endif // vtkSMTransferFunction2DProxy_h
