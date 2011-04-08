/*=========================================================================

   Program: ParaView
   Module:    pqProgressManager.h

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
#ifndef __pqProgressManager_h
#define __pqProgressManager_h

#include "pqCoreExport.h"
#include <QObject>
#include <QPointer>
#include <QList>

class vtkEventQtSlotConnect;
class vtkObject;
class pqServer;
/// pqProgressManager is progress manager. It centralizes progress raising/
/// handling. Provides ability for any object to lock progress so that
/// only progress fired by itself will be notified to the rest of the world.
/// Also, when progress is enabled, it disables handling of mouse/key events 
/// except on those objects in the NonBlockableObjects list.
class PQCORE_EXPORT pqProgressManager : public QObject
{
  Q_OBJECT
public:
  pqProgressManager(QObject* parent=0);
  virtual ~pqProgressManager();

  /// Locks progress to respond to progress signals
  /// set by the \c object alone. All signals sent by other
  /// objects are ignored until Unlock is called.
  void lockProgress(QObject* object);

  /// Releases the progress lock.
  void unlockProgress(QObject* object);

  /// Returns if the progress is currently locked by any object.
  bool isLocked() const;

  /// When progress is enabled, the manager eats all
  /// mouse and key events fired except for those objects
  /// which are in the non-blockable list. 
  /// This is the API to add/remove non-blockable objects.
  void addNonBlockableObject(QObject* o)
    { this->NonBlockableObjects.push_back(o); }
  void removeNonBlockableObject(QObject* o)
    { this->NonBlockableObjects.removeAll(o); }

  /// Returns the list of non-blockable objects.
  const QList<QPointer<QObject> >& nonBlockableObjects() const
    {return this->NonBlockableObjects; }
protected:
  /// Filter QApplication events.
  bool eventFilter(QObject* obj, QEvent *event);

public slots:
  /// Update progress. The progress must be enbled by
  /// calling enableProgress(true) before calling  this method
  /// for the progress to be updated. 
  void setProgress(const QString& message, int progress);

  /// Enables progress. 
  void setEnableProgress(bool);

  /// Convenience slots that simply call setEnableProgress().
  void beginProgress() { this->setEnableProgress(true); }
  void endProgress() { this->setEnableProgress(false); }

  /// Enables abort.
  void setEnableAbort(bool);

  /// fires abort(). Must be called by the GUI that triggers
  /// abort.
  void triggerAbort();
signals:
  /// Emitted to trigger an abort.
  void abort();

  void progress(const QString& message, int progress);

  void enableProgress(bool);

  void enableAbort(bool);

protected slots:
  /// callbacks for signals fired from vtkProcessModule.
  void onStartProgress();
  void onEndProgress();
  void onProgress(vtkObject*);
  void onServerAdded(pqServer*);

protected:
  QPointer<QObject> Lock;
  QList<QPointer<QObject> > NonBlockableObjects;
  int ProgressCount;
  bool InUpdate; // used to avoid recursive updates.

  double LastProgressTime;
  bool EnableProgress;
  bool ReadyEnableProgress;
  vtkEventQtSlotConnect* VTKConnect;
private:
  pqProgressManager(const pqProgressManager&); // Not implemented.
  void operator=(const pqProgressManager&); // Not implemented.
};

#endif

