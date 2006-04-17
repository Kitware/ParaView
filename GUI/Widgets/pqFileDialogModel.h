/*=========================================================================

   Program:   ParaQ
   Module:    pqFileDialogModel.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

#include "QtWidgetsExport.h"
#include <QObject>

class QAbstractItemModel;
class QModelIndex;
class QString;

/**
  Abstract interface to a file-browsing "back-end" that can be used by the pqFileDialog "front-end".
  /sa pqFileDialog, pqLocalFileDialogModel, pqServerFileDialogModel
*/  
  
class QTWIDGETS_EXPORT pqFileDialogModel :
  public QObject
{
  Q_OBJECT

public:
  ~pqFileDialogModel();
  
  /// Returns the path that will be automatically displayed when the file dialog is opened
  virtual QString getStartPath() = 0;
  /// Sets the path that the file dialog will display
  virtual void setCurrentPath(const QString&) = 0;
  /// Returns the path the the file dialog will display
  virtual QString getCurrentPath() = 0;
  /// Return true iff the given row is a directory
  virtual bool isDir(const QModelIndex&) = 0;
  /// Returns the set of file paths associated with the given row (a row may represent one-to-many paths if grouping is implemented)
  virtual QStringList getFilePaths(const QModelIndex&) = 0;
  /// Converts a file into an absolute path
  virtual QString getFilePath(const QString&) = 0;
  /// Returns the parent path of the given file path (this is handled by the back-end so it can deal with issues of delimiters, symlinks, network resources, etc)
  virtual QString getParentPath(const QString&) = 0;
  /// Splits a path into its components (this is handled by the back-end so it can deal with issues of delimiters, symlinks, multi-root filesystems, etc)
  virtual QStringList splitPath(const QString&) = 0;

  /// Returns a Qt model that will contain the contents of the right-hand pane of the file dialog
  virtual QAbstractItemModel* fileModel() = 0;
  /// Returns a Qt model that will contain the contents of the left-hand pane of the file dialog
  virtual QAbstractItemModel* favoriteModel() = 0;

protected:
  pqFileDialogModel(QObject* Parent = 0);
};

#endif // !_pqFileDialogModel_h
