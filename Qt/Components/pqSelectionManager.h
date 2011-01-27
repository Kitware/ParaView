/*=========================================================================

   Program: ParaView
   Module:    pqSelectionManager.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
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
#include <QPair>
#include "vtkType.h"

class pqOutputPort;
class pqPipelineSource;
class pqRenderView;
class pqSelectionManagerImplementation;
class pqView;
class vtkDataObject;
class vtkSelection;
class vtkSMClientDeliveryRepresentationProxy;
class vtkSMProxy;
class vtkSMSourceProxy;

/// pqSelectionManager is the nexus for introspective surface selection in 
//  paraview. 
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

  /// Returns the currently selected source, if any.
  pqOutputPort* getSelectedPort() const;
  
  friend class vtkPQSelectionObserver;

  /// Make a selection source proxy for a client-side selection.
  /// Only supports pedigree id selections.
  static vtkSMSourceProxy* createSelectionSource(vtkSelection* s, vtkIdType connId);

signals:
  /// Fired when the selection changes. Argument is the pqOutputPort (if any)
  /// that was selected. If selection was cleared then the argument is NULL.
  void selectionChanged(pqOutputPort*);

public slots:
  /// Clear all selections. Note that this does not clear
  /// the server manager model selection
  void clearSelection();

  /// Used to keep track of active render module
  void setActiveView(pqView*);

  /// Updates the selected port.
  void select(pqOutputPort*);

private slots:
  /// Called when server manager item is being deleted.
  void onItemRemoved(pqServerManagerModelItem* item);

protected:
  void onSelect(pqOutputPort*,bool forceGlobalIds);

private:
  pqSelectionManagerImplementation* Implementation;

  //helpers
  void selectOnSurface(int screenRectange[4]);
};
#endif

