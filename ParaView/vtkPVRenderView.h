/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVRenderView.h
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
// .NAME vtkPVRenderView - For using styles
// .SECTION Description
// This is a render view that is a parallel object.  It needs to be cloned
// in all the processes to work correctly.  After cloning, the parallel
// nature of the object is transparent.
// Other features:
// I am going to try to divert the events to a vtkInteractorStyle object.
// I also have put compositing into this object.  I had to create a separate
// renderwindow and renderer for the off screen compositing (Hacks).
// Eventually I need to merge these back into the main renderer and renderer
// window.


#ifndef __vtkPVRenderView_h
#define __vtkPVRenderView_h

#include "vtkKWView.h"
#include "vtkInteractorStyle.h"
#include "vtkRenderWindowInteractor.h"

class vtkPVApplication;


class VTK_EXPORT vtkPVRenderView : public vtkKWView
{
public:
  static vtkPVRenderView* New();
  vtkTypeMacro(vtkPVRenderView,vtkKWView);

  // Description:
  // Duplicate this object in the satellite processes.  Clones will
  // have the same tcl name.  Unlike other parallel objects in
  // ParaView, this one does not use Clone to set the application.
  // That is still done in the method "Create" (for superclass reasons).
  void Clone(vtkPVApplication *pvApp);
  
  // Description:
  // Create the TK widgets associated with the view.
  void Create(vtkKWApplication *app, const char *args);

  // Description:
  // This is a parallel version of the superclasses method.
  // It makes the assignment in all of the processes.
  // Pass in a vtkPVApplication, or the method will fail.
  // NOTE: This is a different way of setting the application.
  // All other parallel objects set the application in the Clone method.
  void SetApplication(vtkKWApplication *app);
  
  // Description:
  // The events will be forwarded to this style object,
  void SetInteractorStyle(vtkInteractorStyle *style);
  vtkGetObjectMacro(InteractorStyle, vtkInteractorStyle);

  // Description:
  // These are the event handlers that UIs can use or override.
  void AButtonPress(int num, int x, int y);
  void AButtonRelease(int num, int x, int y);
  void Button1Motion(int x, int y);
  void Button2Motion(int x, int y);
  void Button3Motion(int x, int y);
  void AKeyPress(char key, int x, int y);

  // Description:
  // Special binding added to this subclass.
  void MotionCallback(int x, int y);

  // Description:
  // Compute the bounding box of all the visibile props
  // Used in ResetCamera() and ResetCameraClippingRange()
  void ComputeVisiblePropBounds( float bounds[6] ); 
  
  // Description:
  // Method called by the toolbar reset camera button.
  void ResetCamera();

  // Description:
  // Reset the camera clipping range based on the bounds of the
  // visible actors. This ensures that no props are cut off
  void ResetCameraClippingRange();

  // Description:
  // This method is called in the satellite renderers to compute the bounds of
  // the actors.
  void TransmitBounds();

  // Description:
  // This method is called in the satellite renderers to render and composite
  // the results.
  void RenderHack();
  
  // Description:
  // This method is executed in all processes.
  void AddComposite(vtkKWComposite *c);

  // Description:
  // Since the superclass AddComposite creates the properties and we do not
  // want to deal with UI in the satellite processes ...
  // This is the special call made in the satellite processes.
  void AddCompositeHack(vtkKWComposite *c);

  // Description:
  // Casts to vtkPVApplication.
  vtkPVApplication *GetPVApplication();

  // Description:
  // Composites
  void Render();
  
  // Description:
  // Update all the actors.
  void Update();

  // Description:
  // Callback method bound to expose events.
  void Exposed();
  
  // Description:
  // Are we currently in interactive mode?
  int IsInteractive() { return this->Interactive; }
  
  // Description:
  // My version.
  vtkRenderer *GetRenderer();
  vtkRenderWindow *GetRenderWindow();

  void OffScreenRenderingOn();
  
protected:

  vtkPVRenderView();
  ~vtkPVRenderView();
  vtkPVRenderView(const vtkPVRenderView&) {};
  void operator=(const vtkPVRenderView&) {};

  vtkInteractorStyle *InteractorStyle;
  vtkRenderWindowInteractor *Interactor;

  int Interactive;
  
  //vtkRenderWindow *RenderWindow;
  //vtkRenderer *Renderer;
};


#endif





