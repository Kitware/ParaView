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
 * @class   vtkSMDataDeliveryManagerProxy
 * @brief   server-manager class for
 * vtkPVDataDeliveryManager.
 *
 * vtkSMDataDeliveryManagerProxy is the server-manager wrapper for
 * vtkPVDataDeliveryManager. It manages calling on methods on instances of
 * vtkPVDataDeliveryManager. Before every render call, vtkSMRenderViewProxy
 * calls vtkSMDataDeliveryManagerProxy::Deliver() to ensure that any geometries that
 * need to be delivered are explicitly delivered. This separating into
 * Update-Deliver-Render calls ensures makes it possible to extend the framework
 * for streaming, in future.
 *
 * The streaming components of this class are experimental and will be changed.
 */

#ifndef vtkSMDataDeliveryManagerProxy_h
#define vtkSMDataDeliveryManagerProxy_h

#include "vtkPVServerManagerRenderingModule.h" //needed for exports
#include "vtkSMProxy.h"
#include "vtkWeakPointer.h" // needed for iVars

#include <map> // for std::map

class vtkSMViewProxy;
class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMDataDeliveryManagerProxy : public vtkSMProxy
{
public:
  static vtkSMDataDeliveryManagerProxy* New();
  vtkTypeMacro(vtkSMDataDeliveryManagerProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get/Set the view proxy for whom we are delivering the data.
   */
  void SetViewProxy(vtkSMViewProxy*);

  /**
   * Called to request delivery of the geometry. This checks the client-side
   * vtkPVDataDeliveryManager instance to see if any geometries need to be
   * delivered and then requests delivery for those.
   */
  void Deliver(bool interactive);

  /**
   * EXPERIMEMTAL: Delivery when streaming is enabled.
   * Returns true when some new data was streamed. When this returns false, it
   * implies that there is no more data to stream or streaming is not enabled.
   */
  bool DeliverStreamedPieces();

protected:
  vtkSMDataDeliveryManagerProxy();
  ~vtkSMDataDeliveryManagerProxy() override;

  vtkWeakPointer<vtkSMViewProxy> ViewProxy;
  std::map<int, vtkTimeStamp> DeliveryTimestamps;
  std::map<int, vtkTimeStamp> DeliveryTimestampsLOD;

private:
  vtkSMDataDeliveryManagerProxy(const vtkSMDataDeliveryManagerProxy&) = delete;
  void operator=(const vtkSMDataDeliveryManagerProxy&) = delete;
};

#endif
