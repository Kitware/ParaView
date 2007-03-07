/*=========================================================================

   Program: ParaView
   Module:    pqLookmarkBrowserModel.cxx

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

/// \file pqLookmarkBrowserModel.cxx
/// \date 6/23/2006

#include "pqLookmarkBrowserModel.h"

#include <QList>
#include <QImage>
#include <QString>
#include <QtDebug>
#include <QByteArray>
#include <QBuffer>
#include <QPointer>
#include <QRect>

#include "pqSettings.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "pqApplicationCore.h"
#include "pqServerStartupBrowser.h"
#include "vtkSMProxyManager.h"
#include "pqServer.h"
#include "pqActiveView.h"
#include "pqRenderViewModule.h"
#include "pqLookmarkStateLoader.h"
#include "pqPipelineBuilder.h"
#include "pqGenericViewModule.h"
#include "pqMainWindowCore.h"
#include "QVTKWidget.h"
#include "assert.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "pqStateLoader.h"

// Temporary data structure for storing the metadata of a lookmark. 
struct pqLookmarkInfo
{
  QString Name;
  QString DataName;
  QImage Icon;
  QImage Pipeline;
  QString Comments;
  QString State;
  bool RestoreCamera;
  bool RestoreData;
};


class pqLookmarkBrowserModelInternal : public QList<pqLookmarkInfo> 
{
public:
  QPointer<pqMainWindowCore> MainWindowCore;
};


pqLookmarkBrowserModel::pqLookmarkBrowserModel(QObject *parentObject)
  : QAbstractListModel(parentObject)
{
  this->Internal = new pqLookmarkBrowserModelInternal();
  this->Internal->MainWindowCore = dynamic_cast<pqMainWindowCore*>(parentObject);

  // Restore the contents of the lookmark browser from a previous ParaView session, if any.
  pqSettings* settings = pqApplicationCore::instance()->settings();
  QString key = "LookmarkBrowserState";
  if(settings->contains(key))
    {
    this->addLookmarks(settings->value(key).toString());
    }
}

pqLookmarkBrowserModel::~pqLookmarkBrowserModel()
{
  // Store the contents of the lookmarks browser for a subsequent ParaView session
  pqSettings* settings = pqApplicationCore::instance()->settings();
  settings->setValue("LookmarkBrowserState", this->getAllLookmarks());

  delete this->Internal;
}

int pqLookmarkBrowserModel::rowCount(const QModelIndex &parentIndex) const
{
  if(this->Internal && !parentIndex.isValid())
    {
    return this->Internal->size();
    }

  return 0;
}

QModelIndex pqLookmarkBrowserModel::index(int row, int column,
    const QModelIndex &parentIndex) const
{
  if(this->Internal && !parentIndex.isValid() && column == 0 && row >= 0 &&
      row < this->Internal->size())
    {
    return this->createIndex(row, column, 0);
    }

  return QModelIndex();
}

QVariant pqLookmarkBrowserModel::data(const QModelIndex &idx,
    int role) const
{
  if(this->Internal && idx.isValid() && idx.model() == this)
    {
    pqLookmarkInfo info = (*this->Internal)[idx.row()];
    switch(role)
      {
      case Qt::DisplayRole:
      case Qt::EditRole:
        {
        return QVariant(info.Name);
        }
      case Qt::DecorationRole:
        {
        return QVariant(this->getSmallLookmarkIcon(idx));
        }
      }
    }

  return QVariant();
}

Qt::ItemFlags pqLookmarkBrowserModel::flags(const QModelIndex &) const
{
  return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

QString pqLookmarkBrowserModel::getLookmarkDataName(
    const QModelIndex &idx) const
{
  if(this->Internal && idx.isValid() && idx.model() == this)
    {
    return (*this->Internal)[idx.row()].DataName;
    }

  return QString();
}

QString pqLookmarkBrowserModel::getLookmarkName(
    const QModelIndex &idx) const
{
  if(this->Internal && idx.isValid() && idx.model() == this)
    {
    return (*this->Internal)[idx.row()].Name;
    }

  return QString();
}

void pqLookmarkBrowserModel::setLookmarkName(
    const QModelIndex &idx, QString name)
{
  if(this->Internal && idx.isValid() && idx.model() == this)
    {
    (*this->Internal)[idx.row()].Name = name;
    }
  emit this->dataChanged(idx,idx);
}


QString pqLookmarkBrowserModel::getLookmarkState(
    const QModelIndex &idx) const
{
  if(this->Internal && idx.isValid() && idx.model() == this)
    {
    return (*this->Internal)[idx.row()].State;
    }

  return QString();
}


QString pqLookmarkBrowserModel::getLookmarkComments(
    const QModelIndex &idx) const
{
  if(this->Internal && idx.isValid() && idx.model() == this)
    {
    return (*this->Internal)[idx.row()].Comments;
    }

  return QString();
}


void pqLookmarkBrowserModel::setLookmarkComments(
    const QModelIndex &idx, QString comments)
{
  if(this->Internal && idx.isValid() && idx.model() == this)
    {
    (*this->Internal)[idx.row()].Comments = comments;
    }
}

QImage pqLookmarkBrowserModel::getSmallLookmarkIcon(
    const QModelIndex &idx) const
{
  if(this->Internal && idx.isValid() && idx.model() == this)
    {
    return (*this->Internal)[idx.row()].Icon.scaled(48,48);
    }

  return QImage();
}


QImage pqLookmarkBrowserModel::getLargeLookmarkIcon(
    const QModelIndex &idx) const
{
  if(this->Internal && idx.isValid() && idx.model() == this)
    {
    return (*this->Internal)[idx.row()].Icon;
    }

  return QImage();
}

QImage pqLookmarkBrowserModel::getLookmarkPipeline(
    const QModelIndex &idx) const
{
  if(this->Internal && idx.isValid() && idx.model() == this)
    {
    return (*this->Internal)[idx.row()].Pipeline;
    }

  return QImage();
}


bool pqLookmarkBrowserModel::getLookmarkRestoreCameraFlag(
    const QModelIndex &idx) const
{
  if(this->Internal && idx.isValid() && idx.model() == this)
    {
    return (*this->Internal)[idx.row()].RestoreCamera;
    }
  return 0;
}


void pqLookmarkBrowserModel::setLookmarkRestoreCameraFlag(
    const QModelIndex &idx, bool state)
{
  if(this->Internal && idx.isValid() && idx.model() == this)
    {
    (*this->Internal)[idx.row()].RestoreCamera = state;
    }
}

bool pqLookmarkBrowserModel::getLookmarkRestoreDataFlag(
    const QModelIndex &idx) const
{
  if(this->Internal && idx.isValid() && idx.model() == this)
    {
    return (*this->Internal)[idx.row()].RestoreData;
    }
  return 0;
}


void pqLookmarkBrowserModel::setLookmarkRestoreDataFlag(
    const QModelIndex &idx, bool state)
{
  if(this->Internal && idx.isValid() && idx.model() == this)
    {
    (*this->Internal)[idx.row()].RestoreData = state;
    }
}

QModelIndex pqLookmarkBrowserModel::getIndexFor(
    const QString &filter) const
{
  if(this->Internal && !filter.isEmpty())
    {
    int row = 0;
    for( ; row < this->Internal->size(); row++)
      {
      QString compName = (*this->Internal)[row].Name;
      if(QString::compare(filter, compName) == 0)
        {
        break;
        }
      }
    if(row != this->Internal->size())
      {
      return this->createIndex(row, 0, 0);
      }
    }

  return QModelIndex();
}

void pqLookmarkBrowserModel::addLookmark(QString name, QString dataName, QString comments, QImage &image, QImage &pipeline, QString state, bool restoreData, bool restoreCamera)
{
  if(!this->Internal || name.isEmpty())
    {
    return;
    }

  // Insert the lookmark in alphabetical order.
/*
  int row = 0;
  for( ; row < this->Internal->size(); row++)
    {
    if(QString::compare(name, (*this->Internal)[row].Name) < 0)
      {
      break;
      }
    }
*/
  int row = this->Internal->size();
  pqLookmarkInfo info;
  info.Name = name;
  info.DataName = dataName;
  info.Comments = comments;
  info.Icon = image;
  info.Pipeline = pipeline;
  info.State = state;
  info.RestoreCamera = restoreCamera;
  info.RestoreData = restoreData;

  this->beginInsertRows(QModelIndex(), row, row);
  this->Internal->insert(row, info);
  this->endInsertRows();

  emit this->lookmarkAdded(name);
}

