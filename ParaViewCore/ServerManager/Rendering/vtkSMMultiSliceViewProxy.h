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
// .NAME vtkSMMultiSliceViewProxy
// .SECTION Description
// Custom RenderViewProxy to override CreateDefaultRepresentation method
// so only the Multi-Slice representation will be available to the user

#ifndef vtkSMMultiSliceViewProxy_h
#define vtkSMMultiSliceViewProxy_h

#include "vtkPVServerManagerRenderingModule.h" //needed for exports
#include "vtkSMRenderViewProxy.h"

class vtkSMSourceProxy;

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMMultiSliceViewProxy : public vtkSMRenderViewProxy
{
public:
  static vtkSMMultiSliceViewProxy* New();
  vtkTypeMacro(vtkSMMultiSliceViewProxy, vtkSMRenderViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Similar to IsSelectionAvailable(), however, on failure returns the
  // error message otherwise 0.
  virtual const char* IsSelectVisiblePointsAvailable();

  // Description:
  // Overridden to set initial default slices when a representation is created.
  // Not sure that's the best way to do this, but leaving the logic unchanged in
  // this pass.
  virtual vtkSMRepresentationProxy* CreateDefaultRepresentation(
    vtkSMProxy* proxy, int outputPort);

  // Description:
  // Overridden to forward the call to the internal root view proxy.
  virtual const char* GetRepresentationType(
    vtkSMSourceProxy* producer, int outputPort);

  // Description:
  // Fetchs data bounds from the client-side object. We simply fetch the
  // client-side data bounds since vtkPVMultiSliceView ensures that they are
  // synced among all ranks in Update().
  void GetDataBounds(double bounds[6]);

  // Description:
  // HACK: method to force representation type to "Slices".
  static void ForceRepresentationType(vtkSMProxy* reprProxy, const char* type);

  // Description:
  // HACK: Get source's input data bounds (or BoundingBoxInModelCoordinates if
  // present).
  static bool GetDataBounds(vtkSMSourceProxy* source, int opport, double bounds[6]);
//BTX
protected:
  vtkSMMultiSliceViewProxy();
  ~vtkSMMultiSliceViewProxy();

  // Description:
  // Use the center of the source to initialize the view with three orthogonal
  // slices in x, y, z.
  void InitDefaultSlices(vtkSMSourceProxy* source, int opport,
    vtkSMRepresentationProxy* repr);

private:
  vtkSMMultiSliceViewProxy(const vtkSMMultiSliceViewProxy&); // Not implemented
  void operator=(const vtkSMMultiSliceViewProxy&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
