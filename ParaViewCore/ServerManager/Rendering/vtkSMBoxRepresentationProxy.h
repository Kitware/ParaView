/*=========================================================================

  Program:   ParaView
  Module:    vtkSMBoxRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMBoxRepresentationProxy
 * @brief   proxy for vtkBoxRepresentation
 *
 * vtkSMBoxRepresentationProxy is a proxy for vtkBoxRepresentation. A
 * specialization is needed to set the transform on the vtkBoxRepresentation.
*/

#ifndef vtkSMBoxRepresentationProxy_h
#define vtkSMBoxRepresentationProxy_h

#include "vtkPVServerManagerRenderingModule.h" //needed for exports
#include "vtkSMWidgetRepresentationProxy.h"

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMBoxRepresentationProxy
  : public vtkSMWidgetRepresentationProxy
{
public:
  static vtkSMBoxRepresentationProxy* New();
  vtkTypeMacro(vtkSMBoxRepresentationProxy, vtkSMWidgetRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  void UpdateVTKObjects() VTK_OVERRIDE;
  void UpdatePropertyInformation() VTK_OVERRIDE;
  void UpdatePropertyInformation(vtkSMProperty* prop) VTK_OVERRIDE
  {
    this->Superclass::UpdatePropertyInformation(prop);
  }

protected:
  vtkSMBoxRepresentationProxy();
  ~vtkSMBoxRepresentationProxy() override;

  // This method is overridden to set the transform on the vtkWidgetRepresentation.
  void CreateVTKObjects() VTK_OVERRIDE;

private:
  vtkSMBoxRepresentationProxy(const vtkSMBoxRepresentationProxy&) = delete;
  void operator=(const vtkSMBoxRepresentationProxy&) = delete;
};

#endif
