/*=========================================================================

   Program: ParaView
   Module:    pqViewExporterManager.h

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
#ifndef __pqViewExporterManager_h 
#define __pqViewExporterManager_h

#include <QObject>
#include <QPointer>
#include "pqCoreExport.h"

class pqView;

/// pqViewExporterManager is a manager that manages exporters for views.
/// Currently, we are saying all exporters registered in "exporters" group are
/// available. If neeeded, we can add API to explicitly add exporters.
class PQCORE_EXPORT pqViewExporterManager : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;
public:
  pqViewExporterManager(QObject* parent=0);
  ~pqViewExporterManager();

  /// Returns a file type filtering string suitable for file dialogs. 
  /// Returns only those file formats that can be written using the 
  /// current view will be returned.
  QString getSupportedFileTypes() const;

  /// Exports the current view. The exporter is choosen based on the extension
  /// of the file.
  bool write(const QString& filename);

public slots:
  /// Reloads the list of exporters available. Must be called after plugins are
  /// loaded, or new proxy definitions are added etc.
  void refresh();

  /// Set the current view.
  void setView(pqView*);

signals:
  /// Fired whenever setView is called. Indicates if the current view is
  /// exportable at all.
  void exportable(bool);

private:
  pqViewExporterManager(const pqViewExporterManager&); // Not implemented.
  void operator=(const pqViewExporterManager&); // Not implemented.

  QPointer<pqView> View;
};

#endif