void pqLookmarkBrowserModel::removeLookmark(const QModelIndex &idx)
{
  if(!this->Internal)
    {
    return;
    }

  // Notify the view that the index is going away.
  this->beginRemoveRows(QModelIndex(), idx.row(), idx.row());
  this->Internal->removeAt(idx.row());
  this->endRemoveRows();
}


void pqLookmarkBrowserModel::removeLookmark(QString name)
{
  if(!this->Internal || name.isEmpty())
    {
    return;
    }

  // Find the row for the lookmark.
  int row = 0;
  for( ; row < this->Internal->size(); row++)
    {
    if(QString::compare(name, (*this->Internal)[row].Name) == 0)
      {
      break;
      }
    }
  if(row == this->Internal->size())
    {
    qDebug() << "Compound proxy definition not found in the model.";
    return;
    }

  // Notify the view that the index is going away.
  this->beginRemoveRows(QModelIndex(), row, row);
  this->Internal->removeAt(row);
  this->endRemoveRows();
}


QString pqLookmarkBrowserModel::getAllLookmarks()
{
  
  QModelIndexList list;
  for(int i=0; i<this->rowCount(); i++)
    {
    list.push_back(this->index(i,0));
    }

  return this->getLookmarks(list);
}

QString pqLookmarkBrowserModel::getLookmarks(const QModelIndexList &lookmarks)
{
  vtkPVXMLElement *root = vtkPVXMLElement::New();
  root->SetName("LookmarkDefinitionFile");

  QList<QModelIndex>::const_iterator iter = lookmarks.begin();

  for( ; iter != lookmarks.end(); ++iter)
    {
    vtkPVXMLElement *lookmark = vtkPVXMLElement::New();
    lookmark->SetName("LookmarkDefinition");
    lookmark->AddAttribute("Name", this->getLookmarkName(*iter).toAscii().constData());
    lookmark->AddAttribute("DataName", this->getLookmarkDataName(*iter).toAscii().constData());
    lookmark->AddAttribute("Comments", this->getLookmarkComments(*iter).toAscii().constData());
    lookmark->AddAttribute("RestoreData", this->getLookmarkRestoreDataFlag(*iter));
    lookmark->AddAttribute("RestoreCamera", this->getLookmarkRestoreCameraFlag(*iter));

    // Server manager state
    QString state = this->getLookmarkState(*iter);
    vtkPVXMLParser *parser = vtkPVXMLParser::New();
    char *charArray = new char[state.size()];
    const QChar *ptr = state.unicode();
    int j;
  // This is a hack for converting the QString to a char*. None of qstring's conversion methods were working.
    for(j=0; j<state.size(); j++)
      {
      charArray[j] = (char)ptr->toAscii();
      ptr++;
      if(ptr->isNull())
        {
        break;
        }
      }
    istrstream *is = new istrstream(charArray,j+1);
    parser->SetStream(is);
    parser->Parse();
    vtkPVXMLElement *stateElement = parser->GetRootElement();
    if(stateElement)
      {
      lookmark->AddNestedElement(stateElement);
      }
    parser->Delete();
    delete charArray;
    delete is;

    // Icon
    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    QImage image = this->getLargeLookmarkIcon(*iter);
    image.save(&buffer, "PNG"); // writes image into ba in PNG format
    ba = ba.toBase64();
    vtkPVXMLElement *iconElement = vtkPVXMLElement::New();
    iconElement->SetName("Icon");
    iconElement->AddAttribute("Value",ba.constData());
    lookmark->AddNestedElement(iconElement);
    iconElement->Delete();

    // Pipeline Icon
    QByteArray ba2;
    QBuffer buffer2(&ba2);
    buffer2.open(QIODevice::WriteOnly);
    QImage pipeline = this->getLookmarkPipeline(*iter);
    pipeline.save(&buffer2, "PNG"); // writes image into ba in PNG format
    ba2 = ba2.toBase64();
    vtkPVXMLElement *pipelineElement = vtkPVXMLElement::New();
    pipelineElement->SetName("Pipeline");
    pipelineElement->AddAttribute("Value",ba2.constData());
    lookmark->AddNestedElement(pipelineElement);
    pipelineElement->Delete();

    root->AddNestedElement(lookmark);
    lookmark->Delete();
    }

  ostrstream os;
  root->PrintXML(os,vtkIndent(0));
  os << ends;
  os.rdbuf()->freeze(0);
  QString inspectorState = os.str();
  root->Delete();

  return inspectorState;
}


