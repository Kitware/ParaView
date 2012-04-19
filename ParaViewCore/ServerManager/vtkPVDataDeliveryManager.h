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
// .NAME vtkPVDataDeliveryManager
// .SECTION Description
// vtkPVDataDeliveryManager manages data delivery for all views and
// representations in ParaView.

#ifndef __vtkPVDataDeliveryManager_h
#define __vtkPVDataDeliveryManager_h

#include "vtkObject.h"

class vtkPVView;
class vtkSMDataDeliveryManagerProxy;

class VTK_EXPORT vtkPVDataDeliveryManager : public vtkObject
{
public:
  vtkTypeMacro(vtkPVDataDeliveryManager, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the view for whom we are delivering the data.
  virtual void SetView(vtkPVView*);
  vtkGetObjectMacro(View, vtkPVView);

  virtual void Deliver(vtkSMDataDeliveryManagerProxy*, bool interactive) = 0;

//BTX
protected:
  vtkPVDataDeliveryManager();
  ~vtkPVDataDeliveryManager();

  friend class vtkSMDataDeliveryManagerProxy;

  // called to indicate the view has updated implying that data may have
  // changed.
  virtual void OnViewUpdated()=0;

  vtkPVView* View;
private:
  vtkPVDataDeliveryManager(const vtkPVDataDeliveryManager&); // Not implemented
  void operator=(const vtkPVDataDeliveryManager&); // Not implemented
//ETX
};

#endif
