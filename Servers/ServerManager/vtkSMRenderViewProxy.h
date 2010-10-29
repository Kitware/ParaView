/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRenderViewProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMRenderViewProxy - implementation for View that includes
// render window and renderers.
// .SECTION Description
// vtkSMRenderViewProxy is a 3D view consisting for a render window and two
// renderers: 1 for 3D geometry and 1 for overlayed 2D geometry.

#ifndef __vtkSMRenderViewProxy_h
#define __vtkSMRenderViewProxy_h

#include "vtkSMViewProxy.h"

class vtkCamera;
class vtkCollection;
class vtkPVGenericRenderWindowInteractor;
class vtkRenderer;
class vtkRenderWindow;

class VTK_EXPORT vtkSMRenderViewProxy : public vtkSMViewProxy
{
public:
  static vtkSMRenderViewProxy* New();
  vtkTypeMacro(vtkSMRenderViewProxy, vtkSMViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Makes a new selection source proxy.
  bool SelectSurfaceCells(int region[4],
    vtkCollection* selectedRepresentations,
    vtkCollection* selectionSources,
    bool multiple_selections=false);
  bool SelectSurfacePoints(int region[4],
    vtkCollection* selectedRepresentations,
    vtkCollection* selectionSources,
    bool multiple_selections=false);
  bool SelectFrustumCells(int region[4],
    vtkCollection* selectedRepresentations,
    vtkCollection* selectionSources,
    bool multiple_selections=false);
  bool SelectFrustumPoints(int region[4],
    vtkCollection* selectedRepresentations,
    vtkCollection* selectionSources,
    bool multiple_selections=false);

  // Description:
  // Convenience method to pick a location. Internally uses SelectSurfaceCells
  // to locate the picked object. In future, we can make this faster.
  vtkSMRepresentationProxy* Pick(int x, int y);

  // Description:
  // Checks if color depth is sufficient to support selection.
  // If not, will return 0 and any calls to SelectVisibleCells will
  // quietly return an empty selection.
  virtual bool IsSelectionAvailable();

  // Description:
  // For backwards compatibility in Python scripts.
  void ResetCamera()
    { this->InvokeCommand("ResetCamera"); }
  void ResetCamera(double bounds[6]);

  // Description:
  // Convenience method for zooming to a representation.
  void ZoomTo(vtkSMProxy* representation);

  // Description:
  // Similar to IsSelectionAvailable(), however, on failure returns the
  // error message otherwise 0.
  virtual const char* IsSelectVisibleCellsAvailable();
  virtual const char* IsSelectVisiblePointsAvailable();

  // Description:
  // Returns the interactor.
  vtkPVGenericRenderWindowInteractor* GetInteractor();

  // Description:
  // Returns the client-side render window.
  vtkRenderWindow* GetRenderWindow();

  // Description:
  // Returns the client-side renderer (composited or 3D).
  vtkRenderer* GetRenderer();

  // Description:
  // Returns the client-side camera object.
  vtkCamera* GetActiveCamera();

  // Description:
  // Create a default representation for the given source proxy.
  // Returns a new proxy.
  virtual vtkSMRepresentationProxy* CreateDefaultRepresentation(
    vtkSMProxy*, int);

  // Description:
  // This method calls UpdateInformation on the Camera Proxy
  // and sets the Camera properties according to the info
  // properties.
  // This approach is a bit lame. We need to ensure that camera properties are
  // always/automatically synchronized. Camera properties cannot be treated same
  // way as other properties.
  void SynchronizeCameraProperties();

//BTX
protected:
  vtkSMRenderViewProxy();
  ~vtkSMRenderViewProxy();

  // Description:
  // Subclasses should override this method to do the actual image capture.
  virtual vtkImageData* CaptureWindowInternal(int magnification);

  bool SelectFrustumInternal(int region[4],
    vtkCollection* selectedRepresentations,
    vtkCollection* selectionSources,
    bool multiple_selections,
    int fieldAssociation);

  virtual void PostRender(bool interactive);

  // Description:
  // Fetches the LastSelection from the data-server and then converts it to a
  // selection source proxy and returns that.
  bool FetchLastSelection(vtkCollection* selectedRepresentations,
    vtkCollection* selectionSources);

  // Description:
  // Called at the end of CreateVTKObjects().
  virtual void CreateVTKObjects();

private:
  vtkSMRenderViewProxy(const vtkSMRenderViewProxy&); // Not implemented
  void operator=(const vtkSMRenderViewProxy&); // Not implemented
//ETX
};

#endif
