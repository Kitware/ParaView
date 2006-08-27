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

class pqPipelineSource;
class pqRenderModule;
class pqSelectionManagerImplementation;
class vtkDataObject;
class vtkObject;
class vtkSelection;
class vtkSMDisplayProxy;
class vtkSMProxy;
class vtkSMSelectionProxy;
class vtkSMViewModuleProxy;

/// pqSelectionManager is the link between the server manager level
/// selection and the GUI. It keeps a list of selected proxies and
/// corresponding point/cell list proxies.
class PQCOMPONENTS_EXPORT pqSelectionManager : public QObject
{
  Q_OBJECT

public:
  pqSelectionManager(QObject* parent=NULL);
  virtual ~pqSelectionManager();

  /// Returns the mode the manager is in. In SELECT mode, the
  /// interaction involves drawing a rubber-band. In INTERACT
  /// mode, the user can interact with the view by rotating, panning
  // etc.
  int getMode() 
    {
      return this->Mode;
    }

  /// Returns the number of client-side data objects that were created by
  /// the selection.
  unsigned int getNumberOfSelectedObjects();

  /// Given an index, returns a client-side data objects that was created
  /// by the selection and the corresponding proxy. The return value is
  /// 1 on success, 0 on failure.
  int getSelectedObject(unsigned int idx, 
                        vtkSMProxy*& proxy, 
                        vtkDataObject*& dataObject);

  enum Modes
  {
    SELECT,
    INTERACT
  };

  friend class vtkPQSelectionObserver;

signals:
  void selectionChanged(pqSelectionManager*);
  
public slots:
  /// Change mode to SELECT
  void switchToSelection();
  /// Change mode to INTERACT
  void switchToInteraction();
  /// Clear all selections. Note that this does not clear
  /// the server manager model selection
  void clearSelection();
  /// Used to keep track of active render module
  void setActiveRenderModule(pqRenderModule*);
  /// cleans all internal proxies.
  void cleanSelections();

private slots:
  void sourceRemoved(pqPipelineSource*);
  void proxyRegistered(QString, QString, vtkSMProxy*);
  void proxyUnRegistered(QString, QString, vtkSMProxy*);
  void onSelectionUpdateVTKObjects(vtkObject* sel);
  void updateSelections();

private:
  pqSelectionManagerImplementation* Implementation;
  int Mode;

  int setInteractorStyleToSelect(pqRenderModule*);
  int setInteractorStyleToInteract(pqRenderModule*);
  void processEvents(unsigned long event);
  vtkSMDisplayProxy* getDisplayProxy(pqRenderModule*, vtkSMProxy*, bool create_new=true);
  void createDisplayProxies(vtkSMProxy*, bool show=true);

  void updateSelection(int* eventpos, pqRenderModule* rm);

  void clearClientDisplays();
  void createNewClientDisplays(pqServerManagerSelection&);

  /// set the active selection for a particular connection.
  /// If selection is NULL, then the current active selection is
  /// cleared.
  void setActiveSelection(vtkIdType cid, vtkSMSelectionProxy* selection);

  void activeSelectionRegistered(vtkSMSelectionProxy* selection);
};


#endif

