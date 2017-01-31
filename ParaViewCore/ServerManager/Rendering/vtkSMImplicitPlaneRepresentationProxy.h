/*=========================================================================

  Program:   ParaView
  Module:    vtkSMImplicitPlaneRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMImplicitPlaneRepresentationProxy
 * @brief   proxy for a implicit plane representation
 *
 * Specialized proxy for implicit planes. Overrides the default appearance
 * of VTK implicit plane representation.
*/

#ifndef vtkSMImplicitPlaneRepresentationProxy_h
#define vtkSMImplicitPlaneRepresentationProxy_h

#include "vtkPVServerManagerRenderingModule.h" //needed for exports
#include "vtkSMWidgetRepresentationProxy.h"

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMImplicitPlaneRepresentationProxy
  : public vtkSMWidgetRepresentationProxy
{
public:
  static vtkSMImplicitPlaneRepresentationProxy* New();
  vtkTypeMacro(vtkSMImplicitPlaneRepresentationProxy, vtkSMWidgetRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkSMImplicitPlaneRepresentationProxy();
  ~vtkSMImplicitPlaneRepresentationProxy();

  virtual void SendRepresentation() VTK_OVERRIDE;

private:
  vtkSMImplicitPlaneRepresentationProxy(
    const vtkSMImplicitPlaneRepresentationProxy&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMImplicitPlaneRepresentationProxy&) VTK_DELETE_FUNCTION;
};

#endif
