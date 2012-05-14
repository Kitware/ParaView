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
// .NAME vtkSMDataDeliveryManager - server-manager class for
// vtkPVDataDeliveryManager.
// .SECTION Description
// vtkSMDataDeliveryManager is the server-manager wrapper for
// vtkPVDataDeliveryManager. It manages calling on methods on instances of
// vtkPVDataDeliveryManager.

#ifndef __vtkSMDataDeliveryManager_h
#define __vtkSMDataDeliveryManager_h

#include "vtkSMObject.h"
#include "vtkWeakPointer.h"
class vtkSMViewProxy;

class VTK_EXPORT vtkSMDataDeliveryManager : public vtkSMObject
{
public:
  static vtkSMDataDeliveryManager* New();
  vtkTypeMacro(vtkSMDataDeliveryManager, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the view proxy for whom we are delivering the data.
  void SetViewProxy(vtkSMViewProxy*);

  // Description:
  // Called to request delivery of the geometry.
  void Deliver(bool interactive);

  // Description:
  // EXPERIMEMTAL: Delivery when streaming is enabled.
  bool DeliverNextPiece();

//BTX
protected:
  vtkSMDataDeliveryManager();
  ~vtkSMDataDeliveryManager();

  vtkWeakPointer<vtkSMViewProxy> ViewProxy;

  vtkTimeStamp GeometryDeliveryStamp;
  vtkTimeStamp LODGeometryDeliveryStamp;

private:
  vtkSMDataDeliveryManager(const vtkSMDataDeliveryManager&); // Not implemented
  void operator=(const vtkSMDataDeliveryManager&); // Not implemented
//ETX
};

#endif
