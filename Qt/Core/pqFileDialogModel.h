/*=========================================================================

   Program: ParaView
   Module:    pqFileDialogModel.h

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

=========================================================================*/

#ifndef _pqFileDialogModel_h
#define _pqFileDialogModel_h

#include "pqCoreExport.h"
#include <QObject>
#include <QAbstractItemModel>

class vtkProcessModule;
class pqServer;
class QModelIndex;

/**
pqFileDialogModel allows remote browsing of a connected ParaView server's
filesystem, as well as browsing of the local file system.

To use, pass a new instance of pqServerFileDialogModel to pqFileDialog object.

\sa pqFileDialog
*/
class PQCORE_EXPORT pqFileDialogModel : public QAbstractItemModel
{
  typedef QAbstractItemModel base;
  
  Q_OBJECT

public:
  /// server is the server for which we need the listing.
  /// if the server is NULL, we get file listings locally
  pqFileDialogModel(pqServer* server, QObject* Parent);
  ~pqFileDialogModel();

  /// Returns the path that will be automatically displayed when the file dialog is opened
  QString getStartPath();
  /// Sets the path that the file dialog will display
  void setCurrentPath(const QString&);
  /// Changes the current path to its immediate parent path (this is a no-op if
  /// the current path is already at the root of the filesystem)
  void setParentPath();
  /// Returns the path the the file dialog will display
  QString getCurrentPath();
  /// Return true iff the given row is a directory
  bool isDir(const QModelIndex&);
  /// Returns the set of file paths associated with the given row 
  /// (a row may represent one-to-many paths if grouping is implemented)
  QStringList getFilePaths(const QModelIndex&);
  /// Converts a file into an absolute path
  QString getFilePath(const QString&);
  /// Returns all of the paths that are parents of the given path 
  /// (this is handled by the back-end so it can deal with 
  /// issues of delimiters, symlinks, multi-root filesystems, etc)
  QStringList getParentPaths(const QString&);
  /// Returns whether the file exists
  bool fileExists(const QString&);
  /// Returns whether a directory exists
  bool dirExists(const QString&);

  // overloads for QAbstractItemModel
  int columnCount(const QModelIndex&) const;
  QVariant data(const QModelIndex & idx, int role) const;
  QModelIndex index(int row, int column, const QModelIndex&) const;
  QModelIndex parent(const QModelIndex&) const;
  int rowCount(const QModelIndex&) const;
  bool hasChildren(const QModelIndex& p) const;
  QVariant headerData(int section, Qt::Orientation, int role) const;
  
  /// return the model for favorites
  QAbstractItemModel* favoriteModel();
  
private:
  class pqImplementation;
  pqImplementation* const Implementation;
};

#endif // !_pqFileDialogModel_h

