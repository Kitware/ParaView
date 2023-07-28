// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqProgressManager_h
#define pqProgressManager_h

#include "pqCoreModule.h"
#include <QList>
#include <QObject>
#include <QPointer>

class vtkObject;
class pqServer;
/**
 * pqProgressManager is progress manager. It centralizes progress raising/
 * handling. Provides ability for any object to lock progress so that
 * only progress fired by itself will be notified to the rest of the world.
 * Also, when progress is enabled, it disables handling of mouse/key events
 * except on those objects in the NonBlockableObjects list.
 */
class PQCORE_EXPORT pqProgressManager : public QObject
{
  Q_OBJECT
public:
  pqProgressManager(QObject* parent = nullptr);
  ~pqProgressManager() override;

  /**
   * Locks progress to respond to progress signals set by the \c object alone.
   * All signals sent by other objects are ignored until Unlock is called.
   */
  void lockProgress(QObject* object);

  /**
   * Releases the progress lock.
   */
  void unlockProgress(QObject* object);

  /**
   * Returns if the progress is currently locked by any object.
   */
  bool isLocked() const;

  /**
   * When progress is enabled, the manager eats all mouse and key events fired
   * except for those objects which are in the non-blockable list.
   * This is the API to add/remove non-blockable objects.
   */
  void addNonBlockableObject(QObject* o) { this->NonBlockableObjects.push_back(o); }
  void removeNonBlockableObject(QObject* o) { this->NonBlockableObjects.removeAll(o); }

  /**
   * Returns the list of non-blockable objects.
   */
  const QList<QPointer<QObject>>& nonBlockableObjects() const { return this->NonBlockableObjects; }

protected:
  /**
   * Filter QApplication events.
   */
  bool eventFilter(QObject* obj, QEvent* event) override;

public Q_SLOTS:
  /**
   * Update progress. The progress must be enbled by calling
   * enableProgress(true) before calling  this method for the progress to be
   * updated.
   */
  void setProgress(const QString& message, int progress);

  /**
   * Enables progress.
   */
  void setEnableProgress(bool);

  /**
   * Convenience slots that simply call setEnableProgress().
   */
  void beginProgress() { this->setEnableProgress(true); }
  void endProgress() { this->setEnableProgress(false); }

  /**
   * Enables abort.
   */
  void setEnableAbort(bool);

  /**
   * fires abort(). Must be called by the GUI that triggers abort.
   */
  void triggerAbort();

  /**
   * While progress is enabled, pqProgressManager blocks key/mouse events,
   * except for objects added using `addNonBlockableObject`. Sometimes it's not
   * possible to add objects explicitly and we may want to temporarily skip
   * blocking of events. This methods can be used for that e.g.
   *
   * @code{cpp}
   *  bool prev = progressManager->unblockEvents(true);
   *  QMessageBox::question(....);
   *  progressManager->unblockEvents(prev);
   * @endcode
   *
   * @returns previous value.
   */
  bool unblockEvents(bool val);

Q_SIGNALS:
  /**
   * Emitted to trigger an abort.
   */
  void abort();

  void progress(const QString& message, int progress);

  void enableProgress(bool);

  void enableAbort(bool);

  void progressStartEvent();
  void progressEndEvent();

protected Q_SLOTS:
  /**
   * callbacks for signals fired from vtkProcessModule.
   */
  void onStartProgress();
  void onEndProgress();
  void onProgress(vtkObject*);
  void onServerAdded(pqServer*);

protected: // NOLINT(readability-redundant-access-specifiers)
  QPointer<QObject> Lock;
  QList<QPointer<QObject>> NonBlockableObjects;
  int ProgressCount;
  bool InUpdate; // used to avoid recursive updates.

  bool EnableProgress;
  bool ReadyEnableProgress;
  bool UnblockEvents;

private:
  Q_DISABLE_COPY(pqProgressManager)
};

#endif
