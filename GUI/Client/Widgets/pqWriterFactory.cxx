/*=========================================================================

   Program:   ParaQ
   Module:    pqWriterFactory.cxx

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
#include "pqWriterFactory.h"

// ParaView includes.
#include "vtkProcessModule.h"
#include "vtkProcessModuleConnectionManager.h"
#include "vtkSmartPointer.h"
#include "vtkSMWriterProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"

// Qt includes.
#include <QDomDocument>
#include <QFileInfo>
#include <QList>
#include <QStringList>
#include <QtDebug>

// ParaQ includes.
#include "pqApplicationCore.h"
#include "pqPipelineBuilder.h"
#include "pqPipelineSource.h"
#include "pqServer.h"

struct pqWriterInfo
{
  vtkSmartPointer<vtkSMProxy> PrototypeProxy;
  QString Description;
  QList<QString> Extensions;

  // Tells is the file extension matches the supported extensions.
  bool canWriteFile(const QString& filename) const
    {
    if (!this->PrototypeProxy.GetPointer())
      {
      return false;
      }

    QFileInfo info(filename);
    QString extension = info.suffix();
    return this->Extensions.contains(extension);
    }

  // Checks if the writer can write the output from the given source proxy.
  bool canWriteOutput(pqPipelineSource* source) const
    {
    if (!this->PrototypeProxy.GetPointer() || !source)
      {
      return false;
      }
    vtkSMWriterProxy* writer = vtkSMWriterProxy::SafeDownCast(
      this->PrototypeProxy);
    // If it's not a vtkSMWriterProxy, then we assume that it can
    // always work in parallel.
    if (writer)
      {
      if (source->getServer()->getNumberOfPartitions() > 1
        && !writer->GetSupportsParallel())
        {
        return false;
        }
      }
    vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
      this->PrototypeProxy->GetProperty("Input"));
    if (!pp)
      {
      qDebug() << this->PrototypeProxy->GetXMLGroup()
        << " : " << this->PrototypeProxy->GetXMLName()
        << " has no input property.";
      return false;
      }
    pp->RemoveAllUncheckedProxies();
    pp->AddUncheckedProxy(source->getProxy());
    return pp->IsInDomains();
    }

  // Returns a string for the file type as needed by file dialogs.
  QString getTypeString() const
    {
    QString type ;
    type += this->Description + "(";
    foreach (const QString &ext, this->Extensions)
      {
      type += "*." + ext + " ";
      }
    type += ")";
    return type;
    }
};

class pqWriterFactoryInternal
{
public:
  QList<pqWriterInfo> WriterList;

  vtkSMProxy* getPrototype(const QString& xmlgroup, const QString& xmlname) const
    {
    foreach (const pqWriterInfo& info, this->WriterList)
      {
      if (info.PrototypeProxy && xmlname == info.PrototypeProxy->GetXMLName()
        && xmlgroup == info.PrototypeProxy->GetXMLGroup())
        {
        return info.PrototypeProxy;
        }
      }
    return NULL;
    }
};

//-----------------------------------------------------------------------------
pqWriterFactory::pqWriterFactory(QObject* _parent/*=NULL*/) : QObject(_parent)
{
  this->Internal = new pqWriterFactoryInternal;
}

//-----------------------------------------------------------------------------
pqWriterFactory::~pqWriterFactory()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqWriterFactory::addFileType(const QString& description, 
  const QString& extension, const QString& xmlgroup, const QString& xmlname)
{
  QList<QString> exts;
  exts.push_back(extension);
  this->addFileType(description, exts, xmlgroup, xmlname);
}

