/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkCameraInteractor.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  
Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
// .NAME vtkCameraInteractor - Does all except bindings and rendering.
// .SECTION Description
// vtkCameraInteractor provides methods that can be bound to mouse events.
// The methods move the camera so the user can navigate through the world
// space.  All xy point arguments are in displaycoordinates. Different 
// modes are supported including: Fly,  Translate (Really Pan), 
// Bounding Box Zoom, and Rotate about arbitrary world point.

// .SECTION see also
// vtkPartsInteractor

#ifndef __vtkCameraInteractor_h
#define __vtkCameraInteractor_h

#include "vtkInteractor.h"
class vtkCamera;
class vtkAxes;
class vtkPolyDataMapper;
class vtkActor;


class VTK_EXPORT vtkCameraInteractor : public vtkInteractor
{
 public:
  static vtkCameraInteractor *New();
  vtkTypeMacro(vtkCameraInteractor,vtkInteractor);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Method that moves camera (position and focalPoint) foward/ or backward 
  // in direction of viee plane normal.
  void PanZ(float dist);

  // Description:
  // Moves forward/backwards and turns.  Mean to be used in loop while 
  // mouse is held down.  Velocity is reduced when turns are large.
  void Fly(int x, int y, float velocity);

  // --- Methods originally from RotateCameraIntertactor ---

  // Description:
  // Set center of rotation to a world point, and move the center actor
  // to show that point.
  void SetCenter(double x, double y, double z);
  void SetCenter(double c[3]) {this->SetCenter(c[0], c[1], c[2]);}
  vtkGetVector3Macro(Center, double);

  // Description:
  // These methods turn the center display on and off.
  void ShowCenterOn();
  void ShowCenterOff();

  // Description:
  // This method uses the visible actors to choose the size of the center actor.X
  void ResetCenterActorSize();

  void SetRenderer(vtkRenderer *ren);

  // for debugging
  vtkGetObjectMacro(CenterActor, vtkActor);

  // Description:
  // Rotation is meant to be used tied to mouse motion.  Rotation is 
  // performed around an arbitrary point: ivar "Center".
  void RotateStart(int x1, int y1);
  void RotateMotion(int x2, int y2);

  // Description:
  // Roll is meant to be used tied to mouse motion.  Rotation is 
  // performed around an arbitrary point: ivar "Center". Roll is
  // faster when mouse is near center.
  void RollStart(int x1, int y1);
  void RollMotion(int x2, int y2);
  
  // Description:
  // Hybrid rotate/roll.  If mouse starts near the center of roation, then
  // we are zooming.  Otherwise we pan.  We still need a way to 
  // change the cursor.
  void RotateRollStart(int x, int y);
  void RotateRollMotion(int x2, int y2);


  // --- Methods originally from TranslateCameraIntertactor ---

  // Description:
  // Translation tied to relative mouse motion implemented as 
  // Pan and Pitch (To avoid distace dependency).
  void PanStart(int x, int y);
  void PanMotion(int x2, int y2);

  // Description:
  // Zoom in and out scaled by clipping range.
  void ZoomStart(int x, int y);
  void ZoomMotion(int x2, int y2);

  // Description:
  // Hybrid pan/zoom.  If mouse starts in top or bottom third, then
  // we are zooming.  Otherwise we pan.  We still need a way to 
  // change the cursor.
  void PanZoomStart(int x, int y);
  void PanZoomMotion(int x2, int y2);

  // Description:
  // These methods define a bounding box from two points: start and end.
  // The end method moves forward and pans so that the display bounding
  // box fills the entire renderer.  If the bounding box is too small,
  // an arbitrary zoom factor is used.
  void BoundingBoxZoomStart(int x, int y);
  void BoundingBoxZoomEnd(int x2, int y2);
  

protected:
  vtkCameraInteractor();
  ~vtkCameraInteractor();
  vtkCameraInteractor(const vtkCameraInteractor&) {};
  void operator=(const vtkCameraInteractor&) {};

  int SaveX;
  int SaveY;
  double Center[3];

  // Ror roll: the center in display coordinates.
  float DisplayCenter[2];
  
  // When zooming, this scale is the distance per pixel mouse movement.
  float ZoomScale;

  // For displaying cross hairs at the center of rotation.
  vtkAxes *CenterSource;
  vtkPolyDataMapper *CenterMapper;
  vtkActor *CenterActor;

  // This is for interaction that mixes styles based on where the first
  // mouse press occurs.
  int HybridState;
    
  // Description:
  // Method that puts the first light in the renderer behind the camera.
  void ResetLights();

  // Description:
  // Moves cameras position, viewup, and focal point as if it were an actor.
  void ApplyTransformToCamera(vtkTransform *tform, vtkCamera *cam);

};

#endif


