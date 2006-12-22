/*=========================================================================

   Program: ParaView
   Module:    pqFileChooserWidget.h

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


#ifndef _pqFileChooserWidget_h
#define _pqFileChooserWidget_h

#include "pqComponentsExport.h"
#include <QWidget>
#include <QString>
class QLineEdit;
class pqServer;

/// file chooser widget which consists of a tool button and a line edit
/// hitting the tool button will bring up a file dialog, and the chosen
/// file will be put in the line edit
class PQCOMPONENTS_EXPORT pqFileChooserWidget : public QWidget
{
  Q_OBJECT
  Q_PROPERTY(QString Filename READ filename WRITE setFilename)
  Q_PROPERTY(QString Extension READ extension WRITE setExtension)

public:
  /// constructor
  pqFileChooserWidget(QWidget* p = NULL);
  /// destructor
  ~pqFileChooserWidget();

  /// get the filename
  QString filename();
  /// set the filename
  void setFilename(const QString&);
  
  /// get the file extension for the file dialog
  QString extension();
  /// set the file extension for the file dialog
  void setExtension(const QString&);

  /// set server to work on.
  /// If server is NULL, a local file dialog is used
  void setServer(pqServer* server);
  pqServer* server();

signals:
  /// signal emitted when the filename changes
  void filenameChanged(const QString&);

protected slots:
  void chooseFile();

protected:
  QString Extension;
  QLineEdit* LineEdit;
  pqServer* Server;
};

#endif // _pqFileChooserWidget_h

