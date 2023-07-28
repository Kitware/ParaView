// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqVRQueueHandler_h
#define pqVRQueueHandler_h

#include <QtCore/QObject>
#include <QtCore/QPointer>

class vtkSMVRInteractorStyleProxy;
class vtkVRQueue;
class vtkPVXMLElement;
class vtkSMProxyLocator;

/// pqVRQueueHandler is a class that process events stacked on to vtkVRQueue
/// one by one. One adds vtkSMVRInteractorStyleProxys to the handler to do any actual
/// work with these events.
class pqVRQueueHandler : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqVRQueueHandler(vtkVRQueue* queue, QObject* parent = 0);
  virtual ~pqVRQueueHandler();

  /// Add/remove interactor style.
  void add(vtkSMVRInteractorStyleProxy* style);
  void remove(vtkSMVRInteractorStyleProxy* style);
  void clear();

  QList<vtkSMVRInteractorStyleProxy*> styles();

  static pqVRQueueHandler* instance();

public Q_SLOTS:
  /// start/stop queue processing.
  void start();
  void stop();

  /// clears current interactor styles and loads a new set of styles from the
  /// XML configuration.
  void configureStyles(vtkPVXMLElement* xml, vtkSMProxyLocator* locator);

  /// saves the styles configuration.
  void saveStylesConfiguration(vtkPVXMLElement* root);

Q_SIGNALS:
  void stylesChanged();

protected Q_SLOTS:
  /// called to processes events from the queue.
  void processEvents();

private:
  Q_DISABLE_COPY(pqVRQueueHandler)
  void render();
  class pqInternals;
  pqInternals* Internals;

  friend class pqVRStarter;
  static void setInstance(pqVRQueueHandler*);
  static QPointer<pqVRQueueHandler> Instance;
};

#endif // pqVRQueueHandler_h
