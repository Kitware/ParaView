/*=========================================================================

   Program:   ParaQ
   Module:    pqServerFileDialogModel.h

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

#ifndef _pqServerFileDialogModel_h
#define _pqServerFileDialogModel_h

#include "pqWidgetsExport.h"
#include "pqFileDialogModel.h"

class vtkProcessModule;
class pqServer;

/**
Implementation of pqFileDialogModel that allows remote browsing of a connected ParaView server's filesystem.

To use, pass a new instance of pqServerFileDialogModel to pqFileDialog object.

\sa pqFileDialogModel, pqLocalFileDialogModel, pqFileDialog
*/
class PQWIDGETS_EXPORT pqServerFileDialogModel :
  public pqFileDialogModel
{
  typedef pqFileDialogModel base;
  
  Q_OBJECT

public:
  /// server is the server for which we need the listing.
  pqServerFileDialogModel(QObject* Parent, pqServer* server);
  ~pqServerFileDialogModel();

  QString getStartPath();
  void setCurrentPath(const QString&);
  QString getCurrentPath();
  bool isDir(const QModelIndex&);
  QStringList getFilePaths(const QModelIndex&);
  QString getFilePath(const QString&);
  QString getParentPath(const QString&);
  QStringList splitPath(const QString&);
  QAbstractItemModel* fileModel();
  QAbstractItemModel* favoriteModel();
  
private:
  class pqImplementation;
  pqImplementation* const Implementation;
  pqServer* Server;
};

#endif // !_pqServerFileDialogModel_h

