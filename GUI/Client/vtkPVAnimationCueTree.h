/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAnimationCueTree.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVAnimationCueTree - represents a non-leaf node in
// the animation GUI.
// .SECTION Description
// vtkPVAnimationCueTree can be thought of as a node in the timeline tree
// which has children. This is Virtual Cue i.e. it does not have any actual
// proxies associated with it. It merely acts a container for the children 
// cues. The user cannot directly add/remove keys in the timeline
// associated with vtkPVAnimationCueTree. Its timeline automatically
// adjust to the maximum extent of the children timelines. The user
// can scale children timelines by moving the end points of its timeline.
// .SECTION See Also
// vtkPVAnimationCue

#ifndef __vtkPVAnimationCueTree_h
#define __vtkPVAnimationCueTree_h

#include "vtkPVAnimationCue.h"

class vtkCollection;
class vtkCollectionIterator;
class vtkKWFrame;
class vtkKWCanvas;

class VTK_EXPORT vtkPVAnimationCueTree : public vtkPVAnimationCue
{
public:
  static vtkPVAnimationCueTree* New();
  vtkTypeRevisionMacro(vtkPVAnimationCueTree, vtkPVAnimationCue);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void Create(vtkKWApplication* app, const char* args);

  // Description:
  // Add a child cue.
  // Currently, child can be added only after the parent has been created
  // and the child must not have been created. This method appropriately
  // set the child parent and creates and packs it.
  void AddChild(vtkPVAnimationCue* child);

  // Description:
  // Remove a child cue.
  void RemoveChild(vtkPVAnimationCue* child);

  // Description:
  // Returns the first child with the given name. Method present for
  // Trace/State. vtkPVAnimationManager ensures that these names are unique. 
  vtkPVAnimationCue* GetChild(const char* name);

  // Description:
  // Allocates a new Iterator for the chilren. The caller should delete the allocated 
  // iterator.
  vtkCollectionIterator* NewChildrenIterator();
  
  virtual void PackWidget();
  virtual void UnpackWidget();


  // Description:
  // Expand or collapse the children.
  virtual void ToggleExpandedState();
  void SetExpanded(int expand);

  // Description:
  // Set the timeline parameter bounds. This moves the timeline end points.
  // Depending upon is enable_scaling is set, the internal nodes
  // are scaled. This also updates the children bounds.
  virtual void SetTimeBounds(double bounds[2], int enable_scaling=0);


  // Description:
  // Indicates if this AnimationCueTree
  // must scale the children timeline endpoints if the user changes the endpoints
  // of this tree.
  vtkBooleanMacro(ScaleChildrenOnEndPointsChange, int);
  vtkSetMacro(ScaleChildrenOnEndPointsChange, int);
  vtkGetMacro(ScaleChildrenOnEndPointsChange, int);

  // Description:
  // Indicates if the endpoints of the timeline controlled by this AnimationCueTree
  // must be repositioned if those of the children are moved.
  vtkBooleanMacro(MoveEndPointsWhenChildrenChange, int);
  vtkSetMacro(MoveEndPointsWhenChildrenChange, int);
  vtkGetMacro(MoveEndPointsWhenChildrenChange, int);


  // Description:
  // This method removes the focus of all children cues and 
  // sets the focus of itself.
  virtual void GetFocus();

  // Description:
  // Removes the focus from itself and all the children cues.
  virtual void RemoveFocus();

  // Description:
  // Returns 1 is itself or any of the children cues has the focus.
  virtual int HasFocus();

  // Description:
  // Time marker is a vartical line used to indicate the current time.
  // This method sets the timemarker of the timeline for this cue and
  // all the children cues.
  virtual void SetTimeMarker( double time);

  // Description:
  // Initializes the PropertyStatusManager status. 
  // Calls InitializeStatus on all the children cues.
  virtual void InitializeStatus();

  // Description:
  // Calls KeyFramePropertyChanges on all the children cues.
  virtual void KeyFramePropertyChanges(double ntime, int onlyFocus);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();


  virtual void SaveState(ofstream* file);

  virtual void Zoom(double range[2]);
protected:
  vtkPVAnimationCueTree();
  ~vtkPVAnimationCueTree();

  int Expanded;
  vtkCollection* Children;

  vtkKWFrame* NavigatorContainer;
  vtkKWCanvas* NavigatorCanvas;
  vtkKWFrame* NavigatorChildrenFrame;

  vtkKWFrame* TimeLineChildrenFrame;

  virtual void ExecuteEvent(vtkObject* wdg, unsigned long event, void* calldata);

  
  void DrawChildConnections(vtkPVAnimationCue* child);

  double LastParameterBounds[2];
  vtkSetVector2Macro(LastParameterBounds, double);

  void ScaleChildren(double old_bounds[2], double new_bounds[2]);

  int ScaleChildrenOnEndPointsChange;
  int MoveEndPointsWhenChildrenChange;

  // This flag means that we are forcing the time bounds, 
  // all events triggered saying that the Keys changed are ignored.
  int ForceBounds;

  void RemoveChildrenFocus(vtkPVAnimationCue* exception = NULL);
  void AdjustEndPoints();
private:
  vtkPVAnimationCueTree(const vtkPVAnimationCueTree&); // Not implemented.
  void operator=(const vtkPVAnimationCueTree&); // Not implemented.
};


#endif

