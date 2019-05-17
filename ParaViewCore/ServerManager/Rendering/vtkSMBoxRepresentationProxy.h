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
 * Since `vtkBoxRepresentation::SetTranform` is really akin to `ApplyTransform`,
 * the transform must be set each time the transform changes or the widget
 * is placed using PlaceWidget or something like that.
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
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void UpdateVTKObjects() override;
  void UpdatePropertyInformation() override;
  void UpdatePropertyInformation(vtkSMProperty* prop) override
  {
    this->Superclass::UpdatePropertyInformation(prop);
  }

protected:
  vtkSMBoxRepresentationProxy();
  ~vtkSMBoxRepresentationProxy() override;

  // This method is overridden to set the transform on the vtkWidgetRepresentation.
  void CreateVTKObjects() override;

private:
  vtkSMBoxRepresentationProxy(const vtkSMBoxRepresentationProxy&) = delete;
  void operator=(const vtkSMBoxRepresentationProxy&) = delete;
};

#endif
