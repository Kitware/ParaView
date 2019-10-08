/*=========================================================================

  Program:   ParaView
  Module:    vtkPVContextViewDataDeliveryManager.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPVContextViewDataDeliveryManager
 * @brief vtkPVContextView specific vtkPVDataDeliveryManager subclass
 *
 * vtkPVContextViewDataDeliveryManager handles data movement for
 * vtkPVContextView and subclasses.
 */

#ifndef vtkPVContextViewDataDeliveryManager_h
#define vtkPVContextViewDataDeliveryManager_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVDataDeliveryManager.h"

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVContextViewDataDeliveryManager
  : public vtkPVDataDeliveryManager
{
public:
  static vtkPVContextViewDataDeliveryManager* New();
  vtkTypeMacro(vtkPVContextViewDataDeliveryManager, vtkPVDataDeliveryManager);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkPVContextViewDataDeliveryManager();
  ~vtkPVContextViewDataDeliveryManager();

  void MoveData(vtkPVDataRepresentation* repr, bool low_res, int port) override;

private:
  vtkPVContextViewDataDeliveryManager(const vtkPVContextViewDataDeliveryManager&) = delete;
  void operator=(const vtkPVContextViewDataDeliveryManager&) = delete;
};

#endif
