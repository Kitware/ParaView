// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqProgressManager_h
#define pqProgressManager_h

#include "pqCoreModule.h"
#include "vtkParaViewDeprecation.h" // for PARAVIEW_DEPRECATED_IN_6_3_0

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
   * When progress is enabled, the manager disable all widgets
   * except for those objects which are in the non-blockable list.
   * This is the API to add/remove non-blockable objects.
   */
  void addNonBlockableObject(QObject* o) { this->NonBlockableObjects.push_back(o); }
  void removeNonBlockableObject(QObject* o) { this->NonBlockableObjects.removeAll(o); }

  /**
   * Returns the list of non-blockable objects.
   */
  const QList<QPointer<QObject>>& nonBlockableObjects() const { return this->NonBlockableObjects; }

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Update progress. The progress must be enabled by calling
   * enableProgress(true) before calling  this method for the progress to be
   * updated.
   */
  void setProgress(const QString& message, int progress, bool processEvents = false);

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
   * Enables abort. Disable all widget except the non-blockable ones.
   */
  void setEnableAbort(bool);

  /**
   * This is called when the user clicks on the "Abort" button.
   * It is used to abort the current operation on a vtkAlgorithm object.
   */
  void triggerAbort();

  /**
   * Noop, use addNonBlockableObject/removeNonBlockableObject instead
   *
   * @returns false.
   */
  PARAVIEW_DEPRECATED_IN_6_3_0("Use NonBlockableObject instead.")
  bool unblockEvents(bool) { return false; };

Q_SIGNALS:

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

private:
  Q_DISABLE_COPY(pqProgressManager)

  QList<QPointer<QWidget>> DisabledWidgetsForAbort;
  QPointer<QWidget> FocusWidgetForAbort;
  bool AbortEnabled = false;
};

#endif
