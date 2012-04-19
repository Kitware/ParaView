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
// .NAME vtkSMDataDeliveryManagerProxy
// .SECTION Description
//

#ifndef __vtkSMDataDeliveryManagerProxy_h
#define __vtkSMDataDeliveryManagerProxy_h

#include "vtkSMProxy.h"

class vtkSMViewProxy;

class VTK_EXPORT vtkSMDataDeliveryManagerProxy : public vtkSMProxy
{
public:
  static vtkSMDataDeliveryManagerProxy* New();
  vtkTypeMacro(vtkSMDataDeliveryManagerProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the view proxy for whom we are delivering the data.
  void SetViewProxy(vtkSMViewProxy*);
  vtkSMViewProxy* GetViewProxy();

  void Deliver(bool interactive);
//BTX
protected:
  vtkSMDataDeliveryManagerProxy();
  ~vtkSMDataDeliveryManagerProxy();

  void OnViewUpdated();

  vtkWeakPointer<vtkSMViewProxy> ViewProxy;
  unsigned long UpdateObserverTag;
private:
  vtkSMDataDeliveryManagerProxy(const vtkSMDataDeliveryManagerProxy&); // Not implemented
  void operator=(const vtkSMDataDeliveryManagerProxy&); // Not implemented
//ETX
};

#endif
