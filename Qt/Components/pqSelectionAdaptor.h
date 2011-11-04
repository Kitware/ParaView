/*=========================================================================

   Program: ParaView
   Module:    pqSelectionAdaptor.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/
#ifndef __pqSelectionAdaptor_h
#define __pqSelectionAdaptor_h

#include "pqComponentsExport.h"
#include <QObject>
#include <QItemSelectionModel> //need for qtSelectionFlags

class QAbstractItemModel;
class QItemSelection;
class QItemSelectionModel;
class QModelIndex;
class vtkSMProxy;
class vtkSMProxySelectionModel;

/// pqSelectionAdaptor is the abstract base class for an adaptor that connects a
/// QItemSelectionModel of any QAbstractItemModel to a vtkSMProxySelectionModel.
/// When the selection in the QItemSelectionModel
/// changes, the vtkSMProxySelectionModel will be updated and vice versa.
/// Subclass typically only need to implement
/// mapToProxy() and mapFromProxy().
class PQCOMPONENTS_EXPORT pqSelectionAdaptor : public QObject
{
  Q_OBJECT
public:
  virtual ~pqSelectionAdaptor();

  // Returns a pointer to the QItemSelectionModel.
  QItemSelectionModel* getQSelectionModel() const;

  // Reurns a pointer to the pqServerManagerSelectionModel.
  vtkSMProxySelectionModel* getProxySelectionModel() const;

protected:
  pqSelectionAdaptor(QItemSelectionModel* pipelineSelectionModel,
    vtkSMProxySelectionModel* smSelectionModel, QObject* parent=0);

  /// Maps a vtkSMProxy to an index in the QAbstractItemModel.
  virtual QModelIndex mapFromProxy(vtkSMProxy* proxy) const = 0;

  /// Maps a QModelIndex to a vtkSMProxy.
  virtual vtkSMProxy* mapToProxy(const QModelIndex& index) const =0;

  /// Returns the QAbstractItemModel used by the QSelectionModel.
  /// If QSelectionModel uses a QAbstractProxyModel, this method skips
  /// over all such proxy models and returns the first non-proxy model 
  /// encountered.
  const QAbstractItemModel* getQModel() const;

protected slots:
  virtual void currentChanged(const QModelIndex& current);
  virtual void selectionChanged();

  virtual void currentProxyChanged();
  virtual void proxySelectionChanged();

  // subclasses can override this method to provide model specific selection 
  // overrides such as QItemSelection::Rows or QItemSelection::Columns etc.
  virtual QItemSelectionModel::SelectionFlag qtSelectionFlags() const 
    { return QItemSelectionModel::NoUpdate; }

private:
  class pqSelectionAdaptorInternal;
  pqSelectionAdaptorInternal* Internal;

  /// Given a QModelIndex for the QAbstractItemModel under the QItemSelectionModel,
  /// this returns the QModelIndex for the inner most non-proxy 
  /// QAbstractItemModel.
  QModelIndex mapToSource(const QModelIndex& inIndex) const;

  /// Given a QModelIndex for the innermost non-proxy QAbstractItemModel,
  /// this returns the QModelIndex for the QAbstractItemModel under the 
  /// QItemSelectionModel.
  QModelIndex mapFromSource(const QModelIndex& inIndex, 
    const QAbstractItemModel* model) const;
};


#endif

