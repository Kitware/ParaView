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
// .NAME vtkMultiActorHelper -
// .SECTION Description 

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

  void Rotate(double transform[8]);
  void Pan(double x, double y);
  void UniformScale(double scaleFactor);
  
  void AddActor(vtkActor* actor);

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
