/*=========================================================================

   Program:   ParaQ
   Module:    pqPipelineListWidget.h

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

/// \file pqPipelineListWidget.h
/// \brief
///   The pqPipelineListWidget class is used to display the pipeline
///   in the form of a tree.
///
/// \date 11/25/2005

#ifndef _pqPipelineListWidget_h
#define _pqPipelineListWidget_h

#include "pqWidgetsExport.h"
#include <QWidget>

class pqPipelineListModel;
class QModelIndex;
class QString;
class QTreeView;
class QVTKWidget;
class vtkSMProxy;


/// \class pqPipelineListWidget
/// \brief
///   The pqPipelineListWidget class is used to display the pipeline
///   in the form of a tree.
class PQWIDGETS_EXPORT pqPipelineListWidget : public QWidget
{
  Q_OBJECT

public:
  pqPipelineListWidget(QWidget *parent=0);
  virtual ~pqPipelineListWidget();

  virtual bool eventFilter(QObject *object, QEvent *e);

  pqPipelineListModel *getListModel() const {return this->ListModel;}
  QTreeView *getTreeView() const {return this->TreeView;}

  vtkSMProxy *getSelectedProxy() const;
  vtkSMProxy *getNextProxy() const; // TEMP
  QVTKWidget *getCurrentWindow() const;

signals:
  void proxySelected(vtkSMProxy *proxy);

public slots:
  void selectProxy(vtkSMProxy *proxy);
  void selectWindow(QVTKWidget *window);

  void deleteSelected();
  void deleteProxy(vtkSMProxy *proxy);

private slots:
  void changeCurrent(const QModelIndex &current, const QModelIndex &previous);
  void doViewContextMenu(const QPoint& pos);

private:
  pqPipelineListModel *ListModel;
  QTreeView *TreeView;
};

#endif
