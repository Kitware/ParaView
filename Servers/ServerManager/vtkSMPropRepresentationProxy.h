/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPropRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPropRepresentationProxy - superclass for pipeline representations 
// that can be used in an vtkSMRenderViewProxy.
// .SECTION Description
// vtkSMPropRepresentationProxy can be used as a superclass for
// representations that use vtkProp for showing the input data.
//
// vtkSMRenderViewProxy expects some support from the representation to ensure
// that surface selection works properly. This class defines that API.
// .SECTION See Also
// vtkSMPipelineRepresentationProxy

#ifndef __vtkSMPropRepresentationProxy_h
#define __vtkSMPropRepresentationProxy_h

class vtkCollection;
class vtkSelection;

#include "vtkSMPipelineRepresentationProxy.h"

class VTK_EXPORT vtkSMPropRepresentationProxy : public vtkSMPipelineRepresentationProxy
{
public:
  vtkTypeRevisionMacro(vtkSMPropRepresentationProxy, vtkSMPipelineRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Adds to the passed in collection the props that representation has which
  // are selectable. The passed in collection object must be empty.
  virtual void GetSelectableProps(vtkCollection*)=0;

  // Description:
  // Given a surface selection for this representation, this returns a new
  // vtkSelection for the selected cells/points in the input of this
  // representation.
  virtual void ConvertSurfaceSelectionToVolumeSelection(
   vtkSelection* input, vtkSelection* output) = 0;

//BTX
protected:
  vtkSMPropRepresentationProxy();
  ~vtkSMPropRepresentationProxy();

private:
  vtkSMPropRepresentationProxy(const vtkSMPropRepresentationProxy&); // Not implemented
  void operator=(const vtkSMPropRepresentationProxy&); // Not implemented
//ETX
};

#endif

