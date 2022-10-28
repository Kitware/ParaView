/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPrismViewProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
