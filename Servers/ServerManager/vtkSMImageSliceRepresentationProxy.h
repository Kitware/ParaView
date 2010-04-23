/*=========================================================================

  Program:   ParaView
  Module:    vtkSMImageSliceRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMImageSliceRepresentationProxy - representation to show images.
// .SECTION Description
// vtkSMImageSliceRepresentationProxy is a concrete representation that can be used
// to render 2D images. It also supports rendering a single Z slice of 3D
// images.

#ifndef __vtkSMImageSliceRepresentationProxy_h
#define __vtkSMImageSliceRepresentationProxy_h

#include "vtkSMPropRepresentationProxy.h"

class VTK_EXPORT vtkSMImageSliceRepresentationProxy : public vtkSMPropRepresentationProxy
{
public:
  static vtkSMImageSliceRepresentationProxy* New();
  vtkTypeMacro(vtkSMImageSliceRepresentationProxy, vtkSMPropRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the scalar coloring mode
  void SetColorAttributeType(int type);

  // Description:
  // Set the scalar color array name. If array name is 0 or "" then scalar
  // coloring is disabled.
  void SetColorArrayName(const char* name);

  // Description:
  // Get the bounds for the representation.  Returns true if successful.
  // Default implementation returns non-transformed bounds.
  // Overridden to take "UseXYPlane" property value into consideration.
  virtual bool GetBounds(double bounds[6]);

//BTX
protected:
  vtkSMImageSliceRepresentationProxy();
  ~vtkSMImageSliceRepresentationProxy();

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

  vtkSMProxy* Mapper;
  vtkSMProxy* LODMapper;
  vtkSMProxy* Prop3D;
  vtkSMProxy* Property;
  vtkSMSourceProxy* Slicer;
private:
  vtkSMImageSliceRepresentationProxy(const vtkSMImageSliceRepresentationProxy&); // Not implemented
  void operator=(const vtkSMImageSliceRepresentationProxy&); // Not implemented
//ETX
};

#endif

