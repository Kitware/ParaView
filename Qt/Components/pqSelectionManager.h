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

class pqGenericViewModule;
class pqPipelineSource;
class pqRenderViewModule;
class pqSelectionManagerImplementation;
class vtkDataObject;
class vtkObject;
class vtkSelection;
class vtkSMDisplayProxy;
class vtkSMProxy;
class vtkSMSelectionProxy;
class vtkSMViewModuleProxy;

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

  /// Returns the number of client-side data objects that were created by
  /// the selection.
  unsigned int getNumberOfSelectedObjects();

  /// Given an index, returns a client-side data objects that was created
  /// by the selection and the corresponding proxy. The return value is
  /// 1 on success, 0 on failure.
  int getSelectedObject(unsigned int idx, 
                        vtkSMProxy*& proxy, 
                        vtkDataObject*& dataObject);

  void getSelectedObjects(QList<pqPipelineSource*> &sourceProxies, 
    QList<vtkDataObject*> &dataObjects);

  enum Modes
  {
    INTERACT,
    SELECT, //aka suface selection
    FRUSTUM,
    IDS,
    POINTS,
    THRESHOLDS
  };

  friend class vtkPQSelectionObserver;

  /// Change mode to select listed cell ids
  void setIds(int numvals, vtkIdType *vals);

  /// Change mode to select cells with that contain the given points
  void setPoints(int numvals, double *vals);

  /// Change mode to select data with attributes within the given thresholds
  void setThresholds(int numvals, double *vals);

signals:
  // fired when the selection changes.
  void selectionChanged(pqSelectionManager*);

  // emitted when the user has marked a selection i.e.
  // on mouse up in selection mode.
  void selectionMarked();

  // emitted when the user changes to a 3d view and declares whether
  // it is possible to do color buffer selections in that view or not
  void selectionAvailable(bool);
  
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
  void setActiveView(pqGenericViewModule*);

  /// cleans all internal proxies.
  void cleanSelections();

private slots:
  //checks if any server state relating to selection is out of data and
  //if so tells the server to execute and gets the results into the client
  //in other words it checks proxies then calls
  //vtkSMSelectionProxy::UpdateSelection() and then calls
  //pqSelectionManager::selectionChanged()
  void updateSelections();

  void sourceRemoved(pqPipelineSource*);
  void proxyRegistered(QString, QString, vtkSMProxy*);
  void proxyUnRegistered(QString, QString, vtkSMProxy*);
  void viewModuleRemoved(pqGenericViewModule* rm);
  void onSelectionUpdateVTKObjects(vtkObject* sel);

private:
  pqSelectionManagerImplementation* Implementation;
  int Mode;

  //pushes the state for a requested selection onto the servermanager
  //later, when updateSelections will make it happen
  void prepareForSelection();

  // takes the results of a selection, converts it into a Qt friendly 
  // format to share amongst views, creates the highlighted outline and
  // fires a selectionChanged() signal.
  void selectionChanged(vtkIdType cid);

  //helpers
  int setInteractorStyleToSelect(pqRenderViewModule*);
  int setInteractorStyleToInteract(pqRenderViewModule*);
  void processEvents(unsigned long event);
  vtkSMDisplayProxy* getDisplayProxy(pqRenderViewModule*, vtkSMProxy*, bool create_new=true);
  void createDisplayProxies(vtkSMProxy*, bool show=true);
  void clearClientDisplays();
  void createNewClientDisplays(pqServerManagerSelection&);
  /// set the active selection for a particular connection.
  /// If selection is NULL, then the current active selection is
  /// cleared.
  void setActiveSelection(vtkIdType cid, vtkSMSelectionProxy* selection);
  void activeSelectionRegistered(vtkSMSelectionProxy* selection);
  // creates the SelectionSource proxy for the connection if none
  // exists.
  void initializeSelectionSource(vtkIdType cid);
};


#endif

