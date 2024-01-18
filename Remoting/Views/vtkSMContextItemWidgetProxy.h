// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMContextItemWidgetProxy
 * @brief   proxy for a widget representation
 *
 * vtkSMContextItemWidgetProxy is a specialized proxy that represents
 * VTK 2D widget representation. It adds the capability of syncing the
 * appearance of server-side representation to the client-side
 * context item widget.
 */

#ifndef vtkSMContextItemWidgetProxy_h
#define vtkSMContextItemWidgetProxy_h

#include "vtkSMProxy.h"
#include "vtkSMWidgetRepresentationProxy.h" //needed for exports

class VTKREMOTINGVIEWS_EXPORT vtkSMContextItemWidgetProxy : public vtkSMWidgetRepresentationProxy
{
public:
  static vtkSMContextItemWidgetProxy* New();
  vtkTypeMacro(vtkSMContextItemWidgetProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void OnStartInteraction() override { this->SendRepresentation(); }

protected:
  vtkSMContextItemWidgetProxy() = default;
  ~vtkSMContextItemWidgetProxy() override = default;

  /**
   * Overriden so it can call modified on the server-side widget on each
   * interaction. This is necessary so both views (server side and client side)
   * are being updated at the same time. Not doing that would result in a
   * softlock because client view would wait infinitely for server side
   * view.
   */
  void SendRepresentation() override;

private:
  vtkSMContextItemWidgetProxy(const vtkSMContextItemWidgetProxy&) = delete;
  void operator=(const vtkSMContextItemWidgetProxy&) = delete;
};

#endif
