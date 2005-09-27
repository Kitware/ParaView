/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleTrackballMultiActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInteractorStyleTrackballMultiActor - transform multiple actors
// .SECTION Description 
// vtkInteractorStyleTrackballMultiActor transforms multiple actors based
// on the user interaction. This is a paraview server/client aware
// interactor. Instead of directly calling method on the actors, it
// sets the property values of a helper proxy. The server side object
// the proxy represents (vtkMultiActorHelper) is responsible of actually
// applying the transform to the actors.

#ifndef __vtkInteractorStyleTrackballMultiActor_h
#define __vtkInteractorStyleTrackballMultiActor_h

#include "vtkInteractorStyle.h"

class vtkPVApplication;
class vtkSMProxy;

class VTK_EXPORT vtkInteractorStyleTrackballMultiActor : public vtkInteractorStyle
{
public:
  static vtkInteractorStyleTrackballMultiActor *New();
  vtkTypeRevisionMacro(vtkInteractorStyleTrackballMultiActor,vtkInteractorStyle);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void OnChar() {};

  // Description:
  // Event bindings controlling the effects of pressing mouse buttons
  // or moving the mouse.
  virtual void OnMouseMove();
  virtual void OnLeftButtonDown();
  virtual void OnLeftButtonUp();
  virtual void OnMiddleButtonDown();
  virtual void OnMiddleButtonUp();
  virtual void OnRightButtonDown();
  virtual void OnRightButtonUp();

  // These methods for the different interactions in different modes
  // are overridden in subclasses to perform the correct motion. Since
  // they might be called from OnTimer, they do not have mouse coord parameters
  // (use interactor's GetEventPosition and GetLastEventPosition)
  virtual void Rotate();
  virtual void Pan();
  virtual void UniformScale();

  // Description:
  // In order to make calls on the application, we need a pointer to
  // it.
  void SetApplication(vtkPVApplication*);
  vtkGetObjectMacro(Application, vtkPVApplication);

  // Description:
  // The helper proxy represents the server side objects
  // that is responsible of eventually transforming the actors.
  void SetHelperProxy(vtkSMProxy* HelperProxy);
  vtkGetObjectMacro(HelperProxy, vtkSMProxy);

protected:
  vtkInteractorStyleTrackballMultiActor();
  ~vtkInteractorStyleTrackballMultiActor();

  double MotionFactor;
  int UseObjectCenter;

  vtkPVApplication* Application;
  vtkSMProxy* HelperProxy;

private:
  vtkInteractorStyleTrackballMultiActor(const vtkInteractorStyleTrackballMultiActor&);  // Not implemented.
  void operator=(const vtkInteractorStyleTrackballMultiActor&);  // Not implemented.
};

#endif