void pqLookmarkBrowserModel::addLookmarks(QString state)
{
  if(state.isNull())
    {
    return;
    }

  char *charArray = new char[state.size()];
  const QChar *ptr = state.unicode();
  int j;
  // This is a hack for converting the QString to a char*. None of qstring's conversion methods were working.
  for(j=0; j<state.size(); j++)
    {
    charArray[j] = (char)ptr->toAscii();
    ptr++;
    if(ptr->isNull())
      {
      break;
      }
    }
  istrstream *is = new istrstream(charArray,j+1);

  vtkPVXMLParser *parser = vtkPVXMLParser::New();
  parser->SetStream(is);
  parser->Parse();
  vtkPVXMLElement *root = parser->GetRootElement();
  if(root)
    {
    this->addLookmarks(root);
    }

  parser->Delete();
  delete is;
}


void pqLookmarkBrowserModel::addLookmarks(vtkPVXMLElement *root)
{
  if(!root)
    {
    return;
    }

  int i=0;
  vtkPVXMLElement *lookmark;
  while( (lookmark = root->GetNestedElement(i++)) )
    {
    QString name = lookmark->GetAttribute("Name");
    QString dataName = lookmark->GetAttribute("DataName");
    QString comments = lookmark->GetAttribute("Comments");
    int restoreData;
    int restoreCamera;
    lookmark->GetScalarAttribute("RestoreData",&restoreData);
    lookmark->GetScalarAttribute("RestoreCamera",&restoreCamera);
    vtkPVXMLElement *stateElem = lookmark->FindNestedElementByName("ServerManagerState");
    if(!stateElem)
      {
      continue;
      }
    ostrstream os;
    stateElem->PrintXML(os,vtkIndent(0));
    os << ends;
    os.rdbuf()->freeze(0);
    QString state = os.str(); 

    vtkPVXMLElement *iconElement = lookmark->FindNestedElementByName("Icon");
    QImage image;
    if(iconElement)
      {
      QByteArray array(iconElement->GetAttribute("Value"));
      image.loadFromData(QByteArray::fromBase64(array),"PNG");
      }
    vtkPVXMLElement *pipelineElement = lookmark->FindNestedElementByName("Pipeline");
    QImage pipeline;
    if(pipelineElement)
      {
      QByteArray array(pipelineElement->GetAttribute("Value"));
      pipeline.loadFromData(QByteArray::fromBase64(array),"PNG");
      }
    this->addLookmark(name, dataName, comments, image, pipeline, state, restoreData, restoreCamera);
    }
}


