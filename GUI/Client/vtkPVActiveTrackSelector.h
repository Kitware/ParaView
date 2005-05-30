/*=========================================================================

  Program:   ParaView
  Module:    vtkPVActiveTrackSelector.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVActiveTrackSelector - Widget that shows menus to select the
// active track.
// .SECTION Description


#ifndef __vtkPVActiveTrackSelector_h
#define __vtkPVActiveTrackSelector_h

#include "vtkPVTracedWidget.h"

class vtkKWLabel;
class vtkKWMenuButton;
class vtkPVActiveTrackSelectorInternals;
class vtkPVAnimationCueTree;
class vtkPVAnimationCue;
class vtkSMProxy;

class VTK_EXPORT vtkPVActiveTrackSelector : public vtkPVTracedWidget
{
public:
  static vtkPVActiveTrackSelector* New();
  vtkTypeRevisionMacro(vtkPVActiveTrackSelector, vtkPVTracedWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Creates the widget.
  virtual void Create(vtkKWApplication* app, const char*args);

  // Description:
  // Add the AnimationCue for a PVSource.
  void AddSource(vtkPVAnimationCueTree*);
  void RemoveSource(vtkPVAnimationCueTree*);

  // Description:
  // These are the callbacks for menus.
  void SelectSourceCallback(const char* key);
  void SelectPropertyCallback(int cue_index);

  // Description:
  // When ever a cue gets focus, this method should be called
  // so that the cue gets selected in the Track Selector as well.
  // Call with argument NULL when a cue is unselected.
  void SelectCue(vtkPVAnimationCue*);

  // Description:
  // Accessors to menu buttons
  vtkGetObjectMacro(SourceMenuButton, vtkKWMenuButton);
  vtkGetObjectMacro(PropertyMenuButton, vtkKWMenuButton);

  // Description:
  // Returns the currently selected cue.
  vtkGetObjectMacro(CurrentCue, vtkPVAnimationCue);

  // Description:
  // Determines whether the currently select cue gets the focus in
  // the track view. True by default/
  vtkGetMacro(FocusCurrentCue, int);
  vtkSetMacro(FocusCurrentCue, int);

  // Description:
  // If PackHorizontally, the sub-widgets will be packed horizontally,
  // instead of being gridded vertically. This is false by default.
  // Call before Create().
  vtkSetMacro(PackHorizontally, int);
  vtkGetMacro(PackHorizontally, int);

  // Description:
  // (Shallow) copy all the source cues from the source widget
  void ShallowCopy(vtkPVActiveTrackSelector* source);

protected:
  vtkPVActiveTrackSelector();
  ~vtkPVActiveTrackSelector();
  void SelectSourceCallbackInternal(const char*key);
  void SelectPropertyCallbackInternal(int cue_index);

  void BuildPropertiesMenu(const char* pretext, vtkPVAnimationCueTree* cueTree);
  void CleanupPropertiesMenu();
  void CleanupSource();

  vtkPVAnimationCueTree* CurrentSourceCueTree;
  vtkPVAnimationCue* CurrentCue;
  vtkKWLabel* SourceLabel;
  vtkKWMenuButton* SourceMenuButton;

  vtkKWLabel* PropertyLabel;
  vtkKWMenuButton* PropertyMenuButton;
 
  vtkPVActiveTrackSelectorInternals* Internals;

  int PackHorizontally;
  int FocusCurrentCue;

private:
  vtkPVActiveTrackSelector(const vtkPVActiveTrackSelector&); // Not implemented.
  void operator=(const vtkPVActiveTrackSelector&); // Not implemented.
};

#endif
