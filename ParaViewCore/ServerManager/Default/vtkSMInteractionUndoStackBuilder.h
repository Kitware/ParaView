/*=========================================================================

  Program:   ParaView
  Module:    vtkSMInteractionUndoStackBuilder.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMInteractionUndoStackBuilder
 * @brief   builder server manager undo
 * sets for render view interactions and pushes them on the undo stack.
 *
 * vtkSMInteractionUndoStackBuilder specializes in interaction.
 * This class can create undo elements for only one render view
 * at a time.
*/

#ifndef vtkSMInteractionUndoStackBuilder_h
#define vtkSMInteractionUndoStackBuilder_h

#include "vtkPVServerManagerDefaultModule.h" //needed for exports
#include "vtkSMObject.h"

class vtkSMInteractionUndoStackBuilderObserver;
class vtkSMRenderViewProxy;
class vtkSMUndoStack;
class vtkUndoSet;

class VTKPVSERVERMANAGERDEFAULT_EXPORT vtkSMInteractionUndoStackBuilder : public vtkSMObject
{
public:
  static vtkSMInteractionUndoStackBuilder* New();
  vtkTypeMacro(vtkSMInteractionUndoStackBuilder, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Get/Set the render view proxy for which we are monitoring the
   * interactions.
   */
  void SetRenderView(vtkSMRenderViewProxy*);
  vtkGetObjectMacro(RenderView, vtkSMRenderViewProxy);
  //@}

  //@{
  /**
   * Get/Set the undo stack that this builder will build.
   */
  vtkGetObjectMacro(UndoStack, vtkSMUndoStack);
  virtual void SetUndoStack(vtkSMUndoStack*);
  //@}

  /**
   * Clear the undo set currently being recorded.
   */
  void Clear();

  /**
   * Called to record the state at the beginning of an interaction.
   * Usually, this method isn't called directly, since the builder
   * listens to interaction events on the interactor and calls it
   * automatically. May be used when changing the camera
   * programatically.
   */
  void StartInteraction();

  /**
   * Called to record the state at the end of an interaction and push
   * it on the stack.
   * Usually, this method isn't called directly, since the builder
   * listens to interaction events on the interactor and calls it
   * automatically. May be used when changing the camera
   * programatically.
   */
  void EndInteraction();

protected:
  vtkSMInteractionUndoStackBuilder();
  ~vtkSMInteractionUndoStackBuilder();

  vtkSMRenderViewProxy* RenderView;
  vtkSMUndoStack* UndoStack;
  vtkUndoSet* UndoSet;

  /**
   * Event handler.
   */
  void ExecuteEvent(vtkObject* caller, unsigned long event, void* data);

  void PropertyModified(const char* pname);

  friend class vtkSMInteractionUndoStackBuilderObserver;

private:
  vtkSMInteractionUndoStackBuilder(const vtkSMInteractionUndoStackBuilder&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMInteractionUndoStackBuilder&) VTK_DELETE_FUNCTION;

  vtkSMInteractionUndoStackBuilderObserver* Observer;
};

#endif
