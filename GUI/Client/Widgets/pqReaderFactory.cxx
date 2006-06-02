/*=========================================================================

   Program:   ParaQ
   Module:    pqReaderFactory.cxx

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
#include "pqReaderFactory.h"

// ParaView includes.
#include "vtkClientServerStream.h"
#include "vtkProcessModuleConnectionManager.h"
#include "vtkProcessModule.h"
#include "vtkProcessModule.h"
#include "vtkSmartPointer.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"


// Qt includes.
#include <QDomDocument>
#include <QFileInfo>
#include <QList>
#include <QStringList>
#include <QtDebug>

// ParaQ includes.
#include "pqApplicationCore.h"
#include "pqPipelineBuilder.h"
#include "pqServer.h"


//-----------------------------------------------------------------------------
struct pqReaderInfo
{
  vtkSmartPointer<vtkSMProxy> PrototypeProxy;
  QString Description;
  QList<QString> Extensions;

  QString getTypeString() const
    {
    QString type ;
    type += this->Description + "(";
    foreach (QString ext, this->Extensions)
      {
      type += "*." + ext + " ";
      }
    type += ")";
    return type;
    }

  bool canReadFile(const QString& filename, pqServer* server)
    {
    if (!this->PrototypeProxy.GetPointer())
      {
      return false;
      }

    QFileInfo info(filename);
    QString extension = info.suffix();
    if (!this->Extensions.contains(extension))
      {
      return false;
      }
    // extension matches.
    
    vtkIdType cid = server->GetConnectionID();

    vtkClientServerStream stream;
    // Assume that it can read the file (based on extension match)
    // if CanReadFile does not exist.
    int canRead = 1;
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkClientServerID tmpID = 
      pm->NewStreamObject(this->PrototypeProxy->GetVTKClassName(), stream);
    stream << vtkClientServerStream::Invoke
           << pm->GetProcessModuleID() << "SetReportInterpreterErrors" << 0
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << tmpID << "CanReadFile" << filename.toStdString().c_str()
           << vtkClientServerStream::End;
    pm->SendStream( cid, vtkProcessModule::DATA_SERVER_ROOT, stream);
    pm->GetLastResult(cid,
      vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0, 0, &canRead);
    pm->DeleteStreamObject(tmpID, stream);
    stream << vtkClientServerStream::Invoke
           << pm->GetProcessModuleID() << "SetReportInterpreterErrors" << 1
           << vtkClientServerStream::End;
    pm->SendStream(cid, vtkProcessModule::DATA_SERVER_ROOT, stream);
    return canRead;
    }
};

//-----------------------------------------------------------------------------
class pqReaderFactoryInternal
{
public:
  QList<pqReaderInfo> ReaderList;

  vtkSMProxy* getPrototype(const QString& xmlgroup, const QString& xmlname)
    {
    foreach (const pqReaderInfo& info, this->ReaderList)
      {
      if (info.PrototypeProxy && xmlname == info.PrototypeProxy->GetXMLName()
        && xmlgroup == info.PrototypeProxy->GetXMLGroup())
        {
        return info.PrototypeProxy;
        }
      }
    return NULL;
    }

  // Get a single type string for all supported types.
  QString getTypeString()
    {
    QString types = "ParaView Files (";
    foreach (const pqReaderInfo& info, this->ReaderList)
      {
      foreach (QString ext, info.Extensions)
        {
        types += "*." + ext +" ";
        }
      }
    types += ")";
    return types;
    }
};

//-----------------------------------------------------------------------------
pqReaderFactory::pqReaderFactory(QObject* _parent) : QObject(_parent)
{
  this->Internal = new pqReaderFactoryInternal();
}

//-----------------------------------------------------------------------------
pqReaderFactory::~pqReaderFactory()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqReaderFactory::addFileType(const QString& description, 
  const QString& extension, const QString& xmlgroup, const QString& xmlname)
{
  QList<QString> exts;
  exts.push_back(extension);
  this->addFileType(description, exts, xmlgroup, xmlname);
}

//-----------------------------------------------------------------------------
void pqReaderFactory::addFileType(const QString& description, 
  const QList<QString>& extensions, const QString& xmlgroup, 
  const QString& xmlname)
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSmartPointer<vtkSMProxy> reader;

  reader = this->Internal->getPrototype(xmlgroup, xmlname);
  if (!reader)
    {
    reader.TakeReference(pxm->NewProxy(xmlgroup.toStdString().c_str(), 
      xmlname.toStdString().c_str()));
    if (!reader)
      {
      qDebug() << "Failed to create reader prototype : " << xmlgroup 
        << ", " << xmlname;
      return;
      }
    reader->SetConnectionID(
      vtkProcessModuleConnectionManager::GetSelfConnectionID());
    reader->SetServers(vtkProcessModule::CLIENT);
    }
  this->addFileType(description, extensions, reader);
}

//-----------------------------------------------------------------------------
void pqReaderFactory::addFileType(const QString& description, 
  const QString& extension, vtkSMProxy* prototype)
{
  QList<QString> exts;
  exts.push_back(extension);
  this->addFileType(description, exts, prototype);
}

//-----------------------------------------------------------------------------
void pqReaderFactory::addFileType(const QString& description, 
  const QList<QString>& extensions, vtkSMProxy* prototype)
{
  pqReaderInfo info;
  info.Description = description;
  info.Extensions = extensions;
  info.PrototypeProxy = prototype;

  this->Internal->ReaderList.push_back(info);
}

//-----------------------------------------------------------------------------
bool pqReaderFactory::checkIfFileIsReadable(const QString& filename, 
  pqServer* server)
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSmartPointer<vtkSMProxy> proxy;
  proxy.TakeReference(pxm->NewProxy("file_listing", "ServerFileListing"));
  if (!proxy)
    {
    qDebug() << "Failed to create ServerFileListing proxy.";
    return false;
    }
  proxy->SetConnectionID(server->GetConnectionID());
  proxy->SetServers(vtkProcessModule::DATA_SERVER_ROOT);

  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    proxy->GetProperty("ActiveFileName"));
  svp->SetElement(0, filename.toStdString().c_str());
  proxy->UpdateVTKObjects();

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    proxy->GetProperty("ActiveFileIsReadable"));

  proxy->UpdatePropertyInformation(ivp);
  if (ivp->GetElement(0))
    {
    return true;
    }
  return false;
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqReaderFactory::createReader(const QString& filename, 
  pqServer* server)
{
  if (this->checkIfFileIsReadable(filename, server))
    {
    qDebug() << "File \"" << filename << "\"  cannot be read.";
    return NULL; 
    }
  foreach(pqReaderInfo info, this->Internal->ReaderList)
    {
    if (info.canReadFile(filename, server))
      {
      pqPipelineBuilder* builder = 
        pqApplicationCore::instance()->getPipelineBuilder();
      pqPipelineSource* source = 
        builder->createSource(info.PrototypeProxy->GetXMLGroup(),
        info.PrototypeProxy->GetXMLName(), server, 0);
      return source;
      }
    }
  return NULL;
}

//-----------------------------------------------------------------------------
QString pqReaderFactory::getSupportedFileTypes()
{
  QString types = this->Internal->getTypeString();
  foreach(pqReaderInfo info, this->Internal->ReaderList)
    {
    types += ";;" + info.getTypeString();
    }
  return types;
}

//-----------------------------------------------------------------------------
void pqReaderFactory::loadFileTypes(const QString& xmlfilename)
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
  QDomNodeList readerElements = doc.elementsByTagName("Reader");
  for(int cc=0; cc < readerElements.size(); cc++)
    {
    QDomNode node = readerElements.item(cc);
    QDomElement reader = node.toElement();
    if (reader.isNull())
      {
      continue;
      }
    QString name = reader.attribute("name");
    QString extensions = reader.attribute("extensions");
    QString desc = reader.attribute("file_description");
    QString group = reader.attribute("group", "sources");
    QStringList exts = extensions.split(" ", QString::SkipEmptyParts);
    this->addFileType(desc, exts, group, name.toStdString().c_str());
    }
}