//-----------------------------------------------------------------------------
void pqWriterFactory::addFileType(const QString& description, 
  const QList<QString>& extensions, const QString& xmlgroup, 
  const QString& xmlname)
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSmartPointer<vtkSMProxy> writer;

  writer = this->Internal->getPrototype(xmlgroup, xmlname);
  if (!writer)
    {
    writer.TakeReference(pxm->NewProxy(xmlgroup.toStdString().c_str(), 
      xmlname.toStdString().c_str()));
    if (!writer)
      {
      qDebug() << "Failed to create writer prototype : " << xmlgroup 
        << ", " << xmlname;
      return;
      }
    writer->SetConnectionID(
      vtkProcessModuleConnectionManager::GetSelfConnectionID());
    writer->SetServers(vtkProcessModule::CLIENT);
    }
  this->addFileType(description, extensions, writer);
}

//-----------------------------------------------------------------------------
void pqWriterFactory::addFileType(const QString& description, 
  const QString& extension, vtkSMProxy* prototype)
{
  QList<QString> exts;
  exts.push_back(extension);
  this->addFileType(description, exts, prototype);
}
//-----------------------------------------------------------------------------
void pqWriterFactory::addFileType(const QString& description, 
  const QList<QString>& extensions, vtkSMProxy* writer)
{
  pqWriterInfo info;
  info.Description = description;
  info.Extensions = extensions;
  info.PrototypeProxy = writer;

  this->Internal->WriterList.push_back(info);
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqWriterFactory::newWriter(const QString& filename,
  pqPipelineSource* toWrite)
{
  if (!toWrite)
    {
    qDebug() << "Cannot write output of NULL source.";
    return NULL;
    }
  
  foreach (const pqWriterInfo &info, this->Internal->WriterList)
    {
    if (info.canWriteFile(filename) && info.canWriteOutput(toWrite))
      {
      vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
      vtkSMProxy* proxy = pxm->NewProxy(info.PrototypeProxy->GetXMLGroup(),
        info.PrototypeProxy->GetXMLName());
      if (proxy)
        {
        proxy->SetConnectionID(toWrite->getServer()->GetConnectionID());
        proxy->SetServers(vtkProcessModule::DATA_SERVER);
        return proxy;
        }
      }
    }

  return NULL;
}
  
//-----------------------------------------------------------------------------
QString pqWriterFactory::getSupportedFileTypes(pqPipelineSource* toWrite)
{
  QString types = "";
  if (!toWrite)
    {
    return types;
    }

  QList<QString> supportedWriters;

  // TODO: We are only looking into sources group for now.
  pqApplicationCore::instance()->getPipelineBuilder()->
    getSupportedProxies("writers", toWrite->getServer(), supportedWriters);

  bool first = true;
  foreach(const pqWriterInfo &info , this->Internal->WriterList)
    {
    if (!info.PrototypeProxy || 
      !supportedWriters.contains(info.PrototypeProxy->GetXMLName()))
      {
      // skip writers not supported by the server.
      continue;
      }
    if (info.canWriteOutput(toWrite))
      {
      if (!first)
        {
        types += ";;";
        }
      types += info.getTypeString();
      first = false;
      }
    }
  return types;
}

//-----------------------------------------------------------------------------
void pqWriterFactory::loadFileTypes(const QString& xmlfilename)
{
  QFile xml(xmlfilename);
  if (!xml.open(QIODevice::ReadOnly))
    {
    qDebug() << "Failed to load " << xmlfilename;
    return;
    }

  QDomDocument doc("doc");
  if (!doc.setContent(&xml))
    {
    xml.close();
    qDebug() << "Failed to load " << xmlfilename;
    return;
    }
  QDomNodeList readerElements = doc.elementsByTagName("Writer");
  for(int cc=0; cc < readerElements.size(); cc++)
    {
    QDomNode node = readerElements.item(cc);
    QDomElement writer = node.toElement();
    if (writer.isNull())
      {
      continue;
      }
    QString name = writer.attribute("name");
    QString extensions = writer.attribute("extensions");
    QString desc = writer.attribute("file_description");
    QString group = writer.attribute("group", "writers");
    QStringList exts = extensions.split(" ", QString::SkipEmptyParts);
    this->addFileType(desc, exts, group, name.toStdString().c_str());
    }
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
