/*=========================================================================

  Program:   ParaView
  Module:    vtkSMMultiSliceViewProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMMultiSliceViewProxy
 *
 * Custom RenderViewProxy to override CreateDefaultRepresentation method
 * so only the Multi-Slice representation will be available to the user
*/

#ifndef vtkSMMultiSliceViewProxy_h
#define vtkSMMultiSliceViewProxy_h

#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSMRenderViewProxy.h"

class vtkSMSourceProxy;

class VTKREMOTINGVIEWS_EXPORT vtkSMMultiSliceViewProxy : public vtkSMRenderViewProxy
{
public:
  static vtkSMMultiSliceViewProxy* New();
  vtkTypeMacro(vtkSMMultiSliceViewProxy, vtkSMRenderViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Similar to IsSelectionAvailable(), however, on failure returns the
   * error message otherwise 0.
   */
  const char* IsSelectVisiblePointsAvailable() override;

  /**
   * Overridden to set initial default slices when a representation is created.
   * Not sure that's the best way to do this, but leaving the logic unchanged in
   * this pass.
   */
  vtkSMRepresentationProxy* CreateDefaultRepresentation(vtkSMProxy* proxy, int outputPort) override;

  /**
   * Overridden to forward the call to the internal root view proxy.
   */
  const char* GetRepresentationType(vtkSMSourceProxy* producer, int outputPort) override;

  /**
   * Fetchs data bounds from the client-side object. We simply fetch the
   * client-side data bounds since vtkPVMultiSliceView ensures that they are
   * synced among all ranks in Update().
   */
  void GetDataBounds(double bounds[6]);

  /**
   * HACK: method to force representation type to "Slices".
   */
  static void ForceRepresentationType(vtkSMProxy* reprProxy, const char* type);

  /**
   * HACK: Get source's input data bounds (or BoundingBoxInModelCoordinates if
   * present).
   */
  static bool GetDataBounds(vtkSMSourceProxy* source, int opport, double bounds[6]);

protected:
  vtkSMMultiSliceViewProxy();
  ~vtkSMMultiSliceViewProxy() override;

  /**
   * Use the center of the source to initialize the view with three orthogonal
   * slices in x, y, z.
   */
  void InitDefaultSlices(vtkSMSourceProxy* source, int opport, vtkSMRepresentationProxy* repr);

private:
  vtkSMMultiSliceViewProxy(const vtkSMMultiSliceViewProxy&) = delete;
  void operator=(const vtkSMMultiSliceViewProxy&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
