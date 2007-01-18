/*=========================================================================

   Program:   ParaView
   Module:    pqLinksModel.cxx

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

#include "pqLinksModel.h"

// Server manager includes
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyLink.h"
#include "vtkSMPropertyLink.h"
#include "vtkSMProperty.h"

// pqCore includes
#include "pqServerManagerModel.h"
#include "pqRenderViewModule.h"
#include "pqApplicationCore.h"
#include "pqProxy.h"

static const char* columnHeaders[] = 
{
  "Name",
  "Master",
  "Property",
  "Slave",
  "Property"
};

/// construct a links model
pqLinksModel::pqLinksModel(QObject *p)
  : Superclass(p)
{
}

/// destruct a links model
pqLinksModel::~pqLinksModel()
{
}

// implementation to satisfy api
int pqLinksModel::rowCount(const QModelIndex&) const
{
  return vtkSMProxy::GetProxyManager()->GetNumberOfLinks();
}

int pqLinksModel::columnCount(const QModelIndex&) const
{
  // name, master object, property, slave object, property
  return sizeof(columnHeaders) / sizeof(char*);
}

QVariant pqLinksModel::data(const QModelIndex &idx, int role) const
{
  if(role == Qt::DisplayRole)
    {
    QString name = this->getLinkName(idx);
    vtkSMLink* link = this->getLink(name);
    vtkSMPropertyLink* propertyLink = vtkSMPropertyLink::SafeDownCast(link);
    vtkSMProxyLink* proxyLink = vtkSMProxyLink::SafeDownCast(link);
    
    vtkSMProxy* masterProxy = NULL;
    vtkSMProxy* slaveProxy = NULL;
    QString masterProperty;
    QString slaveProperty;
    
    if(idx.column() == 0)
      {
      return name == QString::null ? "Unknown" : name;
      }
    else
      {
      if(proxyLink)
        {
        int numLinks = proxyLink->GetNumberOfLinkedProxies();
        for(int i=0; i<numLinks; i++)
          {
          vtkSMProxy* proxy = proxyLink->GetLinkedProxy(i);
          int dir = proxyLink->GetLinkedProxyDirection(proxy);
          if(!masterProxy && dir == vtkSMLink::INPUT)
            {
            masterProxy = proxy;
            }
          else if(!slaveProxy && dir == vtkSMLink::OUTPUT)
            {
            slaveProxy = proxy;
            }
          }
        }
      else if(propertyLink)
        {
        int numLinks = propertyLink->GetNumberOfLinkedProperties();
        for(int i=0; i<numLinks; i++)
          {
          vtkSMProxy* proxy = propertyLink->GetLinkedProxy(i);
          const char* prop = propertyLink->GetLinkedPropertyName(i);
          int dir = propertyLink->GetLinkedPropertyDirection(i);
          if(!masterProxy && dir == vtkSMLink::INPUT)
            {
            masterProxy = proxy;
            masterProperty = prop;
            }
          else if(!slaveProxy && dir == vtkSMLink::OUTPUT)
            {
            slaveProxy = proxy;
            slaveProperty = prop;
            }
          }
        }
      }
    
    pqServerManagerModel* smModel =
      pqApplicationCore::instance()->getServerManagerModel();
    
    pqProxy* masterpqProxy = smModel->getPQProxy(masterProxy);
    pqProxy* slavepqProxy = smModel->getPQProxy(slaveProxy);
    
    if(idx.column() == 1)
      {
      return masterpqProxy ? masterpqProxy->getProxyName() : "Unknown";
      }
    else if(idx.column() == 2)
      {
      if(proxyLink)
        {
        return "All";
        }
      return masterProperty != QString::null ? masterProperty : "Unknown"; 
      }
    else if(idx.column() == 3)
      {
      return slavepqProxy ? slavepqProxy->getProxyName() : "Unknown";
      }
    else if(idx.column() == 4)
      {
      if(proxyLink)
        {
        return "All";
        }
      return slaveProperty != QString::null ? slaveProperty : "Unknown"; 
      }
    }
  return QVariant();
}

QVariant pqLinksModel::headerData(int section, Qt::Orientation orient, 
                                  int role) const
{
  if(role == Qt::DisplayRole && 
     orient == Qt::Horizontal &&
     section >= 0 && 
     section < this->columnCount())
    {
    // column headers
    return QString(columnHeaders[section]);
    }
  else if(role == Qt::DisplayRole && orient == Qt::Vertical)
    {
    // row headers, just use numbers 1-n
    return QString("%1").arg(section+1);
    }
  
  return QVariant();
}

QString pqLinksModel::getLinkName(const QModelIndex& idx) const
{
  vtkSMProxyManager* pxm = vtkSMProxy::GetProxyManager();
  QString linkName = pxm->GetLinkName(idx.row());
  return linkName;
}

vtkSMLink* pqLinksModel::getLink(const QString& name) const
{
  vtkSMProxyManager* pxm = vtkSMProxy::GetProxyManager();
  vtkSMLink* link = pxm->GetRegisteredLink(name.toAscii().data());
  return link;
}

pqLinksModel::ItemType pqLinksModel::getLinkType(vtkSMLink* link) const
{
  if(vtkSMPropertyLink::SafeDownCast(link))
    {
    return Property;
    }
  else if(vtkSMProxyLink::SafeDownCast(link))
    {
    return Proxy;
    }
  return Unknown;
}

pqLinksModel::ItemType pqLinksModel::getLinkType(const QModelIndex& idx) const
{
  QString name = this->getLinkName(idx);
  vtkSMLink* link = this->getLink(name);
  return this->getLinkType(link);
}


void pqLinksModel::addProxyLink(const QString& name,
                                   pqProxy* inputProxy,
                                   pqProxy* outputProxy)
{
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  if(qobject_cast<pqRenderViewModule*>(inputProxy) &&
     qobject_cast<pqRenderViewModule*>(outputProxy))
     {
     // TODO: make a vtkSMCameraLink class for linking cameras
     this->addPropertyLink(name + "Position",
                           inputProxy, "CameraPositionInfo",
                           outputProxy, "CameraPosition");
     this->addPropertyLink(name + "FocalPoint",
                           inputProxy, "CameraFocalPointInfo",
                           outputProxy, "CameraFocalPoint");
     this->addPropertyLink(name + "ViewUp",
                           inputProxy, "CameraViewUpInfo",
                           outputProxy, "CameraViewUp");
     }
  else
    {
    vtkSMProxyLink* link = vtkSMProxyLink::New();
    link->AddLinkedProxy(inputProxy->getProxy(), vtkSMLink::INPUT);
    link->AddLinkedProxy(outputProxy->getProxy(), vtkSMLink::OUTPUT);
    pxm->RegisterLink(name.toAscii().data(), link);
    link->Delete();
    }
  
  this->reset();
}

void pqLinksModel::addPropertyLink(const QString& name,
                                      pqProxy* inputProxy,
                                      const QString& inputProp,
                                      pqProxy* outputProxy,
                                      const QString& outputProp)
{
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  vtkSMPropertyLink* link = vtkSMPropertyLink::New();
  
  link->AddLinkedProperty(inputProxy->getProxy(),
                          inputProp.toAscii().data(),
                          vtkSMLink::INPUT);
  link->AddLinkedProperty(outputProxy->getProxy(),
                          outputProp.toAscii().data(),
                          vtkSMLink::OUTPUT);
  pxm->RegisterLink(name.toAscii().data(), link);
  link->Delete();
  
  this->reset();
}

void pqLinksModel::removeLink(const QModelIndex& idx)
{
  if(!idx.isValid())
    {
    return;
    }
  
  // we want an index for the first column
  QModelIndex removeIdx = this->index(idx.row(), 0, idx.parent());
  // get the name from the first column
  QString name = this->data(removeIdx, Qt::DisplayRole).toString();

  if(name != QString::null)
    {
    vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
    pxm->UnRegisterLink(name.toAscii().data());
    this->reset();
    }
}

