// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkPVContextViewDataDeliveryManager
 * @brief vtkPVContextView specific vtkPVDataDeliveryManager subclass
 *
 * vtkPVContextViewDataDeliveryManager handles data movement for
 * vtkPVContextView and subclasses.
 */

#ifndef vtkPVContextViewDataDeliveryManager_h
#define vtkPVContextViewDataDeliveryManager_h

#include "vtkPVDataDeliveryManager.h"
#include "vtkRemotingViewsModule.h" //needed for exports

class VTKREMOTINGVIEWS_EXPORT vtkPVContextViewDataDeliveryManager : public vtkPVDataDeliveryManager
{
public:
  static vtkPVContextViewDataDeliveryManager* New();
  vtkTypeMacro(vtkPVContextViewDataDeliveryManager, vtkPVDataDeliveryManager);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkPVContextViewDataDeliveryManager();
  ~vtkPVContextViewDataDeliveryManager() override;

  void MoveData(vtkPVDataRepresentation* repr, bool low_res, int port) override;

private:
  vtkPVContextViewDataDeliveryManager(const vtkPVContextViewDataDeliveryManager&) = delete;
  void operator=(const vtkPVContextViewDataDeliveryManager&) = delete;
};

#endif
