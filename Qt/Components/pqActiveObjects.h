/*=========================================================================

   Program: ParaView
   Module:    pqActiveObjects.h

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
#ifndef __pqActiveObjects_h 
#define __pqActiveObjects_h

#include <QObject>
#include "pqComponentsExport.h"

class pqView;
class pqPipelineSource;
class pqServer;
class pqOutputPort;
class pqDataRepresentation;
class pqRepresentation;

/// pqActiveObjects is a singleton that keeps track of "active objects"
/// including active view, active source, active representation etc. 
class PQCOMPONENTS_EXPORT pqActiveObjects : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;
public:
  /// Provides access to the singleton.
  static pqActiveObjects& instance();

  /// Returns the active view.
  pqView* activeView()
    { return this->CachedView; }

  /// Returns the active source
  pqPipelineSource* activeSource()
    { return this->CachedSource; }

  /// Returns the active port.
  pqOutputPort* activePort()
    { return this->CachedPort; }

  /// Returns the active server.
  pqServer* activeServer()
    { return this->CachedServer; }

  /// Returns the active representation.
  pqDataRepresentation* activeRepresentation()
    { return this->CachedRepresentation; }

public slots:
  void setActiveView(pqView * view);
  void setActiveSource(pqPipelineSource * source);
  void setActivePort(pqOutputPort * port);
  void setActiveServer(pqServer*);

signals:
  /// fired when active view changes \c view is the new active view.
  void viewChanged(pqView* view);

  void sourceChanged(pqPipelineSource*);

  void portChanged(pqOutputPort*);

  void serverChanged(pqServer*);

  void representationChanged(pqDataRepresentation*);
  void representationChanged(pqRepresentation*);

private slots:
  void activeViewChanged(pqView*);
  void onSelectionChanged();
  void onServerChanged();
  void updateRepresentation();

private:
  pqActiveObjects();
  ~pqActiveObjects();
  pqActiveObjects(const pqActiveObjects&); // Not implemented.
  void operator=(const pqActiveObjects&); // Not implemented.

  pqServer* CachedServer;
  pqPipelineSource* CachedSource;
  pqOutputPort* CachedPort;
  pqView* CachedView;
  pqDataRepresentation* CachedRepresentation;
};

#endif


