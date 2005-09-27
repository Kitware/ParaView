/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiActorHelper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiActorHelper - server side helper object for vtkInteractorStyleTrackballMultiActor
// .SECTION Description 
// vtkMultiActorHelper transforms actors based on the user interaction.
// The user interaction are translated to appropriate property values
// by vtkInteractorStyleTrackballMultiActor and sent to the server.
// .SECTION See Also
// vtkInteractorStyleTrackballMultiActor


#ifndef __vtkMultiActorHelper_h
#define __vtkMultiActorHelper_h

#include "vtkObject.h"

class vtkActor;
class vtkActorCollection;

class VTK_EXPORT vtkMultiActorHelper : public vtkObject
{
public:
  static vtkMultiActorHelper *New();
  vtkTypeRevisionMacro(vtkMultiActorHelper,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Rotate all actors using the transform matrix.
  void Rotate(double transform[8]);

  // Description:
  // Pan all actors.
  void Pan(double x, double y);

  // Description:
  // Scale all actors.
  void UniformScale(double scaleFactor);
  
  // Description:
  // Add an actor to be transformed.
  void AddActor(vtkActor* actor);

  // Description:
  // Removes all actors from the list.
  void RemoveAllActors();

protected:
  vtkMultiActorHelper();
  ~vtkMultiActorHelper();

  vtkActorCollection* Actors;

  void Prop3DTransform(vtkActor *actor, int numRotation,
                       double *rotate, double *scale);

private:
  vtkMultiActorHelper(const vtkMultiActorHelper&);  // Not implemented.
  void operator=(const vtkMultiActorHelper&);  // Not implemented.
};

#endif
