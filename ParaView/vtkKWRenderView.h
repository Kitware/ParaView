/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWRenderView.h
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
// .NAME vtkKWRenderView - a view for rendering 3D props
// .SECTION Description
// A vtkKWView targeted for rendering 3D props. Includes some default
// bindings for mouse manipulation of the camera.

#ifndef __vtkKWRenderView_h
#define __vtkKWRenderView_h

#include "vtkKWView.h"
#include "vtkRenderWindow.h"
#include "vtkKWRadioButton.h"
#include "vtkKWMenu.h"
#include "vtkKWChangeColorButton.h"

class vtkKWApplication;

class VTK_EXPORT vtkKWRenderView : public vtkKWView
{
public:
  static vtkKWRenderView* New();
  vtkTypeMacro(vtkKWRenderView,vtkKWView);

  // Description:
  // Create a RenderView
  void Create(vtkKWApplication *app, char *args);

  // Description:
  // Get the render window used by this widget
  vtkGetObjectMacro(RenderWindow,vtkRenderWindow);

  // Description:
  // Render the scene.
  virtual void Render();

  // Description:
  // These are the event handlers that UIs can use or override.
  virtual void AButtonPress(int num, int x, int y) {
    this->MakeSelected(); this->StartMotion(x,y);};
  virtual void AButtonRelease(int num, int x, int y) {this->EndMotion(x,y);};
  virtual void Button1Motion(int x, int y) {this->Rotate(x,y);};
  virtual void Button2Motion(int x, int y) {this->Pan(x,y);};
  virtual void Button3Motion(int x, int y) {this->Zoom(x,y);};
  virtual void AKeyPress(char key, int x, int y);

  // Description:
  // Some useful interactions
  virtual void Exposed();
  virtual void Enter(int x, int y);
  virtual void StartMotion(int x, int y);
  virtual void EndMotion(int x, int y);
  virtual void Rotate(int x, int y);
  virtual void Pan(int x, int y);
  virtual void Zoom(int x, int y);
  virtual void Reset();
  virtual void Wireframe();
  virtual void Surface();

  // Description:
  // method that sets up some instance variables for interaction.
  // Normally this is only called internally.
  void UpdateRenderer(int x, int y);

  // Description:
  // Provide access to the underlying objects used in the rendering process.
  vtkGetObjectMacro(Renderer,vtkRenderer);
  vtkGetObjectMacro(CurrentLight,vtkLight);
  vtkGetObjectMacro(CurrentCamera,vtkCamera);

  // Description:
  // Return the RenderWindow or ImageWindow as appropriate.
  virtual vtkWindow *GetVTKWindow() {return this->RenderWindow;};
  virtual vtkViewport *GetViewport() {return this->Renderer;};
  
  // Description:
  // Methods to support off screen rendering.
  virtual void SetupMemoryRendering(int width,int height, void *cd);
  virtual void ResumeScreenRendering();
  virtual unsigned char *GetMemoryData();
  virtual void *GetMemoryDC();

  // Description:
  // Chaining method to serialize an object and its superclasses.
  virtual void SerializeSelf(ostream& os, vtkIndent indent);
  virtual void SerializeToken(istream& is,const char token[1024]);
  virtual void SerializeRevision(ostream& os, vtkIndent indent);

  // Description:
  // Callbacks used to set the print quality
  void OnPrint1();
  void OnPrint2();
  void OnPrint3();
  
  // Description:
  // Methods to indicate when this view is the selected view.
  virtual void Select(vtkKWWindow *);
  virtual void Deselect(vtkKWWindow *);

  // Description:
  // Create the properties sheet, called by ShowViewProperties.
  virtual void CreateViewProperties();

  // Description:
  // Set the background color
  void SetBackgroundColor( float r, float g, float b );

protected:
  vtkKWRenderView();
  ~vtkKWRenderView();
  vtkKWRenderView(const vtkKWRenderView&) {};
  void operator=(const vtkKWRenderView&) {};

  vtkRenderWindow  *RenderWindow;
  vtkCamera        *CurrentCamera;
  vtkLight         *CurrentLight;
  vtkRenderer      *Renderer;
  int              LightFollowCamera;
  float            DeltaAzimuth;
  float            DeltaElevation;
  int              InRender;
  int              InMotion;

  vtkKWWidget            *GeneralProperties;
  vtkKWLabeledFrame      *BackgroundFrame;
  vtkKWChangeColorButton *BackgroundColor;

  vtkKWMenu *PageMenu;
};


#endif


