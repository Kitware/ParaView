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
// .NAME vtkSMDataDeliveryManager
// .SECTION Description
//

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

  void Deliver(bool interactive);
  bool DeliverNextPiece();

//BTX
protected:
  vtkSMDataDeliveryManager();
  ~vtkSMDataDeliveryManager();

  void OnViewUpdated();

  vtkWeakPointer<vtkSMViewProxy> ViewProxy;
  unsigned long UpdateObserverTag;

  vtkTimeStamp GeometryDeliveryStamp;
  vtkTimeStamp LODGeometryDeliveryStamp;
  vtkTimeStamp ViewUpdateStamp;

private:
  vtkSMDataDeliveryManager(const vtkSMDataDeliveryManager&); // Not implemented
  void operator=(const vtkSMDataDeliveryManager&); // Not implemented
//ETX
};

#endif
