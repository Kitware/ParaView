/*=========================================================================

   Program: ParaView
   Module:    pqCollaborationManager.h

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

=========================================================================*/
#ifndef _pqCollaborationManager_h
#define _pqCollaborationManager_h

#include "pqCoreExport.h"
#include "vtkSMMessageMinimal.h"
#include <QObject>

class pqServer;
class pqView;
class QSignalMapper;

/// pqCollaborationManager is a QObject that aims to handle the collaboration
/// for the Qt layer. This class is used to synchronize the ActiveObject across
/// client instances as well as managing the rendering request when data has
/// been changed by other clients.
/// This class is responsible to synchronize
///    - rendering request
///    - pqProxy internal state
///    - master/slave (enable/disable edition control in UI)
///    - selected active source

class PQCORE_EXPORT pqCollaborationManager : public  QObject
{
  Q_OBJECT
public:  
  pqCollaborationManager(QObject* parent);
  virtual ~pqCollaborationManager();
  void setServer(pqServer*);

signals:
  void triggerRender(int viewId);

public slots:
  void addCollaborationEventManagement(pqView*);
  void removeCollaborationEventManagement(pqView*);

private slots:
  /// Called when a message has been sent by another client
  /// This method will trigger signals that will be used by other Qt classes
  /// to synchronize their state.
  void onClientMessage(vtkSMMessage* msg);

  /// This will be triggered by the triggerRender(int) signal
  void onTriggerRender(int viewId);

  /// This will call force render on all the renderer that needs to be render
  void render();

private:
  pqCollaborationManager(const pqCollaborationManager&);  // Not implemented.
  pqCollaborationManager& operator=(const pqCollaborationManager&); // Not implemented.

  class pqInternals;
  pqInternals* Internals;
  QSignalMapper* viewsSignalMapper;
};

#endif // !_pqCollaborationManager_h
