/*=========================================================================

  Module:    vtkKWDragAndDropTargets.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWDragAndDropTargets - a set of drag and drop targets
// .SECTION Description
// This class is a container for a set of drag and drop targets.
// It provides method to add basic drag and drop callbacks between a source
// and one or more targets.

#ifndef __vtkKWDragAndDropTargets_h
#define __vtkKWDragAndDropTargets_h

#include "vtkKWObject.h"

class vtkKWWidget;
class vtkKWDragAndDropTargetsInternals;

class VTK_EXPORT vtkKWDragAndDropTargets : public vtkKWObject
{
public:
  static vtkKWDragAndDropTargets* New();
  vtkTypeRevisionMacro(vtkKWDragAndDropTargets,vtkKWObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Enable/disable Drag and Drop between all the source and targets.
  virtual void SetEnable(int);
  vtkBooleanMacro(Enable, int);
  vtkGetMacro(Enable, int);

  // Description:
  // Set/Get the Drag and Drop source widget. This is the widget that the
  // the Drag and Drop originates from.
  // Make sure you have called SetApplication(..) on the instance before.
  virtual void SetSource(vtkKWWidget*);
  vtkGetObjectMacro(Source, vtkKWWidget);

  // Description:
  // Set/Get the Drag and Drop anchor widget. This is the actual widget
  // (or part of) that the user drags and drops. It NULL, it defaults to the
  // Source widget automatically. The anchor widget can be used to specify
  // that a sub-part of the Source widget is to be dragged & dropped, not
  // the whole Source widget.
  // Make sure you have called SetApplication(..) on the instance before.
  virtual void SetAnchor(vtkKWWidget*);
  vtkGetObjectMacro(Anchor, vtkKWWidget);

  // Description:
  // Add/Query/Remove a Drag & Drop target. 
  virtual int AddTarget(vtkKWWidget *target);
  virtual int RemoveTarget(vtkKWWidget *target);
  virtual int HasTarget(vtkKWWidget *target);
  virtual int GetNumberOfTargets();

  // Description:
  // Set a Drag & Drop target callbacks/commands.
  // If the target has not been added yet, it will be added automatically.
  // The StartCommand of all targets is called when Drag & Drop starts.
  // The PerformCommand of all targets is called while Drag & Drop is 
  // performed.
  // The EndCommand of all targets that contain the drop coordinates is called
  // when Drag & Drop ends.
  // Note that the each command is passed the absolute/screen (x,y) mouse 
  // coordinates, the Source widget and the Anchor widget (which are the
  // same most of the times), i.e. the last 4 parameters are: int, int, 
  // vtkKWWidget*, vtkKWWidget*). Additionally, EndCommand is passed a 5th 
  // parameter, the target (vtkKWWidget *).
  virtual int SetTargetStartCommand(
    vtkKWWidget *target, vtkKWObject *object, const char *method);
  virtual int SetTargetPerformCommand(
    vtkKWWidget *target, vtkKWObject *object, const char *method);
  virtual int SetTargetEndCommand(
    vtkKWWidget *target, vtkKWObject *object, const char *method);

  // Description:
  // Drag and Drop callbacks
  virtual void StartCallback(int x, int y);
  virtual void PerformCallback(int x, int y);
  virtual void EndCallback(int x, int y);

  // Description:
  // Independent of any targets, the following commands can be set:
  // The StartCommand is called when Drag & Drop starts.
  // The PerformCommand is called while Drag & Drop is performed.
  // The EndCommand is called when Drag & Drop ends.
  // Note that the each command is passed the absolute/screen (x,y) mouse 
  // coordinates.
  // None of them is called is Enable is false.
  // StartCommand and PerformCommand are called *before* any target's
  // corresponding command. EndCommand is called *after* any target's
  // EndCommand.
  virtual void SetStartCommand(vtkKWObject *object, const char *method);
  virtual void SetPerformCommand(vtkKWObject *object, const char *method);
  virtual void SetEndCommand(vtkKWObject *object, const char *method);

protected:
  vtkKWDragAndDropTargets();
  ~vtkKWDragAndDropTargets();

  // Drag and Drop

  //BTX

  class TargetSlot
  {
  public:
    vtkKWWidget *Target;
    char *StartCommand;
    char *PerformCommand;
    char *EndCommand;

    void SetStartCommand(const char*);
    void SetEndCommand(const char*);
    void SetPerformCommand(const char*);

    TargetSlot();
    ~TargetSlot();
  };

  // PIMPL Encapsulation for STL containers

  vtkKWDragAndDropTargetsInternals *Internals;
  friend class vtkKWDragAndDropTargetsInternals;

  TargetSlot* GetTarget(vtkKWWidget *target);

  //ETX

  int Enable;

  vtkKWWidget *Anchor;
  vtkKWWidget *Source;

  char *StartCommand;
  char *PerformCommand;
  char *EndCommand;

  virtual void AddBindings();
  virtual void RemoveBindings();
  virtual void DeleteAllTargets();

private:
  
  vtkKWDragAndDropTargets(const vtkKWDragAndDropTargets&); // Not implemented
  void operator=(const vtkKWDragAndDropTargets&); // Not implemented
};

#endif
