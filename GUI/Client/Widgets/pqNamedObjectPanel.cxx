/*=========================================================================

   Program:   ParaQ
   Module:    pqNamedObjectPanel.cxx

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

// this include
#include "pqNamedObjectPanel.h"

// Qt includes
#include <QLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QGroupBox>
#include <QSlider>
#include <QDoubleSpinBox>

// VTK includes

// paraview includes
#include "vtkSMProperty.h"
#include "vtkSMPropertyIterator.h"

// paraq includes
#include "pqListWidgetItemObject.h"
#include "pqPipelineData.h"
#include "pqPipelineDisplay.h"
#include "pqPipelineModel.h"
#include "pqPipelineSource.h"
#include "pqServerManagerModel.h"
#include "pqSignalAdaptors.h"
#include "pqSMAdaptor.h"
#include "pqSMProxy.h"
#include "pqSMSignalAdaptors.h"


/// constructor
pqNamedObjectPanel::pqNamedObjectPanel(QWidget* p)
  : pqObjectPanel(p)
{
}
/// destructor
pqNamedObjectPanel::~pqNamedObjectPanel()
{
}

void pqNamedObjectPanel::linkServerManagerProperties()
{
  vtkSMPropertyIterator *iter = this->Proxy->NewPropertyIterator();
  for(iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProperty* SMProperty = iter->GetProperty();
    if(SMProperty->GetInformationOnly())
      {
      continue;
      }

    QString regex = QString("^") + QString(iter->GetKey());
    QList<QObject*> foundObjects = this->findChildren<QObject*>(QRegExp(regex));
    for(int i=0; i<foundObjects.size(); i++)
      {
      QObject* foundObject = foundObjects[i];
      pqSMAdaptor::PropertyType pt = pqSMAdaptor::getPropertyType(SMProperty);

      if(pt == pqSMAdaptor::MULTIPLE_ELEMENTS)
        {
        QLayout* propertyLayout = qobject_cast<QLayout*>(foundObject);
        if(propertyLayout)
          {
          QList<QList<QVariant> > domain = pqSMAdaptor::getMultipleElementPropertyDomain(SMProperty);
          for(int j=0; j<domain.size(); j++)
            {
            QString name;
            name.setNum(j);
            name = QString(iter->GetKey()) + QString(":") + name;
            QLineEdit* le = propertyLayout->parentWidget()->findChild<QLineEdit*>(name);
            if(le)
              {
              pqObjectPanel::PropertyManager.registerLink(le, "text", SIGNAL(textChanged(const QString&)),
                                                 this->Proxy, SMProperty, j);
              }
            }
          }
        }
      else if(pt == pqSMAdaptor::ENUMERATION)
        {
        // enumerations can be combo boxes or check boxes
        QCheckBox* checkBox = qobject_cast<QCheckBox*>(foundObject);
        QComboBox* comboBox = qobject_cast<QComboBox*>(foundObject);
        QLineEdit* lineEdit = qobject_cast<QLineEdit*>(foundObject);
        if(checkBox)
          {
          pqObjectPanel::PropertyManager.registerLink(checkBox, "checked", SIGNAL(toggled(bool)),
                                             this->Proxy, SMProperty);
          }
        else if(comboBox)
          {
          QList<QVariant> domain = pqSMAdaptor::getEnumerationPropertyDomain(SMProperty);
          comboBox->clear();
          foreach(QVariant v, domain)
            {
            comboBox->addItem(v.toString());
            }
          pqSignalAdaptorComboBox* adaptor = new pqSignalAdaptorComboBox(comboBox);
          adaptor->setObjectName("ComboBoxAdaptor");
          pqObjectPanel::PropertyManager.registerLink(adaptor, "currentText", SIGNAL(currentTextChanged(const QString&)),
                                             this->Proxy, SMProperty);
          }
        else if(lineEdit)
          {
          pqObjectPanel::PropertyManager.registerLink(lineEdit, "text", SIGNAL(textChanged(const QString&)),
                                             this->Proxy, SMProperty);
          }
        }
      else if(pt == pqSMAdaptor::SELECTION)
        {
        // selections can be list widgets
        QListWidget* listWidget = qobject_cast<QListWidget*>(foundObject);
        if(listWidget)
          {
          listWidget->clear();
          QList<QVariant> sel_domain = pqSMAdaptor::getSelectionPropertyDomain(SMProperty);
          for(int j=0; j<sel_domain.size(); j++)
            {
            pqListWidgetItemObject* item = new pqListWidgetItemObject(sel_domain[j].toString(), listWidget);
            pqObjectPanel::PropertyManager.registerLink(item, "checked", SIGNAL(checkedStateChanged(bool)),
                                               this->Proxy, SMProperty, j);
            }
          }
        }
      else if(pt == pqSMAdaptor::PROXY)
        {
        QComboBox* comboBox = qobject_cast<QComboBox*>(foundObject);
        if(comboBox)
          {
          QList<pqSMProxy> propertyDomain = pqSMAdaptor::getProxyPropertyDomain(this->Proxy, SMProperty);
          comboBox->clear();
          foreach(pqSMProxy v, propertyDomain)
            {
            pqPipelineSource* o = pqServerManagerModel::instance()->getPQSource(v);
            if(o)
              {
              comboBox->addItem(o->getProxyName());
              }
            }
          pqSignalAdaptorComboBox* comboAdaptor = new pqSignalAdaptorComboBox(comboBox);
          comboAdaptor->setObjectName("ComboBoxAdaptor");
          pqSignalAdaptorProxy* proxyAdaptor = new pqSignalAdaptorProxy(comboAdaptor, "currentText", 
                                                 SIGNAL(currentTextChanged(const QString&)));
          proxyAdaptor->setObjectName("ComboBoxProxyAdaptor");
          pqObjectPanel::PropertyManager.registerLink(proxyAdaptor, "proxy", SIGNAL(proxyChanged(const QVariant&)),
                                             this->Proxy, SMProperty);
          }
        }
      else if(pt == pqSMAdaptor::SINGLE_ELEMENT)
        {
        QComboBox* comboBox = qobject_cast<QComboBox*>(foundObject);
        QLineEdit* lineEdit = qobject_cast<QLineEdit*>(foundObject);
        QSlider* slider = qobject_cast<QSlider*>(foundObject);
        QDoubleSpinBox* doubleSpinBox = qobject_cast<QDoubleSpinBox*>(foundObject);
        if(comboBox)
          {
          QList<QVariant> domain = pqSMAdaptor::getElementPropertyDomain(SMProperty);
          comboBox->clear();
          for(int j=0; j<domain.size(); j++)
            {
            comboBox->addItem(domain[j].toString());
            }

          pqSignalAdaptorComboBox* adaptor = new pqSignalAdaptorComboBox(comboBox);
          adaptor->setObjectName("ComboBoxAdaptor");
          pqObjectPanel::PropertyManager.registerLink(adaptor, "currentText", SIGNAL(currentTextChanged(const QString&)),
                                             this->Proxy, SMProperty);
          }
        else if(lineEdit)
          {
          pqObjectPanel::PropertyManager.registerLink(lineEdit, "text", SIGNAL(textChanged(const QString&)),
                                             this->Proxy, SMProperty);
          }
        else if(slider)
          {
          pqObjectPanel::PropertyManager.registerLink(slider, "value", SIGNAL(valueChanged(int)),
                                             this->Proxy, SMProperty);
          }
        else if(doubleSpinBox)
          {
          pqObjectPanel::PropertyManager.registerLink(doubleSpinBox, "value", SIGNAL(valueChanged(double)),
                                             this->Proxy, SMProperty);
          }
        }
      else if(pt == pqSMAdaptor::FILE_LIST)
        {
        QLineEdit* lineEdit = qobject_cast<QLineEdit*>(foundObject);
        if(lineEdit)
          {
          pqObjectPanel::PropertyManager.registerLink(lineEdit, "text", SIGNAL(textChanged(const QString&)),
                                             this->Proxy, SMProperty);
          }
        }
      }
    }
  iter->Delete();

}

void pqNamedObjectPanel::unlinkServerManagerProperties()
{
  vtkSMPropertyIterator *iter = this->Proxy->NewPropertyIterator();
  for(iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProperty* SMProperty = iter->GetProperty();
    if(SMProperty->GetInformationOnly())
      {
      continue;
      }

    QString regex = QString("^") + QString(iter->GetKey());
    QList<QObject*> foundObjects = this->findChildren<QObject*>(QRegExp(regex));
    for(int i=0; i<foundObjects.size(); i++)
      {
      QObject* foundObject = foundObjects[i];
      
      pqSMAdaptor::PropertyType pt = pqSMAdaptor::getPropertyType(SMProperty);

      if(pt == pqSMAdaptor::MULTIPLE_ELEMENTS)
        {
        QLayout* propertyLayout = qobject_cast<QLayout*>(foundObject);
        if(propertyLayout)
          {
          QList<QList<QVariant> > domain = pqSMAdaptor::getMultipleElementPropertyDomain(SMProperty);
          for(int j=0; j<domain.size(); j++)
            {
            QString name;
            name.setNum(j);
            name = QString(iter->GetKey()) + QString(":") + name;
            QLineEdit* le = propertyLayout->parentWidget()->findChild<QLineEdit*>(name);
            if(le)
              {
              pqObjectPanel::PropertyManager.unregisterLink(le, "text", SIGNAL(textChanged(const QString&)),
                                               this->Proxy, SMProperty, j);
              }
            }
          }
        }
      else if(pt == pqSMAdaptor::ENUMERATION)
        {
        // enumerations can be combo boxes or check boxes
        QCheckBox* checkBox = qobject_cast<QCheckBox*>(foundObject);
        QComboBox* comboBox = qobject_cast<QComboBox*>(foundObject);
        QLineEdit* lineEdit = qobject_cast<QLineEdit*>(foundObject);
        if(checkBox)
          {
          pqObjectPanel::PropertyManager.unregisterLink(checkBox, "checked", SIGNAL(toggled(bool)),
                                             this->Proxy, SMProperty);
          }
        else if(comboBox)
          {
          pqSignalAdaptorComboBox* adaptor = comboBox->findChild<pqSignalAdaptorComboBox*>("ComboBoxAdaptor");
          pqObjectPanel::PropertyManager.unregisterLink(adaptor, "currentText", SIGNAL(currentTextChanged(const QString&)),
                                             this->Proxy, SMProperty);
          delete adaptor;
          }
        else if(lineEdit)
          {
          pqObjectPanel::PropertyManager.unregisterLink(lineEdit, "text", SIGNAL(textChanged(const QString&)),
                                             this->Proxy, SMProperty);
          }
        }
      else if(pt == pqSMAdaptor::SELECTION)
        {
        // selections can be list widgets
        QListWidget* listWidget = qobject_cast<QListWidget*>(foundObject);
        if(listWidget)
          {
          for(int ii=0; ii<listWidget->count(); ii++)
            {
            pqListWidgetItemObject* item = static_cast<pqListWidgetItemObject*>(listWidget->item(ii));
            pqObjectPanel::PropertyManager.unregisterLink(item, "checked", SIGNAL(checkedStateChanged(bool)),
                                               this->Proxy, SMProperty, ii);
            }
          listWidget->clear();
          }
        }
      else if(pt == pqSMAdaptor::PROXY)
        {
        QComboBox* comboBox = qobject_cast<QComboBox*>(foundObject);
        if(comboBox)
          {
          QObject* comboAdaptor = comboBox->findChild<pqSignalAdaptorComboBox*>("ComboBoxAdaptor");
          QObject* proxyAdaptor = comboBox->findChild<pqSignalAdaptorComboBox*>("ComboBoxProxyAdaptor");
          pqObjectPanel::PropertyManager.unregisterLink(proxyAdaptor, "proxy", SIGNAL(proxyChanged(const QVariant&)),
                                             this->Proxy, SMProperty);
          delete proxyAdaptor;
          delete comboAdaptor;
          }
        }
      else if(pt == pqSMAdaptor::SINGLE_ELEMENT)
        {
        QComboBox* comboBox = qobject_cast<QComboBox*>(foundObject);
        QLineEdit* lineEdit = qobject_cast<QLineEdit*>(foundObject);
        QSlider* slider = qobject_cast<QSlider*>(foundObject);
        QDoubleSpinBox* doubleSpinBox = qobject_cast<QDoubleSpinBox*>(foundObject);
        if(comboBox)
          {
          pqSignalAdaptorComboBox* adaptor = comboBox->findChild<pqSignalAdaptorComboBox*>("ComboBoxAdaptor");
          pqObjectPanel::PropertyManager.unregisterLink(adaptor, "currentText", SIGNAL(currentTextChanged(const QString&)),
                                             this->Proxy, SMProperty);
          delete adaptor;
          }
        else if(lineEdit)
          {
          pqObjectPanel::PropertyManager.unregisterLink(lineEdit, "text", SIGNAL(textChanged(const QString&)),
                                             this->Proxy, SMProperty);
          }
        else if(slider)
          {
          pqObjectPanel::PropertyManager.unregisterLink(slider, "value", SIGNAL(valueChanged(int)),
                                             this->Proxy, SMProperty);
          }
        else if(doubleSpinBox)
          {
          pqObjectPanel::PropertyManager.unregisterLink(doubleSpinBox, "value", SIGNAL(valueChanged(double)),
                                             this->Proxy, SMProperty);
          }
        }
      else if(pt == pqSMAdaptor::FILE_LIST)
        {
        QLineEdit* lineEdit = qobject_cast<QLineEdit*>(foundObject);
        if(lineEdit)
          {
          pqObjectPanel::PropertyManager.unregisterLink(lineEdit, "text", SIGNAL(textChanged(const QString&)),
                                             this->Proxy, SMProperty);
          }
        }
      }
    }
  iter->Delete();
  this->Proxy->UpdateVTKObjects();
}


