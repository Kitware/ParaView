/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWRenderWidget.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkKWRenderWidget
// .SECTION Description

#ifndef __vtkKWRenderWidget_h
#define __vtkKWRenderWidget_h

#define VTK_KW_INTERACTIVE_RENDER  0
#define VTK_KW_STILL_RENDER        1
#define VTK_KW_DISABLED_RENDER     2
#define VTK_KW_SINGLE_RENDER       3

#include "vtkKWWidget.h"

class vtkCamera;
class vtkKWEventMap;
class vtkKWGenericRenderWindowInteractor;
class vtkKWRenderWidgetObserver;
class vtkKWWindow;
class vtkCornerAnnotation;
class vtkProp;
class vtkRenderer;
class vtkRenderWindow;

class VTK_EXPORT vtkKWRenderWidget : public vtkKWWidget
{
public:
  vtkTypeRevisionMacro(vtkKWRenderWidget, vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Create the widget
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Set the widget's parent window.
  virtual void SetParentWindow(vtkKWWindow *window);
  vtkGetObjectMacro(ParentWindow, vtkKWWindow);

  // Description:
  // Render the scene.
  virtual void Render();

  // Description:
  // Enable/disable rendering.
  vtkGetMacro(RenderState, int);
  vtkSetClampMacro(RenderState, int, 0, 1);
  vtkBooleanMacro(RenderState, int);
  
  // Description:
  // Set/get the rendering mode
  vtkSetClampMacro(RenderMode, int, 
                   VTK_KW_INTERACTIVE_RENDER, VTK_KW_SINGLE_RENDER);
  vtkGetMacro(RenderMode, int);
  virtual void SetRenderModeToInteractive() 
    { this->RenderMode = VTK_KW_INTERACTIVE_RENDER; };
  virtual void SetRenderModeToStill() 
    { this->RenderMode = VTK_KW_STILL_RENDER; };
  virtual void SetRenderModeToSingle() 
    { this->RenderMode = VTK_KW_SINGLE_RENDER; };
  virtual void SetRenderModeToDisabled() 
    { this->RenderMode = VTK_KW_DISABLED_RENDER; };

  // Description:
  // Reset the view/widget (will usually reset the camera too).
  virtual void Reset() = 0;
  
  // Description:
  // Close the widget. 
  // Usually called when new data is about to be loaded.
  virtual void Close();
  
  // Description:
  // Setup/remove the widget bindings.
  // SetupBindings(), which sets up general bindings like Expose or Configure
  // events, will ultimately call SetupInteractionBindings() which sets up
  // interaction bindings (mouse events, keyboard events, etc.).
  virtual void SetupBindings();
  virtual void RemoveBindings();
  virtual void SetupInteractionBindings();
  virtual void RemoveInteractionBindings();
  
  // Description:
  // Manage props inside this widget renderer(s). Add, remove, query.
  virtual void AddProp(vtkProp *prop);
  virtual int  HasProp(vtkProp *prop);
  virtual void RemoveProp(vtkProp *prop);
  virtual void RemoveAllProps();
  
  // Description:
  // Set the widget background color
  virtual float* GetBackgroundColor();
  virtual void SetBackgroundColor(float r, float g, float b);
  virtual void SetBackgroundColor(float *rgb)
    { this->SetBackgroundColor(rgb[0], rgb[1], rgb[2]); };

  // Description:
  // Event handlers and useful interactions
  virtual void MouseMove(int num, int x, int y);
  virtual void AButtonPress(int num, int x, int y, int ctrl, int shift);
  virtual void AButtonRelease(int num, int x, int y);
  virtual void AKeyPress(char key, int x, int y, int ctrl, int shift);
  virtual void Exposed();
  virtual void Configure(int width, int height);
  virtual void Enter(int /*x*/, int /*y*/) {}

  // Description:
  // Get the underlying vtkCornerAnnotation.
  // Set the annotation visibility.
  // Set the corner text color.
  virtual void SetCornerAnnotationVisibility(int v);
  virtual void SetCornerTextColor(float r, float g, float b);
  virtual void SetCornerTextColor(float *rgb)
    { this->SetCornerTextColor(rgb[0], rgb[1], rgb[2]); };
  vtkGetObjectMacro(CornerAnnotation, vtkCornerAnnotation);
      
  // Description:
  // Set/Get the units that pixel sizes are measured in
  vtkSetStringMacro(Units);
  vtkGetStringMacro(Units);
  
  // Description:
  // Get the current camera
  vtkGetObjectMacro(CurrentCamera, vtkCamera);

  // Description:
  // Set/Get the scalar shift/scale
  vtkSetMacro(ScalarShift, float);
  vtkGetMacro(ScalarShift, float);
  vtkSetMacro(ScalarScale, float);
  vtkGetMacro(ScalarScale, float);

  // Description:
  // Set / Get the collapsing of renders. If this is set to true, then
  // all renders will be collapsed. Once this is set to false, if
  // there are any pending render requests. The widget will render.
  virtual void SetCollapsingRenders(int);
  vtkBooleanMacro(CollapsingRenders, int);
  vtkGetMacro(CollapsingRenders, int);

  // Description:
  // Get the event map
  vtkGetObjectMacro(EventMap, vtkKWEventMap);

  // Description:
  // Get the renderer and render window
  vtkGetObjectMacro(Renderer, vtkRenderer);
  vtkGetObjectMacro(RenderWindow, vtkRenderWindow);

  // Description:
  // Get the VTK widget
  vtkGetObjectMacro(VTKWidget, vtkKWWidget);
  
#ifdef _WIN32
  void SetupPrint(RECT &rcDest, HDC ghdc,
                  int printerPageSizeX, int printerPageSizeY,
                  int printerDPIX, int printerDPIY,
                  float scaleX, float scaleY,
                  int screenSizeX, int screenSizeY);
#endif
  vtkSetMacro(Printing, int);
  vtkGetMacro(Printing, int);
  
  virtual void SetupMemoryRendering(int width, int height, void *cd);
  virtual void ResumeScreenRendering();
  virtual void* GetMemoryDC();
  
  virtual void ExecuteEvent(vtkObject *wdg, unsigned long event, void *calldata);

  // Update the enable state. This should propagate similar calls to the
  // internal widgets.
  virtual void UpdateEnableState();

protected:
  vtkKWRenderWidget();
  ~vtkKWRenderWidget();
  
  vtkKWWidget *VTKWidget;

  vtkCornerAnnotation *CornerAnnotation;

  vtkRenderer     *Renderer;
  vtkRenderWindow *RenderWindow;
  vtkKWWindow     *ParentWindow;
  
  vtkKWGenericRenderWindowInteractor *Interactor;
  vtkKWEventMap *EventMap;
  
  int InExpose;

  int RenderState;
  int RenderMode;
  int Printing;
  
  char *Units;

  vtkCamera *CurrentCamera;

  float ScalarShift;
  float ScalarScale;

  int CollapsingRenders;
  int CollapsingRendersCount;

  vtkKWRenderWidgetObserver *Observer;
  
private:
  vtkKWRenderWidget(const vtkKWRenderWidget&);  // Not implemented
  void operator=(const vtkKWRenderWidget&);  // Not implemented
};

#endif
