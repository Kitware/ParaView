// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
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

#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSMWidgetRepresentationProxy.h"

class VTKREMOTINGVIEWS_EXPORT vtkSMBoxRepresentationProxy : public vtkSMWidgetRepresentationProxy
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