void pqLookmarkBrowserModel::loadLookmark(const QModelIndex &idx, pqServer *server)
{
  if(!idx.isValid() || !server)
    {
    return;
    }

  QString state = this->getLookmarkState(idx);
  vtkPVXMLParser *parser = vtkPVXMLParser::New();

  // This is a hack for converting the QString to a char*. None of qstring's conversion methods were working.
  char *charArray = new char[state.size()];
  const QChar *ptr = state.unicode();
  int i;
  for(i=0; i<state.size(); i++)
    {
    charArray[i] = (char)ptr->toAscii();
    ptr++;
    if(ptr->isNull())
      {
      break;
      }
    }
  istrstream *is = new istrstream(charArray,i+1);
  parser->SetStream(is);
  parser->Parse();
  vtkPVXMLElement *smState = parser->GetRootElement();
  if(smState)
    {  
    // for now, load it in the current view
    vtkSmartPointer<pqLookmarkStateLoader> loader;
    loader.TakeReference(pqLookmarkStateLoader::New());
    loader->SetMainWindowCore(this->Internal->MainWindowCore);
    loader->SetUseCameraFlag(this->getLookmarkRestoreCameraFlag(idx));
    loader->SetUseDataFlag(this->getLookmarkRestoreDataFlag(idx));
    // only support render views for now
    pqRenderViewModule* renModule = qobject_cast<pqRenderViewModule*>(pqActiveView::instance().current());
    if(!renModule)
      {
      renModule = qobject_cast<pqRenderViewModule*>(pqPipelineBuilder::instance()->createView(server,pqRenderViewModule::renderViewType()));
      }
    loader->AddPreferredRenderModule(renModule->getRenderModuleProxy());
    pqApplicationCore::instance()->loadState(smState,server,loader);
/*
    renModule->render();
    QVTKWidget* const widget = qobject_cast<QVTKWidget*>(renModule->getWidget());
    assert(widget);
    widget->resize(widget->width(),widget->height());
    int *size = widget->GetRenderWindow()->GetSize();
*/
    //loader->Delete();

    emit this->lookmarkLoaded();
    }
  parser->Delete();
  delete charArray;
  delete is;
}

