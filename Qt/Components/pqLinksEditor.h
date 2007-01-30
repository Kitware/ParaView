/*=========================================================================

   Program: ParaView
   Module:    pqLinksEditor.h

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

#ifndef _pqLinksEditor_h
#define _pqLinksEditor_h

#include <QDialog>
#include "pqComponentsExport.h"
#include "ui_pqLinksEditor.h"
#include "pqLinksModel.h"

class pqLinksEditorProxyModel;

class PQCOMPONENTS_EXPORT pqLinksEditor :
  public QDialog, private Ui::pqLinksEditor
{
  Q_OBJECT
  typedef QDialog base;
public:

  /// create a link editor to create/edit a link
  /// initial values are retrieved from the provided vtkSMLink
  pqLinksEditor(vtkSMLink* link, QWidget* p=0);
  ~pqLinksEditor();

  QString linkName();

  pqLinksModel::ItemType linkMode();

  vtkSMProxy* selectedInputProxy();
  vtkSMProxy* selectedOutputProxy();
  
  QString selectedInputProperty();
  QString selectedOutputProperty();

private slots:
  void currentInputProxyChanged(const QModelIndex& cur, const QModelIndex&);
  void currentOutputProxyChanged(const QModelIndex& cur, const QModelIndex&);
  
  void currentInputPropertyChanged(QListWidgetItem* item);
  void currentOutputPropertyChanged(QListWidgetItem* item);
  
  void updateEnabledState();

private:

  void updatePropertyList(QListWidget* tw, vtkSMProxy* proxy);

  pqLinksEditorProxyModel* InputProxyModel;
  pqLinksEditorProxyModel* OutputProxyModel;
  
  vtkSMProxy* SelectedInputProxy;
  vtkSMProxy* SelectedOutputProxy;
  QString SelectedInputProperty;
  QString SelectedOutputProperty;

};

#endif

