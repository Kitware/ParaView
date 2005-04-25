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
// .NAME vtkInteractorStyleTrackballMultiActor -
// .SECTION Description 

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
  virtual void Spin();
  virtual void Pan();
  virtual void Dolly();
  virtual void UniformScale();

  vtkSetMacro(UseObjectCenter, int);
  vtkGetMacro(UseObjectCenter, int);
  vtkBooleanMacro(UseObjectCenter, int);

  // Description:
  // In order to make calls on the application, we need a pointer to
  // it.
  void SetApplication(vtkPVApplication*);
  vtkGetObjectMacro(Application, vtkPVApplication);

  // Description:
  void SetHelperProxy(vtkSMProxy* HelperProxy);
  vtkGetObjectMacro(HelperProxy, vtkSMProxy);

protected:
  vtkInteractorStyleTrackballMultiActor();
  ~vtkInteractorStyleTrackballMultiActor();

  void Prop3DTransform(vtkProp3D *prop3D,
                       int NumRotation,
                       double **rotate,
                       double *scale);
  
  double MotionFactor;
  int UseObjectCenter;

  vtkPVApplication* Application;
  vtkSMProxy* HelperProxy;

private:
  vtkInteractorStyleTrackballMultiActor(const vtkInteractorStyleTrackballMultiActor&);  // Not implemented.
  void operator=(const vtkInteractorStyleTrackballMultiActor&);  // Not implemented.
};

#endif
