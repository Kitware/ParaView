/*=========================================================================

  Program:   ParaView
  Module:    vtkKWInteractor.h
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
// .NAME vtkKWInteractor
// .SECTION Description
// This is the superclass for interactors used by vtkCadInteractorPanel

#ifndef __vtkKWInteractor_h
#define __vtkKWInteractor_h

#include "vtkKWWidget.h"
#include "vtkKWRadioButton.h"
class vtkKWToolbar;
class vtkPVRenderView;

class VTK_EXPORT vtkKWInteractor : public vtkKWWidget
{
public:
  static vtkKWInteractor* New() {return new vtkKWInteractor;};
  vtkTypeMacro(vtkKWInteractor,vtkKWWidget);

  // Description:
  // This does nothing but create the widgets frame.
  virtual void Create(vtkKWApplication *app, char *args);

  // Description:
  // These methods allow the Composite to be turned on and off.
  // It is up to the subclasses to determine what that means.
  // It could be adding special actors to the renderer, changing
  // cursors, or adding and enabling UI features.
  virtual void Select();
  virtual void Deselect();

  // Description:
  // Composites may want to add actors to the renderer.
  virtual void SetRenderView(vtkPVRenderView *view);

  // Description:
  // Setting this reference causes this superclass to manage
  // the buttons state with the selected state of the composite.
  vtkSetObjectMacro(ToolbarButton, vtkKWRadioButton);
  vtkGetObjectMacro(ToolbarButton, vtkKWRadioButton);

  // Description:
  // The render view forwards these messages.
  virtual void AButtonPress(int vtkNotUsed(num), int vtkNotUsed(x), 
                            int vtkNotUsed(y)) {};
  virtual void AButtonRelease(int vtkNotUsed(num), int vtkNotUsed(x), 
                              int vtkNotUsed(y)) {};
  virtual void Button1Motion(int vtkNotUsed(x), int vtkNotUsed(y)) {};
  virtual void Button2Motion(int vtkNotUsed(x), int vtkNotUsed(y)) {};
  virtual void Button3Motion(int vtkNotUsed(x), int vtkNotUsed(y)) {};
  virtual void MotionCallback(int vtkNotUsed(x), int vtkNotUsed(y)) {};
  virtual void AKeyPress(char vtkNotUsed(key), int vtkNotUsed(x), 
                         int vtkNotUsed(y)) {};  
  
  // Description:
  // Get rid of all references.  A quick and dirty way
  // of dealing with reference loops.
  virtual void PrepareForDelete() {};

  // Description:
  // Set/Get whether the interactor has been added to the trace
  vtkSetClampMacro(TraceInitialized, int, 0, 1);
  vtkGetMacro(TraceInitialized, int);

  // Description:
  // Specify whether to trace the interactor
  vtkSetClampMacro(Tracing, int, 0, 1);
  vtkGetMacro(Tracing, int);
  vtkBooleanMacro(Tracing, int);

  // Description:
  // This method is for tracing the camera movements.
  void SetCameraState(float p0, float p1, float p2,
                      float fp0, float fp1, float fp2,
                      float up0, float up1, float up2);
  
protected:
  vtkKWInteractor();
  ~vtkKWInteractor();
  vtkKWInteractor(const vtkKWInteractor&) {};
  void operator=(const vtkKWInteractor&) {};

  int SelectedState;
  vtkPVRenderView *RenderView;

  // If set, select and deselect will set its state.
  // The button does not belong to the toolbar below.
  // The button should be used for selecting the composite.
  vtkKWRadioButton *ToolbarButton;

  // If the composite has a toolbar, the this super class
  // will manage packing the toolbar with our selection status.
  vtkKWToolbar *Toolbar;
  
  int TraceInitialized;
  int Tracing;
};


#endif
