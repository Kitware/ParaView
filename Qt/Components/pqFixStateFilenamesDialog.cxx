/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
#include "pqFixStateFilenamesDialog.h"
#include "ui_pqFixStateFilenamesDialog.h"

#include "pqActiveObjects.h"
#include "pqCollapsedGroup.h"
#include "pqFileChooserWidget.h"
#include "vtkFileSequenceParser.h"
#include "pqSMAdaptor.h"
#include "vtkPVFileInformation.h"
#include "vtkPVFileInformationHelper.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMDomain.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMFileListDomain.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"

#include <QDebug>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QSet>
#include <QStringList>
#include <QFileInfo>

class pqFixStateFilenamesDialog::pqInternals : public Ui::FixStateFilenamesDialog
{
  QStringList getValues(vtkSMProperty* prop)
    {
    QStringList filenames;
    QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(prop);
    foreach (QVariant val, values)
      {
      filenames << val.toString();
      }
    return filenames;
    }

  QSet<QString> locateFileNameProperties(vtkSMProxy* proxy)
    {
    QSet<QString> filenameProperties;
    vtkSMPropertyIterator* piter = proxy->NewPropertyIterator();
    for (piter->Begin(); !piter->IsAtEnd(); piter->Next())
      {
      vtkSMProperty* property = piter->GetProperty();
      vtkSMDomainIterator* diter = property->NewDomainIterator();
      for (diter->Begin(); !diter->IsAtEnd(); diter->Next())
        {
        if (vtkSMFileListDomain::SafeDownCast(diter->GetDomain()))
          {
          filenameProperties.insert(piter->GetKey());
          }
        }
      diter->Delete();
      }
    piter->Delete();
    return filenameProperties;
    }

  void processProxy(vtkPVXMLElement* proxyXML)
    {
    Q_ASSERT(strcmp(proxyXML->GetName(), "Proxy") == 0);

    const char* group = proxyXML->GetAttribute("group");
    const char* type = proxyXML->GetAttribute("type");
    if (group == NULL || type == NULL)
      {
      qWarning("Possibly invalid state file.");
      return;
      }
    vtkSMProxy* prototype = vtkSMProxyManager::GetProxyManager()->
      GetPrototypeProxy(group, type);
    if (!prototype)
      {
      return;
      }

    QSet<QString> filenameProperties = this->locateFileNameProperties(prototype);
    if (filenameProperties.size() == 0)
      {
      return;
      }

    vtkSMProxy* tempClone = vtkSMProxyManager::GetProxyManager()->NewProxy(
      group, type);
    tempClone->SetConnectionID(0);

    // makes it easier to determine current values for filenames.
    tempClone->LoadState(proxyXML, NULL);

    int proxyid = QString(proxyXML->GetAttribute("id")).toInt();
    // iterate over all property xmls in the proxyXML and add those xmls which
    // are in the filenameProperties set.
    for (unsigned int cc=0; cc < proxyXML->GetNumberOfNestedElements(); cc++)
      {
      vtkPVXMLElement* propXML = proxyXML->GetNestedElement(cc);
      if (propXML && propXML->GetName() && strcmp(propXML->GetName(),
          "Property") == 0)
        {
        QString propName = propXML->GetAttribute("name");
        if (filenameProperties.contains(propName))
          {
          vtkSMProperty* smproperty = tempClone->GetProperty(
            propName.toAscii().data());
          PropertyInfo info;
          info.XMLElement = propXML;
          // Is the property has repeat_command, then it's repeatable.
          info.SupportsMultiple = (smproperty->GetRepeatable() != 0);
          // Is this a directory?
          info.IsDirectory = (smproperty->GetHints() &&
            smproperty->GetHints()->FindNestedElementByName("UseDirectoryName"));
          info.Values = this->getValues(smproperty);
          info.Prototype = tempClone;
          this->PropertiesMap[proxyid][propName] = info;
          }
        }
      }
    tempClone->Delete();
    this->ProxyLabels[proxyid] = proxyXML->GetAttribute("type");
    }

