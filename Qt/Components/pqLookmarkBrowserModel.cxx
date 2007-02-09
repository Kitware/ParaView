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

#include "pqSettings.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "pqApplicationCore.h"
#include "pqServerStartupBrowser.h"
#include "vtkSMProxyManager.h"
#include "pqServer.h"

// Temporary data structure for storing the metadata of a lookmark. 
struct pqLookmarkInfo
{
  QString Name;
  QImage Icon;
  QString Comments;
  QString State;
};


class pqLookmarkBrowserModelInternal : public QList<pqLookmarkInfo> {};


pqLookmarkBrowserModel::pqLookmarkBrowserModel(QObject *parentObject)
  : QAbstractListModel(parentObject)
{
  this->Internal = new pqLookmarkBrowserModelInternal();

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
        return QVariant(info.Icon);
        }
      }
    }

  return QVariant();
}

Qt::ItemFlags pqLookmarkBrowserModel::flags(const QModelIndex &) const
{
  return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
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


QString pqLookmarkBrowserModel::getLookmarkState(
    const QModelIndex &idx) const
{
  if(this->Internal && idx.isValid() && idx.model() == this)
    {
    return (*this->Internal)[idx.row()].State;
    }

  return QString();
}



QImage pqLookmarkBrowserModel::getLookmarkIcon(
    const QModelIndex &idx) const
{
  if(this->Internal && idx.isValid() && idx.model() == this)
    {
    return (*this->Internal)[idx.row()].Icon;
    }

  return QImage();
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

void pqLookmarkBrowserModel::addLookmark(QString name, QImage &image, QString state)
{
  if(!this->Internal || name.isEmpty())
    {
    return;
    }

  // Insert the lookmark in alphabetical order.
  int row = 0;
  for( ; row < this->Internal->size(); row++)
    {
    if(QString::compare(name, (*this->Internal)[row].Name) < 0)
      {
      break;
      }
    }

  pqLookmarkInfo info;
  info.Name = name;
  info.Icon = image;
  info.State = state;

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
    QImage image = this->getLookmarkIcon(*iter);
    image.save(&buffer, "PNG"); // writes image into ba in PNG format
    ba = ba.toBase64();
    vtkPVXMLElement *iconElement = vtkPVXMLElement::New();
    iconElement->SetName("Icon");
    iconElement->AddAttribute("Value",ba.constData());
    lookmark->AddNestedElement(iconElement);
    iconElement->Delete();

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
    this->addLookmark(name,image,state);
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
    // We need to add a root <ParaView> tag in order pqApplication to parse it correctly
    vtkPVXMLElement *root = vtkPVXMLElement::New();
    root->SetName("ParaView");
    root->AddNestedElement(smState);
    pqApplicationCore::instance()->loadState(root,server);
    root->Delete();
    }
  parser->Delete();
  delete charArray;
  delete is;
}

