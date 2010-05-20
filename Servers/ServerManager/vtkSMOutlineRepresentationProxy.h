/*=========================================================================

  Program:   ParaView
  Module:    vtkSMOutlineRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMOutlineRepresentationProxy - representation that can be used to
// show a 3D outline in a render view.
// .SECTION Description
// vtkSMOutlineRepresentationProxy is a concrete representation that can be used
// to render the outline in a vtkSMRenderViewProxy. It uses a
// vtkOutlineFilter to convert non-polydata input to polydata that can be
// rendered. 

#ifndef __vtkSMOutlineRepresentationProxy_h
#define __vtkSMOutlineRepresentationProxy_h

#include "vtkSMPropRepresentationProxy.h"

class VTK_EXPORT vtkSMOutlineRepresentationProxy : 
  public vtkSMPropRepresentationProxy
{
public:
  static vtkSMOutlineRepresentationProxy* New();
  vtkTypeMacro(vtkSMOutlineRepresentationProxy, vtkSMPropRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the scalar coloring mode
  void SetColorAttributeType(int type);

  // Description:
  // Set the scalar color array name. If array name is 0 or "" then scalar
  // coloring is disabled.
  void SetColorArrayName(const char* name);

  // Description:
  // HACK: vtkSMAnimationSceneGeometryWriter needs acces to the processed data
  // so save out. This method should return the proxy that goes in as the input
  // to strategies (eg. in case of SurfaceRepresentation, it is the geometry
  // filter).
  virtual vtkSMProxy* GetProcessedConsumer()
    { return (vtkSMProxy*)(this->OutlineFilter); }

  // Description:
  // Check if this representation has the prop by checking its vtkClientServerID
  virtual bool HasVisibleProp3D(vtkProp3D* prop);
  
  // Description:
  // Views typically support a mechanism to create a selection in the view
  // itself, e.g. by click-and-dragging over a region in the view. The view
  // passes this selection to each of the representations and asks them to
  // convert it to a proxy for a selection which can be set on the view. 
  // Its representation does not support selection creation, it should simply
  // return NULL. This method returns a new vtkSMProxy instance which the 
  // caller must free after use.
  // This implementation converts a prop selection to a selection source.
  virtual vtkSMProxy* ConvertSelection(vtkSelection* input);

  // Description:
  // Get the bounds and transform according to rotation, translation, and scaling.
  // Returns true if the bounds are "valid" (and false otherwise)
  virtual bool GetBounds(double bounds[6]);

//BTX
protected:
  vtkSMOutlineRepresentationProxy();
  ~vtkSMOutlineRepresentationProxy();

  // Description:
  // This representation needs a surface compositing strategy.
  // Overridden to request the correct type of strategy from the view.
  virtual bool InitializeStrategy(vtkSMViewProxy* view);

    // Description:
  // This method is called at the beginning of CreateVTKObjects().
  // This gives the subclasses an opportunity to set the servers flags
  // on the subproxies.
  // If this method returns false, CreateVTKObjects() is aborted.
  virtual bool BeginCreateVTKObjects();

  // Description:
  // This method is called after CreateVTKObjects(). 
  // This gives subclasses an opportunity to do some post-creation
  // initialization.
  virtual bool EndCreateVTKObjects();

  vtkSMSourceProxy* OutlineFilter;
  vtkSMProxy* Mapper;
  vtkSMProxy* Prop3D;
  vtkSMProxy* Property;

private:
  vtkSMOutlineRepresentationProxy(const vtkSMOutlineRepresentationProxy&); // Not implemented
  void operator=(const vtkSMOutlineRepresentationProxy&); // Not implemented
//ETX
};

#endif