  void processProxyCollection(vtkPVXMLElement* proxyCollectionXML)
    {
    Q_ASSERT(strcmp(proxyCollectionXML->GetName(), "ProxyCollection") == 0);



    const char* name = proxyCollectionXML->GetAttribute("name");
    if (name == NULL)
      {
      qWarning("Possibly invalid state file. Proxy Collection doesn't have a name attribute.");
      return;
      }

    if (strcmp(name,"sources") != 0)
      {
         return;
      }

    // iterate over all property xmls in the proxyXML and add those xmls which
    // are in the filenameProperties set.
    for (unsigned int cc=0; cc < proxyCollectionXML->GetNumberOfNestedElements(); cc++)
      {
      vtkPVXMLElement* itemXML = proxyCollectionXML->GetNestedElement(cc);
      if (itemXML && itemXML->GetName() && strcmp(itemXML->GetName(),
          "Item") == 0)
        {
        int itemId = QString(itemXML->GetAttribute("id")).toInt();
        this->CollectionsMap[itemId] = proxyCollectionXML;
        }
      }
    }

public:
  struct PropertyInfo
    {
    vtkPVXMLElement* XMLElement;
    bool IsDirectory;
    bool SupportsMultiple;
    QStringList Values;
    bool Modified;
    vtkSmartPointer<vtkSMProxy> Prototype;
    PropertyInfo() : XMLElement(0), IsDirectory(false), SupportsMultiple(false), Modified(false)
      {
      }
    };
  typedef QMap<int, QMap<QString, PropertyInfo> > PropertiesMapType;
  PropertiesMapType PropertiesMap;
  QMap<int, vtkPVXMLElement*> CollectionsMap;
  QMap<int, QString> ProxyLabels;

  vtkSmartPointer<vtkPVXMLElement> XMLRoot;

  void process(vtkPVXMLElement* xml)
    {
    if (xml == NULL)
      {
      return;
      }

    if (QString("ServerManagerState") == xml->GetName())
      {
      for (unsigned int cc=0; cc < xml->GetNumberOfNestedElements(); cc++)
        {
        vtkPVXMLElement* child = xml->GetNestedElement(cc);
        if (child && QString("Proxy") == child->GetName())
          {
          this->processProxy(child);
          }
        else if (child && QString("ProxyCollection") == child->GetName())
          {
          this->processProxyCollection(child);
          }
        }
      }
    else
      {
      for (unsigned int cc=0; cc < xml->GetNumberOfNestedElements(); cc++)
        {
        this->process(xml->GetNestedElement(cc));
        }
      }
    }
};

//-----------------------------------------------------------------------------
pqFixStateFilenamesDialog::pqFixStateFilenamesDialog(
  vtkPVXMLElement* xml, QWidget* parentObject, Qt::WindowFlags winFlags)
  : Superclass(parentObject, winFlags)
{
  Q_ASSERT(xml != 0);
  this->Internals = new pqInternals();
  this->Internals->setupUi(this);

  this->Internals->XMLRoot = xml;

  // Builds the necessary data-structures using the state file.
  this->Internals->process(xml);

  QVBoxLayout* vbox = qobject_cast<QVBoxLayout*>(this->layout());
  int position = 0;
  // Now build the gui
  pqInternals::PropertiesMapType::iterator iter;
  for (iter = this->Internals->PropertiesMap.begin();
    iter != this->Internals->PropertiesMap.end(); ++iter)
    {
    pqCollapsedGroup* group = new pqCollapsedGroup(this);
    group->setTitle(this->Internals->ProxyLabels[iter.key()]);
    vbox->insertWidget(position++, group);

    // Add properties.
    int propnum=0;
    QGridLayout* gridLayout = new QGridLayout(group);
    gridLayout->setColumnStretch(1, 2);
    QMap<QString, pqInternals::PropertyInfo>::iterator iter2 = iter.value().begin();
    for (;iter2 != iter.value().end(); ++iter2, ++propnum)
      {
      QLabel* label = new QLabel(iter2.key(), this);
      gridLayout->addWidget(label, propnum, 0);

      pqFileChooserWidget* fileChooser = new pqFileChooserWidget(this);
      fileChooser->setObjectName(iter2.key());
      fileChooser->setProperty("pq_proxy_key", iter.key());
      fileChooser->setProperty("pq_property_key", iter2.key());
      fileChooser->setForceSingleFile(!iter2.value().SupportsMultiple);
      fileChooser->setUseDirectoryMode(iter2.value().IsDirectory);
      fileChooser->setFilenames(iter2.value().Values);
      fileChooser->setServer(pqActiveObjects::instance().activeServer());
      QObject::connect(
        fileChooser, SIGNAL(filenamesChanged(const QStringList&)),
        this, SLOT(onFileNamesChanged()));
      gridLayout->addWidget(fileChooser, propnum, 1);
      }
    }
  this->SequenceParser = vtkFileSequenceParser::New();
}

