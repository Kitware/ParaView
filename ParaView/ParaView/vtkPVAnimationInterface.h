/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVAnimationInterface.h
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
// .NAME vtkPVAnimationInterface - An object that controlls an animation.
// .SECTION Description
// This object will have a link to a vtkPVSource and a method.
// It will create an animation byu changing that method and rendering 
// in a loop.  vtkPVAnimtion may have a polydata timeline to display 
// to make it like other sources.  I am going to have to find a way to 
// choose the vtkPVSource to control.  It is different than the input 
// selection list I have been considering (source vs data object)?
// I imagine it will default to the current source when the animation
// object was created.  Selecting a method will be harder.  Mainly
// because the method can be for the PVSource or the VTK source.
// I am going to have to make the parameter widget list accessible.
// The object may also be able to save images or an AVI of the animation.

#ifndef __vtkPVAnimationInterface_h
#define __vtkPVAnimationInterface_h

#include "vtkPVSource.h"
#include "vtkPVRenderView.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWCheckButton.h"
#include "vtkKWText.h"
class vtkKWLabeledEntry;
class vtkKWMenuButton;
class vtkPVWindow;
class vtkKWScale;

class VTK_EXPORT vtkPVAnimationInterface : public vtkKWWidget
{
public:
  static vtkPVAnimationInterface* New();
  vtkTypeMacro(vtkPVAnimationInterface, vtkPVSource);

  // Description:
  void Create(vtkKWApplication *app, char *frameArgs);

  // Description:
  // Access to the animation parmeters
  void SetTimeStart(float t);
  vtkGetMacro(TimeStart, float);
  void SetTimeEnd(float t);
  vtkGetMacro(TimeEnd, float);
  void SetTimeStep(float t);
  vtkGetMacro(TimeStep, float);
  void SetCurrentTime(float time);
  float GetCurrentTime();

  // Decription:
  // Callback for setting the animation parameters from the
  // entries.
  void EntryCallback();

  // Description:
  // This sets the entries from the animation parameters.
  // This will be called when the user sets the animation
  // parameters programmatically.
  void EntryUpdate();

  // Description:
  // This method gets called whenever the current time changes.
  // It executes the script.
  void CurrentTimeCallback();

  // Description:
  // Callback that starts an animation.
  void Play();

  // Description:
  // Callback that stops the animation that is running.
  void Stop();

  // ------------------------------------------------

  // Description:
  // Access to the render view. Needed to  render.
  void SetView(vtkPVRenderView *renderView);
  vtkGetObjectMacro(View, vtkPVRenderView);

  // Description:
  // Access to the render view. Needed to build up the source list.
  // SetWindow can't be an object macro because that sets up a circular
  // reference.
  void SetWindow(vtkPVWindow *window);
  vtkGetObjectMacro(Window, vtkPVWindow);

  // Description:
  // The object which is being manipulated to produce the animation.
  void SetPVSource(vtkPVSource *source);
  vtkGetObjectMacro(PVSource,vtkPVSource);

  // Description:
  // A callback that sets the method to vary of the source.
  void SetMethodInterfaceIndex(int idx);

  // Description:
  // Rebuild the source menu to select the current source list.
  void UpdateSourceMenu();

  // Description:
  // Rebuild the method menu to select which method will be used.
  void UpdateMethodMenu();

  // Description:
  // Method callback to toggle between source/method and script editor.
  void ScriptCheckButtonCallback();

  // Description:
  // Access to the interface from scripts.
  void SetScriptCheckButtonState(int);
  void SetScript(const char* script);
  const char *GetScript();

  // Description:
  // Make the tcl script save the images of the animation.
  void SaveInTclScript(ofstream *file, const char *fileRoot);

protected:
  vtkPVAnimationInterface();
  ~vtkPVAnimationInterface();
  vtkPVAnimationInterface(const vtkPVAnimationInterface&) {};
  void operator=(const vtkPVAnimationInterface&) {};

  vtkPVRenderView *View;
  vtkPVWindow *Window;

  vtkKWLabeledFrame *ControlFrame;
  vtkKWWidget *ControlButtonFrame;
  vtkKWPushButton *PlayButton;
  vtkKWPushButton *StopButton;

  vtkKWScale *TimeScale;

  vtkKWWidget *TimeFrame;
  vtkKWLabeledEntry *TimeStartEntry;
  vtkKWLabeledEntry *TimeEndEntry;
  vtkKWLabeledEntry *TimeStepEntry;

  // Animation parameters
  float TimeStart;
  float TimeEnd;
  float TimeStep;
  int StopFlag;

  // New interface ------------------------------------------------

  // Menu showing all the possible sources to select.
  vtkKWLabeledFrame *ActionFrame;
  // Here to get a left justified check button.
  vtkKWWidget *ScriptCheckButtonFrame;
  vtkKWCheckButton *ScriptCheckButton;
  vtkKWText *ScriptEditor;
  vtkKWWidget *SourceMethodFrame;
  vtkKWLabel *SourceLabel;
  vtkKWMenuButton *SourceMenuButton;

  // Menu Showing all of the possible methods of the selected source.
  vtkKWLabel *MethodLabel;
  vtkKWMenuButton *MethodMenuButton;

  // The source selected.
  vtkPVSource *PVSource;
  // The method selected.
  int MethodInterfaceIndex;
  // The argument selected.
  int ArgumentIndex;

  // The formated string to evaluate.
  char *ScriptString;
};

#endif
