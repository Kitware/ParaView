// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMPrismViewProxy
 * @brief   implementation for View that includes render window and renderers for prism data
 *
 * vtkSMPrismViewProxy is a 3D view for prism data consisting for a render window and two
 * renderers: 1 for 3D geometry and 1 for overlaid 2D geometry.
 */

#ifndef vtkSMPrismViewProxy_h
#define vtkSMPrismViewProxy_h

#include "vtkDataObject.h"       // needed for vtkDataObject::FieldAssociation
#include "vtkPrismViewsModule.h" // needed for exports
#include "vtkSMRenderViewProxy.h"

class VTKPRISMVIEWS_EXPORT vtkSMPrismViewProxy : public vtkSMRenderViewProxy
{
public:
  static vtkSMPrismViewProxy* New();
  vtkTypeMacro(vtkSMPrismViewProxy, vtkSMRenderViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Overridden to set the view axis names.
   */
  void Update() override;

  /**
   * Overridden to check through the various representations that this view can
   * create.
   */
  const char* GetRepresentationType(vtkSMSourceProxy* producer, int outputPort) override;

  /**
   * Get the Selection Representation proxy name.
   */
  const char* GetSelectionRepresentationProxyName() override
  {
    return "PrismSelectionRepresentation";
  }

  /**
   * Function to copy selection representation properties.
   */
  void CopySelectionRepresentationProperties(
    vtkSMProxy* fromSelectionRep, vtkSMProxy* toSelectionRep) override;

protected:
  vtkSMPrismViewProxy();
  ~vtkSMPrismViewProxy() override;

private:
  vtkSMPrismViewProxy(const vtkSMPrismViewProxy&) = delete;
  void operator=(const vtkSMPrismViewProxy&) = delete;
};

#endif // vtkSMPrismViewProxy_h
