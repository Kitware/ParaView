/*=========================================================================

   Program:   ParaQ
   Module:    pqPipelineBrowser.h

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

/// \file pqPipelineBrowser.h
/// \date 4/20/2006

#ifndef _pqPipelineBrowser_h
#define _pqPipelineBrowser_h


#include "pqWidgetsExport.h"
#include <QWidget>

class pqFlatTreeView;
class pqPipelineModel;
class pqPipelineObject;
class pqPipelineServer;
class pqServer;
class QItemSelectionModel;
class QModelIndex;
class vtkSMProxy;


class PQWIDGETS_EXPORT pqPipelineBrowser : public QWidget
{
  Q_OBJECT

public:
  pqPipelineBrowser(QWidget *parent=0);
  virtual ~pqPipelineBrowser();

  virtual bool eventFilter(QObject *object, QEvent *e);

  pqPipelineModel *getListModel() const {return this->ListModel;}
  pqFlatTreeView *getTreeView() const {return this->TreeView;}

  QItemSelectionModel *getSelectionModel() const;
  pqPipelineServer *getCurrentServer() const;

  vtkSMProxy *getSelectedProxy() const;
  vtkSMProxy *getNextProxy() const; // TEMP

signals:
  void proxySelected(vtkSMProxy *proxy);

public slots:
  void selectProxy(vtkSMProxy *proxy);
  void selectServer(pqServer *server);

private slots:
  void changeCurrent(const QModelIndex &current, const QModelIndex &previous);

private:
  pqPipelineModel *ListModel;
  pqFlatTreeView *TreeView;
};

#endif
