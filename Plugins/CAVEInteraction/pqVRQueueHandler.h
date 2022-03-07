/*=========================================================================

   Program: ParaView
   Module:  pqVRQueueHandler.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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
#ifndef pqVRQueueHandler_h
#define pqVRQueueHandler_h

#include <QtCore/QObject>
#include <QtCore/QPointer>

class vtkVRInteractorStyle;
class vtkVRQueue;
class vtkPVXMLElement;
class vtkSMProxyLocator;

/// pqVRQueueHandler is a class that process events stacked on to vtkVRQueue
/// one by one. One adds vtkVRInteractorStyles to the handler to do any actual
/// work with these events.
class pqVRQueueHandler : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqVRQueueHandler(vtkVRQueue* queue, QObject* parent = 0);
  virtual ~pqVRQueueHandler();

  /// Add/remove interactor style.
  void add(vtkVRInteractorStyle* style);
  void remove(vtkVRInteractorStyle* style);
  void clear();

  QList<vtkVRInteractorStyle*> styles();

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
