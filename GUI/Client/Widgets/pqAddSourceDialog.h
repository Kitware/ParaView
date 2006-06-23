/*=========================================================================

   Program: ParaView
   Module:    pqAddSourceDialog.h

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

========================================================================*/

/// \file pqAddSourceDialog.h
/// \date 5/26/2006

#ifndef _pqAddSourceDialog_h
#define _pqAddSourceDialog_h


#include "pqWidgetsExport.h"
#include <QDialog>

class pqAddSourceDialogForm;
class pqSourceInfoModel;
class QAbstractItemModel;
class QAbstractListModel;
class QModelIndex;
class QString;
class QStringList;


class PQWIDGETS_EXPORT pqAddSourceDialog : public QDialog
{
  Q_OBJECT

public:
  pqAddSourceDialog(QWidget *parent=0);
  virtual ~pqAddSourceDialog();

  void setSourceLabel(const QString &label);

  void setSourceList(QAbstractItemModel *sources);
  void setHistoryList(QAbstractListModel *history);

  void getPath(QString &path);
  void setPath(const QString &path);
  void setSource(const QString &name);

public slots:
  void navigateBack();
  void navigateUp();
  void addFolder();
  void addFavorite();

private slots:
  void validateChoice();
  void changeRoot(const QModelIndex &index);
  void changeRoot(int index);
  void updateFromSources(const QModelIndex &current,
      const QModelIndex &previous);
  void updateFromHistory(const QModelIndex &current,
      const QModelIndex &previous);

private:
  void getPath(const QModelIndex &index, QStringList &path);

private:
  pqAddSourceDialogForm *Form;
  QAbstractItemModel *Sources;
  QAbstractListModel *History;
  pqSourceInfoModel *SourceInfo;
};

#endif
