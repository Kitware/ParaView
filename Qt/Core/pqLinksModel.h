/*=========================================================================

   Program: ParaView
   Module:    pqLinksModel.h

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

#ifndef _pqLinksModel_h
#define _pqLinksModel_h


#include "pqCoreExport.h"
#include <QAbstractItemModel>

class vtkSMLink;
class vtkSMProxy;
class vtkSMRenderModuleProxy;
class pqServerManagerModelItem;
class pqProxy;
class vtkSMProxyListDomain;


class PQCORE_EXPORT pqLinksModel : public QAbstractTableModel
{
  Q_OBJECT
  typedef QAbstractTableModel Superclass;

public:
    /// type of link (camera, proxy or property)
  enum ItemType
    {
    Unknown,
    Proxy,
    Camera,
    Property
    };

public:
  /// construct a links model
  pqLinksModel(QObject *parent=0);

  /// destruct a links model
  ~pqLinksModel();

  // implementation to satisfy api
  int rowCount(const QModelIndex &parent=QModelIndex()) const;
  int columnCount(const QModelIndex &parent=QModelIndex()) const;
  QVariant data(const QModelIndex &index, int role=Qt::DisplayRole) const;
  QVariant headerData(int section, Qt::Orientation orient, 
                      int role=Qt::DisplayRole) const;

  // subclass specific implementation
  ItemType getLinkType(const QModelIndex& idx) const;
  vtkSMLink* getLink(const QModelIndex& idx) const;
  QModelIndex findLink(vtkSMLink* link) const;

  vtkSMProxy* getInputProxy(const QModelIndex& idx) const;
  vtkSMProxy* getOutputProxy(const QModelIndex& idx) const;
  
  QString getInputProperty(const QModelIndex& idx) const;
  QString getOutputProperty(const QModelIndex& idx) const;
  
  QString getLinkName(const QModelIndex& idx) const;
  vtkSMLink* getLink(const QString& name) const;

  void addProxyLink(const QString& name, 
                    vtkSMProxy* inputProxy, vtkSMProxy* outputProxy);
  
  void addCameraLink(const QString& name, 
                    vtkSMRenderModuleProxy* inputProxy,
                    vtkSMRenderModuleProxy* outputProxy);

  void addPropertyLink(const QString& name,
                       vtkSMProxy* inputProxy, const QString& inputProp,
                       vtkSMProxy* outputProxy, const QString& outputProp);

  void removeLink(const QModelIndex& idx);
  void removeLink(const QString& name);

  /// Return a representative proxy.
  /// It could be itself, or in the case of internal proxies, the owning
  /// pqProxy.
  static pqProxy* representativeProxy(vtkSMProxy* proxy);
  
  /// return the proxy list domain for a proxy
  /// this domain is used to get internal linkable proxies
  static vtkSMProxyListDomain* proxyListDomain(vtkSMProxy* proxy);

private:
  ItemType getLinkType(vtkSMLink* link) const;
  vtkSMProxy* getProxyFromIndex(const QModelIndex& idx, int dir) const;
  QString getPropertyFromIndex(const QModelIndex& idx, int dir) const;

  class pqInternal;
  pqInternal* Internal;
};


class pqLinksModelObject : public QObject
{
  Q_OBJECT
public:
  pqLinksModelObject(QString name, pqLinksModel* p);
  ~pqLinksModelObject();

  QString name() const;
  vtkSMLink* link() const;

private slots:
  void proxyModified(pqServerManagerModelItem*);
  void refresh();
  void remove();

private:
  class pqInternal;
  pqInternal* Internal;
};

#endif

