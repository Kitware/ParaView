/*=========================================================================

   Program: ParaView
   Module:    pqReaderFactory.cxx

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
#include "pqReaderFactory.h"

// ParaView Server Manager includes.
#include "vtkClientServerStream.h"
#include "vtkProcessModuleConnectionManager.h"
#include "vtkProcessModule.h"
#include "vtkProcessModule.h"
#include "vtkSmartPointer.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMDomain.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"


// Qt includes.
#include <QFileInfo>
#include <QDir>
#include <QList>
#include <QStringList>
#include <QtDebug>

// ParaView includes.
#include "pqApplicationCore.h"
#include "pqObjectBuilder.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqPluginManager.h"


//-----------------------------------------------------------------------------
struct pqReaderInfo
{
  vtkSmartPointer<vtkSMProxy> PrototypeProxy;
  QString Description;
  QList<QString> Extensions;

  bool operator==(const pqReaderInfo& other) const
    {
    return (this->Description == other.Description &&
            this->PrototypeProxy == other.PrototypeProxy &&
            this->Extensions == other.Extensions);
    }

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

  bool canReadFile(const QString& filename, const QString& extension, pqServer* server) const
    {
    if (!this->PrototypeProxy.GetPointer())
      {
      return false;
      }

    if (!extension.isEmpty() && !this->Extensions.contains(extension))
      {
      return false;
      }
    // extension matches.
    
    vtkIdType cid = server->GetConnectionID();

    vtkClientServerStream stream;
    // Assume that it can read the file (based on extension match)
    // if CanReadFile does not exist.
    int canRead = 1;
    // ImageReader always returns 0 so don't test it
    if (strcmp(this->PrototypeProxy->GetXMLName(), "ImageReader") != 0)
      {
      vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
      vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
      vtkSMProxy* proxy = 
        pxm->NewProxy("sources", this->PrototypeProxy->GetXMLName());
      proxy->SetConnectionID(cid);
      proxy->SetServers(vtkProcessModule::DATA_SERVER_ROOT);
      proxy->UpdateVTKObjects();
      stream << vtkClientServerStream::Invoke
             << pm->GetProcessModuleID() << "SetReportInterpreterErrors" << 0
             << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke
             << proxy->GetID() << "CanReadFile" << filename.toAscii().data()
             << vtkClientServerStream::End;
      pm->SendStream(cid, vtkProcessModule::DATA_SERVER_ROOT, stream);
      pm->GetLastResult(cid,
        vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0, 0, &canRead);
      stream << vtkClientServerStream::Invoke
             << pm->GetProcessModuleID() << "SetReportInterpreterErrors" << 1
             << vtkClientServerStream::End;
      pm->SendStream(cid, vtkProcessModule::DATA_SERVER_ROOT, stream);
      proxy->Delete();
      }
    return canRead;
    }
};

//-----------------------------------------------------------------------------
class pqReaderFactoryInternal
{
public:
  QList<pqReaderInfo> ReaderList;

  vtkSMProxy* getPrototype(const QString& xmlgroup, const QString& xmlname) const
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
  QString getTypeString() const
    {
    QString types = "ParaView Files (";
    foreach (const pqReaderInfo& info, this->ReaderList)
      {
      QList<QString>::const_iterator extIter = info.Extensions.begin();
      for (;extIter!=info.Extensions.end(); extIter++)
        {
        types += "*." + *extIter +" ";
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
  if (!reader && pxm->ProxyElementExists(xmlgroup.toAscii().data(),
      xmlname.toAscii().data()))
    {
    reader.TakeReference(pxm->NewProxy(xmlgroup.toAscii().data(), 
      xmlname.toAscii().data()));
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
  if (reader)
    {
    this->addFileType(description, extensions, reader);
    }
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
  
  // check that it is already added
  foreach(const pqReaderInfo &i, this->Internal->ReaderList)
    {
    if(info == i)
      {
      return;
      }
    }

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
  svp->SetElement(0, filename.toAscii().data());
  proxy->UpdateVTKObjects();
  proxy->UpdatePropertyInformation();

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    proxy->GetProperty("ActiveFileIsReadable"));

  if (ivp->GetElement(0))
    {
    return true;
    }
  return false;
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqReaderFactory::createReader(const QStringList& files,
  const QString& readerName, pqServer* server)
{
  foreach(const pqReaderInfo &info, this->Internal->ReaderList)
    {
    if(readerName == info.PrototypeProxy->GetXMLName())
      {
      pqObjectBuilder* builder = 
        pqApplicationCore::instance()->getObjectBuilder();
      pqPipelineSource* source = 
        builder->createReader("sources",      // TODO: support other groups
          info.PrototypeProxy->GetXMLName(), files, server);
      return source;
      }
    }
  return NULL;
}

//-----------------------------------------------------------------------------
QString pqReaderFactory::getReaderType(const QString& filename, 
  pqServer* server)
{
  int num = this->Internal->ReaderList.size();
  QFileInfo finfo(filename);
  QStringList exts = finfo.completeSuffix().split('.');
  // start with the last extension component working our way back to handle
  // cases such as "foo.xyz.vtk" as well as "foo.vtk.000".
  for (int cc=(exts.size()-1); cc >= 0; cc--)
    {
    QString extension = exts[cc];
    // loop backwards, allowing extensions to be overloaded
    for (int i=num-1; i >= 0; i--)
      {
      const pqReaderInfo &info = this->Internal->ReaderList[i];
      if (info.canReadFile(filename, extension, server))
        {
        return QString(info.PrototypeProxy->GetXMLName());
        }
      }
    }
  return QString();
}

//-----------------------------------------------------------------------------
QString pqReaderFactory::getSupportedFileTypes(pqServer* server)
{
  QList<QString> supportedSources;

  // TODO: We are only looking into sources group for now.
  server->getSupportedProxies("sources", supportedSources);
  

  QString types = this->Internal->getTypeString();
  foreach(const pqReaderInfo &info, this->Internal->ReaderList)
    {
    if (info.PrototypeProxy && 
      supportedSources.contains(info.PrototypeProxy->GetXMLName()))
      {
      types += ";;" + info.getTypeString();
      }
    }
  return types;
}

//-----------------------------------------------------------------------------
QStringList pqReaderFactory::getSupportedReaders(pqServer* server)
{
  return this->getSupportedReadersForFile(server, QString());
}

//-----------------------------------------------------------------------------
QStringList pqReaderFactory::getSupportedReadersForFile(pqServer *server,
                                                        const QString &filename)
{
  QStringList supportedSources;
  QStringList supportedReaders;

  // TODO: We are only looking into sources group for now.
  server->getSupportedProxies("sources", supportedSources);

  foreach(const pqReaderInfo &info, this->Internal->ReaderList)
    {
    if (   info.PrototypeProxy
        && supportedSources.contains(info.PrototypeProxy->GetXMLName())
        && (filename.isEmpty() || info.canReadFile(filename,QString(),server)) )
      {
      supportedReaders.append(info.PrototypeProxy->GetXMLName());
      }
    }
  return supportedReaders;
}

//-----------------------------------------------------------------------------
QString pqReaderFactory::getReaderDescription(const QString& reader)
{
  foreach(const pqReaderInfo &info, this->Internal->ReaderList)
    {
    if (info.PrototypeProxy && 
      reader == info.PrototypeProxy->GetXMLName())
      {
      return info.Description;
      }
    }
  return QString("No Description");
}

QString pqReaderFactory::getExtensionTypeString(pqPipelineSource* reader)
{
  QString ext;
  foreach(const pqReaderInfo &info, this->Internal->ReaderList)
    {
    vtkSMSourceProxy* psp;
    psp = vtkSMSourceProxy::SafeDownCast(info.PrototypeProxy);
    vtkSMSourceProxy* sp;
    sp = vtkSMSourceProxy::SafeDownCast(reader->getProxy());

    if (sp && psp && 
        strcmp(psp->GetXMLName(), sp->GetXMLName()) == 0)
      {
      ext = info.getTypeString();
      }
    }
  return ext;
}

//-----------------------------------------------------------------------------
void pqReaderFactory::loadFileTypes()
{
  QString readersDirName(":/CustomResources");
  QDir readersDir(readersDirName);
  if (!readersDir.exists("CustomReaders.xml"))
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
void pqReaderFactory::loadFileTypes(const QString& xmlfilename)
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
    if(QString(reader->GetName()) == "Reader")
      {
      QString name = reader->GetAttribute("name");
      QString extensions = reader->GetAttribute("extensions");
      QString desc = reader->GetAttribute("file_description");
      const char* grp = reader->GetAttribute("group");
      QString group = grp ? grp : "sources";
      QStringList exts = extensions.split(" ", QString::SkipEmptyParts);
      this->addFileType(desc, exts, group, name.toAscii().data());
      }
    }
}

