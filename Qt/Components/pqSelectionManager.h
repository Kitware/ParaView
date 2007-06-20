/*=========================================================================

   Program: ParaView
   Module:    pqSelectionManager.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#ifndef __pqSelectionManager_h
#define __pqSelectionManager_h

#include "pqComponentsExport.h"
#include "pqServerManagerSelectionModel.h"

#include <QObject>
#include "vtkType.h"

class pqView;
class pqPipelineSource;
class pqRenderView;
class pqSelectionManagerImplementation;
class vtkDataObject;
class vtkSMClientDeliveryRepresentationProxy;
class vtkSMProxy;

/// pqSelectionManager is the nexus for selection in paraview.
/// It responds to UI events to tell the servermanager to setup for making
/// selections. It watches the servermanager's state to see if the selection
/// parameters are changed (either from the UI or from playback) and tells 
/// the servermanager to perform the requested selection.
/// It is also the link between the server manager level selection and the 
/// GUI, converting servermanager selection result datastructures into pq/Qt 
/// level selection datastructures so that all views can be synchronized and
/// show the same selection in their own manner.
class PQCOMPONENTS_EXPORT pqSelectionManager : public QObject
{
  Q_OBJECT

public:
  pqSelectionManager(QObject* parent=NULL);
  virtual ~pqSelectionManager();

  /// Returns the client side selection displayer for the source,
  /// if the source is currently selected, otherwise return 0;
  vtkSMClientDeliveryRepresentationProxy* getClientSideDisplayer(
    pqPipelineSource* source) const;

  /// Returns the currently selected source, if any.
  pqPipelineSource* getSelectedSource() const;
  
  enum Modes
  {
    INTERACT,
    SELECT, //aka suface selection
    FRUSTUM,
    IDS,
    LOCATIONS,
    THRESHOLDS
  };

  friend class vtkPQSelectionObserver;

  /// Returns a list of indices currently selected.
  /// The list is in the form (<process id>, <index>, <process id>, <index> ...).
  QList<QVariant> getSelectedIndicesWithProcessIDs() const;

  /// Returns a list of global ids currently selected.
  /// This list does not include the process Ids.
  QList<QVariant> getSelectedGlobalIDs() const;
signals:
  /// fired when the selection changes.
  void selectionChanged(pqSelectionManager*);

  /// emitted when the user has marked a selection i.e.
  /// on mouse up in selection mode.
  void selectionMarked();

  /// emitted before performing non-undoable server manager changes.
  void beginNonUndoableChanges();

  /// emitted after having performed non-undoable server
  /// manager changes.
  void endNonUndoableChanges();

public slots:
  /// Change mode to INTERACT
  void switchToInteraction();

  /// Change mode to surface selection
  void switchToSelection();

  /// Change mode to frustum selection
  void switchToSelectThrough();

  /// Clear all selections. Note that this does not clear
  /// the server manager model selection
  void clearSelection();

  /// Used to keep track of active render module
  void setActiveView(pqView*);

private slots:
  /// Called when a source is removed from the pipeline browser
  void sourceRemoved(pqPipelineSource*);

  /// Called when a view is deleted
  void viewRemoved(pqView* rm);

protected:  
  void select();

private:
  pqSelectionManagerImplementation* Implementation;
  int Mode;

  //helpers
  int setInteractorStyleToSelect(pqRenderView*);
  int setInteractorStyleToInteract(pqRenderView*);
  void processEvents(unsigned long event);
  void createSelectionDisplayer(vtkSMProxy* input);
  void selectOnSurface(int screenRectange[4]);
};
#endif

