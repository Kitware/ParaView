/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeProp.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCompositeProp - Prop used for compositing
// .SECTION Description
// vtkCompositeProp is 
//
// .SECTION See Also
//  vtkProp vtkCompositeManager

#ifndef __vtkCompositeProp_h
#define __vtkCompositeProp_h

#include "vtkProp.h"

class vtkPropCollection;

class VTK_COMMON_EXPORT vtkCompositeProp : public vtkProp
{
public:
  vtkTypeRevisionMacro(vtkCompositeProp,vtkProp);
  static vtkCompositeProp* New();
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description: 
  // For some exporters and other other operations we must be
  // able to collect all the actors or volumes. These methods
  // are used in that process.
  virtual void GetActors(vtkPropCollection *) {}
  virtual void GetActors2D(vtkPropCollection *) {}
  virtual void GetVolumes(vtkPropCollection *) {}

  // Description:
  // Method invokes PickMethod() if one defined and the prop is picked.
  virtual void Pick();

  // Get the bounds for this Prop as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
  // in world coordinates. NULL means that the bounds are not defined.
  virtual float *GetBounds();

//BTX  
  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THESE METHODS OUTSIDE OF THE RENDERING PROCESS
  // All concrete subclasses must be able to render themselves.
  // There are three key render methods in vtk and they correspond
  // to three different points in the rendering cycle. Any given
  // prop may implement one or more of these methods. 
  // The first method is intended for rendering all opaque geometry. The
  // second method is intended for rendering all translucent geometry. Most
  // volume rendering mappers draw their results during this second method.
  // The last method is to render any 2D annotation or overlays.
  // Each of these methods return an integer value indicating
  // whether or not this render method was applied to this data. 
  virtual int RenderOpaqueGeometry(      vtkViewport *);
  virtual int RenderTranslucentGeometry( vtkViewport *);
  virtual int RenderOverlay(             vtkViewport *);

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *);
//ETX

  // Description:
  // Add or remove prop from the composite prop.
  void AddProp(vtkProp *p);
  void RemoveProp(vtkProp *p);

protected:
  vtkCompositeProp();
  ~vtkCompositeProp();

  vtkPropCollection *Props;
  float Bounds[6];

private:
  vtkCompositeProp(const vtkCompositeProp&);  // Not implemented.
  void operator=(const vtkCompositeProp&);  // Not implemented.
};

#endif


