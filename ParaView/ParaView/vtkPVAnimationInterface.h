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

#include "vtkKWWidget.h"

class vtkKWCheckButton;
class vtkKWLabel;
class vtkKWLabeledEntry;
class vtkKWLabeledFrame;
class vtkKWMenuButton;
class vtkKWPushButton;
class vtkKWScale;
class vtkKWText;
class vtkPVRenderView;
class vtkPVSource;
class vtkPVWindow;
class vtkPVWidget;
class vtkPVApplication;
class vtkCollection;
class vtkCollectionIterator;

//BTX
class vtkPVAnimationInterfaceEntry;
//ETX

class VTK_EXPORT vtkPVAnimationInterface : public vtkKWWidget
{
public:
  static vtkPVAnimationInterface* New();
  vtkTypeRevisionMacro(vtkPVAnimationInterface, vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  virtual void Create(vtkKWApplication *app, char *frameArgs);

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

  void SetTimeStart(int idx, float t);
  void SetTimeEnd(int idx, float t);
  void SetTimeStep(int idx, float t);

  // Description:
  // Callback that starts an animation.
  void Play();

  // Description:
  // Callback that stops the animation that is running.
  void Stop();

  // Description:
  // Callback that enable/disable the animation loop.
  void SetLoop(int v);
  vtkGetMacro(Loop, int);
  vtkBooleanMacro(Loop, int);

  // Description:
  // Callback that sets the current time to the beginning of the time range.
  void GoToBeginning();

  // Description:
  // Callback that sets the current time to the end of the time range.
  void GoToEnd();

  // ------------------------------------------------

  // Description:
  // Access to the render view. Needed to  render.
  virtual void SetView(vtkPVRenderView *renderView);
  vtkGetObjectMacro(View, vtkPVRenderView);

  // Description:
  // Access to the render view. Needed to build up the source list.
  // SetWindow can't be an object macro because that sets up a circular
  // reference.
  virtual void SetWindow(vtkPVWindow *window);
  vtkGetObjectMacro(Window, vtkPVWindow);

  // Description:
  // Access to the interface from scripts.
  void SetScriptCheckButtonState(int);
  void SetLabelAndScript(const char *label, const char* script);
  const char *GetScript();

  // Description:
  // Make the tcl script save the images of the animation.
  void SaveInBatchScript(ofstream *file, 
                         const char *imageFileName, 
                         const char* geometryFileName);

  // Description:
  // If the animation is controlling a specific PVWidget, then
  // this widget will be updated to reflect the new value.
  // The menu sets it here.
  virtual void SetControlledWidget(vtkPVWidget*);
  vtkGetObjectMacro(ControlledWidget, vtkPVWidget);

  // ------------------------------------------------

  // Description:
  // Method callback to toggle between source/method and script editor.
  void ScriptCheckButtonCallback();

  // Description:
  // This method gets called when the loop button is pressed
  void LoopCheckButtonCallback();

  // Decription:
  // Callback for setting the animation parameters from the
  // entries.
  void TimeStartEntryCallback();
  void TimeEndEntryCallback();
  void TimeStepEntryCallback();

  // Description:
  // This sets the entries from the animation parameters.
  // This will be called when the user sets the animation
  // parameters programmatically.
  void UpdateInterface();

  // Description:
  // This method gets called whenever the current time changes.
  // It executes the script.
  void TimeScaleCallback();

  // Description:
  // Save images or geometry for replay.
  void SaveImagesCallback();
  void SaveImages(const char* fileRoot, const char* ext);
  void SaveGeometryCallback();
  void SaveGeometry(const char* fileRoot, int numParititons);

  // Description:
  // Convenience method.
  vtkPVApplication* GetPVApplication();

  // Description:
  // Cache geoemtry callback and control.
  void CacheGeometryCheckCallback();
  int GetCacheGeometry();
  void SetCacheGeometry(int flag);

  // Description:
  // Add an empty source item
  void AddEmptySourceItem();
  void UpdateEntries();
  void DeleteSourceItem(int item);
  void UpdateSourceMenu(int idx);
  void UpdateMethodMenu(int idx);
  void SetPVSource(vtkPVSource *source, int idx);

  void SetLastEntryIndex(int);
  void ShowEntryInFrame(int idx);
  void UpdateNewScript();
  void SetTypeToInt();
  void PrepareAnimationInterface(vtkPVWindow* win);

//BTX
  vtkPVAnimationInterfaceEntry* GetSourceEntry(int idx);
//ETX

protected:
  vtkPVAnimationInterface();
  ~vtkPVAnimationInterface();

  vtkPVRenderView *View;
  vtkPVWindow *Window;

  vtkKWLabeledFrame *ControlFrame;
  vtkKWWidget *ControlButtonFrame;
  vtkKWPushButton *PlayButton;
  vtkKWPushButton *StopButton;
  vtkKWPushButton *GoToBeginningButton;
  vtkKWPushButton *GoToEndButton;
  vtkKWCheckButton *LoopCheckButton;

  vtkKWScale *TimeScale;

  vtkKWWidget *TimeFrame;
  vtkKWLabeledEntry *TimeStartEntry;
  vtkKWLabeledEntry *TimeEndEntry;
  vtkKWLabeledEntry *TimeStepEntry;

  void EntryCallback();

  // Animation parameters
  float TimeStart;
  float TimeEnd;
  float TimeStep;

  float MinTime;
  float MinStep;
  float MaxTime;

  int StopFlag;
  int InPlay;
  int Loop;

  // New interface ------------------------------------------------

  // Menu showing all the possible sources to select.
  vtkKWLabeledFrame *ActionFrame;
  // Here to get a left justified check button.
  vtkKWWidget *ScriptCheckButtonFrame;
  vtkKWCheckButton *ScriptCheckButton;
  vtkKWText *ScriptEditor;

  // The source selected.
  vtkPVSource *PVSource;
  vtkPVWidget *ControlledWidget;

  // The formated string to evaluate.
  char *ScriptString;

  vtkSetStringMacro(NewScriptString);
  char* NewScriptString;

  // Should be a better way (menu?)
  vtkKWLabeledFrame* SaveFrame;
  vtkKWWidget*       SaveButtonFrame;
  vtkKWPushButton*   SaveImagesButton;
  vtkKWPushButton*   SaveGeometryButton;
  vtkKWCheckButton*  CacheGeometryCheck;

  // Even newer interface ------------------------------------------
  // Collection of animation entries
  vtkCollection* AnimationEntries;
  vtkCollectionIterator* AnimationEntriesIterator;
  vtkKWLabeledFrame* AnimationEntriesFrame;
  vtkKWPushButton* AddItemButton;
  vtkKWPushButton* DeleteItemButton;
  vtkKWFrame* AnimationEntryInformation;
  vtkKWMenuButton* AnimationEntriesMenu;
  int InShowEntryInFrame;

  // Description:
  // Empty the entry frame
  void EmptyEntryFrame();

  int LastEntryIndex;

  vtkPVAnimationInterface(const vtkPVAnimationInterface&); // Not implemented
  void operator=(const vtkPVAnimationInterface&); // Not implemented
};

#endif
