/*=========================================================================

   Program: ParaView
   Module:    pqWriterFactory.cxx

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
#include "pqWriterFactory.h"

// ParaView Server Manager includes.
#include "vtkProcessModule.h"
#include "vtkProcessModuleConnectionManager.h"
#include "vtkSmartPointer.h"
#include "vtkSMWriterProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMInputProperty.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"

// Qt includes.
#include <QFileInfo>
#include <QDir>
#include <QList>
#include <QStringList>
#include <QtDebug>

// ParaView Server Manager includes.
#include "pqApplicationCore.h"
#include "pqPipelineSource.h"
#include "pqOutputPort.h"
#include "pqServer.h"
#include "pqPluginManager.h"

struct pqWriterInfo
{
  vtkSmartPointer<vtkSMProxy> PrototypeProxy;
  QString Description;
  QList<QString> Extensions;
  
  bool operator==(const pqWriterInfo& other) const
    {
    return (this->Description == other.Description &&
            this->PrototypeProxy == other.PrototypeProxy &&
            this->Extensions == other.Extensions);
    }

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
  bool canWriteOutput(pqOutputPort* port) const
    {
    if (!this->PrototypeProxy.GetPointer() || !port)
      {
      return false;
      }

    pqPipelineSource* source = port->getSource();

    vtkSMWriterProxy* writer = vtkSMWriterProxy::SafeDownCast(
      this->PrototypeProxy);
    // If it's not a vtkSMWriterProxy, then we assume that it can
    // always work in parallel.
    if (writer)
      {
      if (source->getServer()->getNumberOfPartitions() > 1)
        {
        if (!writer->GetSupportsParallel())
          {
          return false;
          }
        }
      else
        {
        if (writer->GetParallelOnly())
          {
          return false;
          }
        }
      }
    vtkSMInputProperty* pp = vtkSMInputProperty::SafeDownCast(
      this->PrototypeProxy->GetProperty("Input"));
    if (!pp)
      {
      qDebug() << this->PrototypeProxy->GetXMLGroup()
        << " : " << this->PrototypeProxy->GetXMLName()
        << " has no input property.";
      return false;
      }
    pp->RemoveAllUncheckedProxies();
    pp->AddUncheckedInputConnection(source->getProxy(), port->getPortNumber());
    bool status = pp->IsInDomains();
    pp->RemoveAllUncheckedProxies();
    return status;
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
  this->loadFileTypes();
  
  // watch for both types of plugins
  // the client vs. server load order is not defined
  // and we require both before adding a new reader to the GUI
  QObject::connect(pqApplicationCore::instance()->getPluginManager(),
                   SIGNAL(guiExtensionLoaded()),
                   this, SLOT(loadFileTypes()));
  QObject::connect(pqApplicationCore::instance()->getPluginManager(),
                   SIGNAL(serverManagerExtensionLoaded()),
                   this, SLOT(loadFileTypes()));
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
  if (!writer && pxm->ProxyElementExists(xmlgroup.toAscii().data(),
      xmlname.toAscii().data()))
    {
    writer.TakeReference(pxm->NewProxy(xmlgroup.toAscii().data(), 
      xmlname.toAscii().data()));
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
  
  // check that it is already added
  foreach(const pqWriterInfo &i, this->Internal->WriterList)
    {
    if(info == i)
      {
      return;
      }
    }

  this->Internal->WriterList.push_back(info);
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqWriterFactory::newWriter(const QString& filename,
  pqOutputPort* toWrite)
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
QString pqWriterFactory::getSupportedFileTypes(pqOutputPort* toWrite)
{
  QString types = "";
  if (!toWrite)
    {
    return types;
    }

  QList<QString> supportedWriters;

  // TODO: We are only looking into writers group for now.
  toWrite->getServer()->getSupportedProxies("writers", supportedWriters);

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
void pqWriterFactory::loadFileTypes()
{
  QString readersDirName(":/CustomResources");
  QDir readersDir(readersDirName);
  if (!readersDir.exists("CustomWriters.xml"))
    {
    readersDirName=":/ParaViewResources";
    readersDir.setPath(readersDirName);
    }
  QStringList resources = readersDir.entryList(QDir::Files);
  foreach(QString resource, resources)
    {
    if (QFileInfo(resource).suffix() == "xml")
      {
      this->loadFileTypes(readersDirName + QString("/") + resource);
      }
    }
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
  
  QByteArray dat = xml.readAll();
  
  vtkSmartPointer<vtkPVXMLParser> parser = 
    vtkSmartPointer<vtkPVXMLParser>::New();

  if(!parser->Parse(dat.data()))
    {
    qDebug() << "Failed to parse " << xmlfilename;
    xml.close();
    return;
    }

  vtkPVXMLElement* elem = parser->GetRootElement();
  int num = elem->GetNumberOfNestedElements();
  for(int i=0; i<num; i++)
    {
    vtkPVXMLElement* reader = elem->GetNestedElement(i);
    if(QString(reader->GetName()) == "Writer")
      {
      QString name = reader->GetAttribute("name");
      QString extensions = reader->GetAttribute("extensions");
      QString desc = reader->GetAttribute("file_description");
      const char* grp = reader->GetAttribute("group");
      QString group = grp ? grp : "writers";
      QStringList exts = extensions.split(" ", QString::SkipEmptyParts);
      this->addFileType(desc, exts, group, name.toAscii().data());
      }
    }
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
