/*=========================================================================

   Program: ParaView
   Module:    pqFileDialogRecentDirsModel.h

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
#ifndef __pqFileDialogRecentDirsModel_h 
#define __pqFileDialogRecentDirsModel_h

#include <QAbstractListModel>
#include <QStringList>
#include <QPointer>
#include "pqCoreExport.h"

class pqServer;
class pqFileDialogModel;

/// pqFileDialogRecentDirsModel is a model which used by file dialog
/// (pqFileDialog) to populate the list showing the recent directory locations.
/// This is per server based, since the locations are connection dependent.
class PQCORE_EXPORT pqFileDialogRecentDirsModel : public QAbstractListModel
{
  Q_OBJECT
  typedef QAbstractListModel Superclass;
public:
  /// server is the server for which we need the listing.
  /// if the server is NULL, we get file listings locally (i.e. builtin server).
  /// pqFileDialogModel is used to test the validity of directories.
  pqFileDialogRecentDirsModel(pqFileDialogModel*model, pqServer* server, QObject* parent);
  ~pqFileDialogRecentDirsModel();

  /// Set the directory chosen by the user so that it gets added to the recent
  /// list.
  void setChosenDir(const QString& dir);

  /// returns the path.
  QString filePath(const QModelIndex&) const;

  /// returns the data for an item
  QVariant data(const QModelIndex& idx, int role) const;
 
  /// return the number of rows in the model 
  int rowCount(const QModelIndex& idx) const;
  
  /// return header data
  QVariant headerData(int section, Qt::Orientation, int role) const;

public slots:
  void setChosenFiles(const QList<QStringList>& files);

protected:
  QStringList Directories;
  QString SettingsKey;
  QPointer<pqFileDialogModel> FileDialogModel;

private:
  pqFileDialogRecentDirsModel(const pqFileDialogRecentDirsModel&); // Not implemented.
  void operator=(const pqFileDialogRecentDirsModel&); // Not implemented.
};

#endif


