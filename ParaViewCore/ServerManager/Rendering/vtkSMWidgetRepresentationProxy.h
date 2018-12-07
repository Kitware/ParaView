/*=========================================================================

  Program:   ParaView
  Module:    vtkSMWidgetRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMWidgetRepresentationProxy
 * @brief   proxy for a widget representation
 *
 * vtkSMWidgetRepresentationProxy is a specialized proxy that represents
 * VTK widget representation. It adds the capability of syncing the
 * appearance of server-side representation to the client-side
 * representation
*/

#ifndef vtkSMWidgetRepresentationProxy_h
#define vtkSMWidgetRepresentationProxy_h

#include "vtkPVServerManagerRenderingModule.h" //needed for exports
#include "vtkSMProxy.h"

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMWidgetRepresentationProxy : public vtkSMProxy
{
public:
  static vtkSMWidgetRepresentationProxy* New();
  vtkTypeMacro(vtkSMWidgetRepresentationProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * These method forward the representation state of the client side
   * widget representation to the server.
   */
  virtual void OnStartInteraction();
  virtual void OnEndInteraction();
  virtual void OnInteraction();
  //@}

protected:
  vtkSMWidgetRepresentationProxy();
  ~vtkSMWidgetRepresentationProxy() override;

  virtual void SendRepresentation();

  int RepresentationState;

private:
  vtkSMWidgetRepresentationProxy(const vtkSMWidgetRepresentationProxy&) = delete;
  void operator=(const vtkSMWidgetRepresentationProxy&) = delete;
};

#endif
