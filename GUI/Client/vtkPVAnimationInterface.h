/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAnimationInterface.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

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
class vtkKWRange;
class vtkPVAnimationInterfaceEntry;
class vtkPVAnimationInterfaceObserver;
class vtkKWFrame;

class VTK_EXPORT vtkPVAnimationInterface : public vtkKWWidget
{
public:
  static vtkPVAnimationInterface* New();
  vtkTypeRevisionMacro(vtkPVAnimationInterface, vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  virtual void Create(vtkKWApplication *app, const char *frameArgs);

  // Description:
  // Access to the animation parmeters
  void SetNumberOfFrames(int t);
  vtkGetMacro(NumberOfFrames, int);

  void SetCurrentTime(int time)
    {
    this->SetCurrentTime(time, 1);
    }
  void SetCurrentTime(int time, int trace);
  int GetCurrentTime();

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
  const char *GetScript();

  // Description:
  // Make the tcl script save the images of the animation.
  void SaveInBatchScript(ofstream *file, 
                         const char *imageFileName, 
                         const char* geometryFileName,
                         const char* writerName);
  void SaveState(ofstream *file);

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
  // When script is changed manually, save it to trace file.
  void ScriptEditorCallback();

  // Description:
  // This method gets called when the loop button is pressed
  void LoopCheckButtonCallback();

  // Decription:
  // Callback for setting the animation parameters from the
  // entries.
  void NumberOfFramesEntryCallback();

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
  void SaveImages(const char* fileRoot, const char* ext,
                  int width, int height);
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
  // This method returns 1 when there is at least one valid action.
  int IsAnimationValid();

  // Description:
  // Add an empty source item
  vtkPVAnimationInterfaceEntry* AddEmptySourceItem();
  void UpdateEntries();
  void DeleteSourceItem(int item);
  void UpdateSourceMenu(int idx);

  void ShowEntryInFrame(vtkPVAnimationInterfaceEntry *entry, int idx);
  void ShowEntryInFrame(int idx);
  void ShowEntryInFrameCallback(int idx);
  void UpdateNewScript();
  void PrepareAnimationInterface(vtkPVWindow* win);

  vtkPVAnimationInterfaceEntry* GetSourceEntry(int idx);
  int GetSourceEntryIndex(vtkPVAnimationInterfaceEntry* entry);

  void PrepareForDelete();

  // Description:
  // This method is called when the source is deleted.
  void DeleteSource(vtkPVSource* src);

  // Description:
  // Return true if the animation is available.
  vtkGetMacro(ScriptAvailable, int);

  // Description:
  // For event handling.
  void ExecuteEvent(vtkObject *o, unsigned long event, void* calldata);
  
  // Description:
  // Callback for the delay scale
  void DelayScaleCallback();
  void SetDelay(int value);
  
  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Get whether animation is playing.
  vtkGetMacro(InPlay, int);

  // Description:
  // Is it in the process of saving data?
  vtkGetMacro(SavingData, int);
 
protected:
  vtkPVAnimationInterface();
  ~vtkPVAnimationInterface();

  vtkPVRenderView *View;
  vtkPVWindow *Window;

  vtkKWFrame        *TopFrame;
  vtkKWLabeledFrame *ControlFrame;
  vtkKWWidget *ControlButtonFrame;
  vtkKWPushButton *PlayButton;
  vtkKWPushButton *StopButton;
  vtkKWPushButton *GoToBeginningButton;
  vtkKWPushButton *GoToEndButton;
  vtkKWCheckButton *LoopCheckButton;

  vtkKWScale *TimeScale;

  vtkKWWidget *TimeFrame;
  vtkKWLabeledEntry *NumberOfFramesEntry;
  vtkKWRange        *TimeRange;
  vtkKWScale *AnimationDelayScale;

  void EntryCallback();

  // Animation parameters
  int NumberOfFrames;

  int StopFlag;
  int InPlay;
  int Loop;

  int GetGlobalStart();
  int GetGlobalEnd();

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

  int Dirty;

  vtkSetStringMacro(NewScriptString);
  char* NewScriptString;
  int ScriptAvailable;

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

  int UpdatingScript;
  int SavingData;

  unsigned long ErrorEventTag;
  vtkPVAnimationInterfaceObserver* Observer;

  vtkPVAnimationInterface(const vtkPVAnimationInterface&); // Not implemented
  void operator=(const vtkPVAnimationInterface&); // Not implemented
};

#endif