//-----------------------------------------------------------------------------
pqFixStateFilenamesDialog::~pqFixStateFilenamesDialog()
{
  this->SequenceParser->Delete();
  delete this->Internals;
}

//-----------------------------------------------------------------------------
bool pqFixStateFilenamesDialog::hasFileNames() const
{
  return (this->Internals->PropertiesMap.size() > 0);
}

//-----------------------------------------------------------------------------
void pqFixStateFilenamesDialog::onFileNamesChanged()
{
  pqFileChooserWidget* file_widget =
    qobject_cast<pqFileChooserWidget*>(this->sender());
  QStringList parts = file_widget->objectName().split('+');
  int key1 = file_widget->property("pq_proxy_key").toInt();
  QString key2 = file_widget->property("pq_property_key").toString();

  pqInternals::PropertyInfo& info =
    this->Internals->PropertiesMap[key1][key2];

  QStringList filenames = file_widget->filenames();
  if (info.Values != filenames)
    {
    info.Values = filenames;
    info.Modified = true;
    }
}

//-----------------------------------------------------------------------------
vtkPVXMLElement* pqFixStateFilenamesDialog::xmlRoot() const
{
  return this->Internals->XMLRoot;
}

//-----------------------------------------------------------------------------
void pqFixStateFilenamesDialog::accept()
{
  pqInternals::PropertiesMapType::iterator iter;
  for (iter = this->Internals->PropertiesMap.begin();
    iter != this->Internals->PropertiesMap.end(); ++iter)
    {
    QMap<QString, pqInternals::PropertyInfo>::iterator iter2 = iter.value().begin();
    for (;iter2 != iter.value().end(); ++iter2)
      {
      pqInternals::PropertyInfo& info = iter2.value();
      if (!info.Modified)
        {
        continue;
        }

      // Update XML Element using new values.
      info.XMLElement->AddAttribute("number_of_elements", info.Values.size());
      for (int cc = info.XMLElement->GetNumberOfNestedElements()-1; cc >= 0; cc--)
        {
        // remove old "Element" elements.
        vtkPVXMLElement* child = info.XMLElement->GetNestedElement(cc);
        if (strcmp(child->GetName(), "Element") == 0)
          {
          info.XMLElement->RemoveNestedElement(child);
          }
        }
      int index=0;
      foreach (QString filename, info.Values)
        {
        vtkPVXMLElement* elementElement = vtkPVXMLElement::New();
        elementElement->SetName("Element");
        elementElement->AddAttribute("index", index++);
        elementElement->AddAttribute("value", filename.toAscii().data());
        info.XMLElement->AddNestedElement(elementElement);
        elementElement->Delete();
        }

      // Also fix up sources proxy collection
      int id = iter.key();
      QMap<int, vtkPVXMLElement*>::iterator proxyCollectionIter
        = this->Internals->CollectionsMap.begin();
      vtkPVXMLElement * proxyCollectionXML = proxyCollectionIter.value();
      for (unsigned int cc=0; cc < proxyCollectionXML->GetNumberOfNestedElements(); cc++)
        {
        // locate and remove old element.
        vtkPVXMLElement* itemXML = proxyCollectionXML->GetNestedElement(cc);
        int itemId = QString(itemXML->GetAttribute("id")).toInt();
        if(id == itemId)
          {
          proxyCollectionXML->RemoveNestedElement(itemXML);

          // create a new element with new source name
          vtkPVXMLElement* newItemXML = vtkPVXMLElement::New();
          newItemXML->SetName("Item");
          newItemXML->AddAttribute("id", itemId);


          newItemXML->AddAttribute("name",
            this->ConstructPipelineName(info.Values).toAscii().data());
          proxyCollectionXML->AddNestedElement(newItemXML);
          newItemXML->Delete();
          break;
          }
        }
      }
    }


  this->Superclass::accept();
}


QString pqFixStateFilenamesDialog::ConstructPipelineName(QStringList files)
{
  QFileInfo qFileInfo(files[0]);

  if(this->SequenceParser->ParseFileSequence(qFileInfo.fileName().toAscii().data()))
    {
    return this->SequenceParser->GetSequenceName();
    }

  return qFileInfo.fileName();
}