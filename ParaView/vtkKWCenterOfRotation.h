/*=========================================================================

  Program:   ParaView
  Module:    vtkKWCenterOfRotation.h
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
// .NAME vtkKWCenterOfRotation
// .SECTION Description
// This widget is used for picking a point for the center of rotation.
// The point can be entered explicitly, or speicified interactively.
// The options are: 1: Use the center of the screen, 2: Pick a point, or 
// 3: Pick a part and use its center.

#ifndef __vtkKWCenterOfRotation_h
#define __vtkKWCenterOfRotation_h

#include "vtkKWInteractor.h"
#include "vtkAxes.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkCameraInteractor.h"
class vtkKWEntry;
class vtkKWCheckButton;
class vtkPVRenderView;
class vtkPVWorldPointPicker;

class VTK_EXPORT vtkKWCenterOfRotation : public vtkKWInteractor
{
public:
  static vtkKWCenterOfRotation* New();
  vtkTypeMacro(vtkKWCenterOfRotation,vtkKWInteractor);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, char *args);
  
  // Description:
  // We need to add our actor to the renderer.
  void SetRenderView(vtkPVRenderView *view);

  // Description:
  // Sets visibility of the actor that displays the center.
  void ShowCenterOn();
  void ShowCenterOff();

  // When camera changes, we need to recompute the center of rotation.
  void CameraMovedNotify();

  // Description:
  // Uses the bounds of actors to set the center actors size.
  void ResetCenterActorSize();

  // Description:
  // If the center of the screen changes, this can be called to track
  // the center of rotation.
  void ComputeScreenCenter(double center[3]);

  // Description:
  // Trying to collect the methods to update position and size.
  void Update();

  // Description:
  // This Gets called when enter is pressed in one of the Entry widgets.
  void EntryCallback();

  // Description:
  // Callback methods from buttons.
  void OpenCallback();
  void CloseCallback();
  void PickCallback();
  void ResetCallback();

  // Description:
  // The render view forwards these events to us when we are picking 
  // a new center.
  void AButtonPress(int num, int x, int y);
  void AButtonRelease(int num, int x, int y);
  void Button1Motion(int x, int y);

  // Description:
  // This widget can controll the center of an assembly, or
  // the center of rotation of a view.
  // One or the other, but not both.
  void SetCameraInteractor(vtkCameraInteractor *interactor);
  vtkGetObjectMacro(CameraInteractor, vtkCameraInteractor);

  // Description:
  // This is the KW interactor that regains control after a new
  // center of rotation is picked.
  vtkSetObjectMacro(ParentInteractor, vtkKWInteractor);
  vtkGetObjectMacro(ParentInteractor, vtkKWInteractor);


  // Description:
  // Other interactors need access to this button to add bubble help.
  vtkGetObjectMacro(ResetButton, vtkKWWidget);

  // Description:
  // Get rid of all references.  A quick and dirty way
  // of dealing with reference loops.
  virtual void PrepareForDelete();

protected: 
  vtkKWCenterOfRotation();
  ~vtkKWCenterOfRotation();
  vtkKWCenterOfRotation(const vtkKWCenterOfRotation&) {};
  void operator=(const vtkKWCenterOfRotation&) {};

  vtkCameraInteractor *CameraInteractor;
  vtkKWInteractor *ParentInteractor;

  // Button to pick
  vtkKWWidget *PickButton;
  // Button to reset the center.
  vtkKWWidget *ResetButton;
  // Button to expand the ui to include entries.
  vtkKWWidget *OpenButton;
  // Button to remove the entries.
  vtkKWWidget *CloseButton;

  // This point entry could be a widget of its own.
  vtkKWWidget *EntryFrame;
  vtkKWWidget *XLabel;
  vtkKWEntry *XEntry;
  vtkKWWidget *YLabel;
  vtkKWEntry *YEntry;
  vtkKWWidget *ZLabel;
  vtkKWEntry *ZEntry;

  // We need our own picker, because we do not want the center of part selection
  // to change any picked parts.
  vtkPVWorldPointPicker *Picker;

  // stuff to display the center
  vtkAxes *CenterSource;
  vtkPolyDataMapper *CenterMapper;
  vtkActor *CenterActor;

  // For the special condition for view interactors.
  // When Default is used, the center has to be computed every time
  // the camera translates.
  int DefaultFlag;
};


#endif


