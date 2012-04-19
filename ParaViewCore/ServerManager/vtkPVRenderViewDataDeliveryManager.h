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
// .NAME vtkPVRenderViewDataDeliveryManager
// .SECTION Description
// Delivery manager for render view.

#ifndef __vtkPVRenderViewDataDeliveryManager_h
#define __vtkPVRenderViewDataDeliveryManager_h

#include "vtkPVDataDeliveryManager.h"

class vtkPVSessionBase;

class VTK_EXPORT vtkPVRenderViewDataDeliveryManager : public vtkPVDataDeliveryManager
{
public:
  static vtkPVRenderViewDataDeliveryManager* New();
  vtkTypeMacro(vtkPVRenderViewDataDeliveryManager, vtkPVDataDeliveryManager);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Overridden to access the vtkRepresentedDataStorage instances from the
  // render view as well as set up call-backs.
  virtual void SetView(vtkPVView*);

  virtual void Deliver(vtkSMDataDeliveryManagerProxy*, bool);

  void Deliver(int use_lod, unsigned int size , int* values);
//BTX
protected:
  vtkPVRenderViewDataDeliveryManager();
  ~vtkPVRenderViewDataDeliveryManager();

  virtual void OnViewUpdated();

  vtkTimeStamp GeometryDeliveryStamp;
  vtkTimeStamp LODGeometryDeliveryStamp;
  vtkTimeStamp ViewUpdateStamp;

private:
  vtkPVRenderViewDataDeliveryManager(const vtkPVRenderViewDataDeliveryManager&); // Not implemented
  void operator=(const vtkPVRenderViewDataDeliveryManager&); // Not implemented
//ETX
};

#endif
