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
// .NAME vtkSMWidgetRepresentationProxy - proxy for a widget representation
// .SECTION Description
// vtkSMWidgetRepresentationProxy is a specialized proxy that represents
// VTK widget representation. It adds the capability of syncing the 
// appearance of server-side representation to the client-side 
// representation

#ifndef __vtkSMWidgetRepresentationProxy_h
#define __vtkSMWidgetRepresentationProxy_h

#include "vtkSMProxy.h"

class VTK_EXPORT vtkSMWidgetRepresentationProxy : public vtkSMProxy
{
public:
  static vtkSMWidgetRepresentationProxy* New();
  vtkTypeMacro(vtkSMWidgetRepresentationProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // These method forward the representation state of the client side
  // widget representation to the server.
  virtual void OnStartInteraction();
  virtual void OnEndInteraction();
  virtual void OnInteraction();

protected:
  vtkSMWidgetRepresentationProxy();
  ~vtkSMWidgetRepresentationProxy();

  virtual void SendRepresentation();

  int RepresentationState;

private:
  vtkSMWidgetRepresentationProxy(const vtkSMWidgetRepresentationProxy&); // Not implemented
  void operator=(const vtkSMWidgetRepresentationProxy&); // Not implemented
};

#endif
