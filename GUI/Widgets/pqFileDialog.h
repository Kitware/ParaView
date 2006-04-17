/*=========================================================================

   Program:   ParaQ
   Module:    pqFileDialog.h

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

#ifndef _pqFileDialog_h
#define _pqFileDialog_h

#include "QtWidgetsExport.h"
#include <QDialog>

class pqFileDialogModel;
namespace Ui { class pqFileDialog; }
class QModelIndex;

/**
  Provides a standard file dialog "front-end" for the pqFileDialogModel "back-end", i.e. it can be used for both local and remote file browsing.

  When creating an instance of pqFileDialog, you must provide an instance of the back-end.  To get the file(s) selected by the user, connect
  to the filesSelected() signal:
  
  /code
  pqFileDialog* dialog = new pqFileDialog(new pqLocalFileDialogModel(), "Open Session File", this, "OpenSessionFile");
  dialog << pqConnect(SIGNAL(filesSelected(const QStringList&)), this, SLOT(onOpenSessionFile(const QStringList&)));
  /endcode
  
  /sa pqLocalFileDialogModel, pqServerFileDialogModel
*/

class QTWIDGETS_EXPORT pqFileDialog :
  public QDialog
{
  typedef QDialog base;
  
  Q_OBJECT
  
public:
  pqFileDialog(pqFileDialogModel* Model, const QString& Title, QWidget* Parent, const char* const Name);

signals:
  /// Signal emitted when the user has chosen a set of files and accepted the dialog
  void filesSelected(const QStringList&);

private:
  ~pqFileDialog();
  pqFileDialog(const pqFileDialog&);
  pqFileDialog& operator=(const pqFileDialog&);

  void accept();
  
  pqFileDialogModel* const Model;
  Ui::pqFileDialog* const Ui;
  const QModelIndex* Temp;
  
private slots:
  void onDataChanged(const QModelIndex&, const QModelIndex&);
  void onActivated(const QModelIndex&);
  void onManualEntry(const QString&);
  void onNavigate(const QString&);
  void onNavigateUp();
  void onNavigateDown();
};

#endif // !_pqFileDialog_h
