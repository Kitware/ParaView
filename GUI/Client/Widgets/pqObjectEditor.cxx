/*=========================================================================

   Program:   ParaQ
   Module:    pqObjectEditor.cxx

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
#include "pqObjectEditor.h"

// Qt includes
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QScrollArea>
#include <QGroupBox>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QApplication>

// VTK includes
#include "QVTKWidget.h"

// paraview includes
#include "vtkSMProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMUndoStack.h"

// paraq includes
#include "pqSignalAdaptors.h"
#include "pqListWidgetItemObject.h"
#include "pqSMSignalAdaptors.h"
#include "pqSMAdaptor.h"
#include "pqSMProxy.h"
#include "pqPipelineData.h"
#include "pqPipelineObject.h"


pqPropertyManager pqObjectEditor::PropertyManager;

/// constructor
pqObjectEditor::pqObjectEditor(QWidget* p)
  : QWidget(p), Proxy(NULL)
{
  this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  QBoxLayout* mainlayout = new QVBoxLayout(this);
  mainlayout->setMargin(0);
  
  QBoxLayout* buttonlayout = new QHBoxLayout();
  mainlayout->addLayout(buttonlayout);

  QScrollArea* qscroll = new QScrollArea(this);
  qscroll->setObjectName("ScrollArea");
  QWidget* w = new QWidget;
  w->setObjectName("Panel");
  w->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));
  qscroll->setWidgetResizable(true);
  qscroll->setWidget(w);
  this->PanelLayout = new QGridLayout(w);
  mainlayout->addWidget(qscroll);
  
  QPushButton* acceptButton = new QPushButton(this);
  acceptButton->setObjectName("Accept");
  acceptButton->setText(tr("Accept"));
  QPushButton* resetButton = new QPushButton(this);
  resetButton->setObjectName("Reset");
  resetButton->setText(tr("Reset"));
  buttonlayout->addWidget(acceptButton);
  buttonlayout->addWidget(resetButton);

  this->connect(acceptButton, SIGNAL(pressed()), SLOT(accept()));
  this->connect(resetButton, SIGNAL(pressed()), SLOT(reset()));

  acceptButton->setEnabled(false);
  resetButton->setEnabled(false);
  
  QObject::connect(&pqObjectEditor::PropertyManager, SIGNAL(canAcceptOrReject(bool)), 
                   acceptButton, SLOT(setEnabled(bool)));
  QObject::connect(&pqObjectEditor::PropertyManager, SIGNAL(canAcceptOrReject(bool)), 
                   resetButton, SLOT(setEnabled(bool)));
}
/// destructor
pqObjectEditor::~pqObjectEditor()
{
}

QSize pqObjectEditor::sizeHint() const
{
  // return a size hint that would reasonably fit several properties
  ensurePolished();
  QFontMetrics fm(font());
  int h = 20 * (qMax(fm.lineSpacing(), 14));
  int w = fm.width('x') * 25;
  QStyleOptionFrame opt;
  opt.rect = rect();
  opt.palette = palette();
  opt.state = QStyle::State_None;
  return (style()->sizeFromContents(QStyle::CT_LineEdit, &opt, QSize(w, h).
                                    expandedTo(QApplication::globalStrut()), this));
}

/// set the proxy to display properties for
void pqObjectEditor::setProxy(pqSMProxy p)
{
  this->setUpdatesEnabled(false);
  if(this->Proxy)
    {
    this->unlinkServerManagerProperties(this->Proxy, this);
    this->deleteWidgets();
    }
  this->Proxy = p;
  if(this->Proxy)
    {
    this->createWidgets();
    this->linkServerManagerProperties(this->Proxy, this);
    }
  this->setUpdatesEnabled(true);
}

/// get the proxy for which properties are displayed
pqSMProxy pqObjectEditor::proxy()
{
  return this->Proxy;
}

/// accept the changes made to the properties
/// changes will be propogated down to the server manager
void pqObjectEditor::accept()
{
  if(!this->Proxy)
    {
    return;
    }

  vtkSMUndoStack* urMgr = vtkSMProxyManager::GetProxyManager()->GetUndoStack();
  urMgr->BeginUndoSet(this->Proxy->GetConnectionID(), "Accept");
  this->PropertyManager.accept();
  urMgr->EndUndoSet();
  
  // cause the screen to update
  QVTKWidget *qwindow = pqPipelineData::instance()->getWindowFor(this->Proxy);
  if(qwindow)
    {
      qwindow->update();
    }
}

/// reset the changes made
/// editor will query properties from the server manager
void pqObjectEditor::reset()
{
  if(!this->Proxy)
    {
    return;
    }
  this->PropertyManager.reject();
}

void pqObjectEditor::createWidgets()
{

  if(!this->Proxy)
    {
    return;
    }


  int rowCount = 0;

  // query for proxy properties, and create widgets
  vtkSMPropertyIterator *iter = this->Proxy->NewPropertyIterator();
  for(iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProperty* SMProperty = iter->GetProperty();

    // skip information properties
    if(SMProperty->GetInformationOnly())
      {
      continue;
      }
    pqSMAdaptor::PropertyType pt = pqSMAdaptor::getPropertyType(SMProperty);

    // skip input properties
    if(pt == pqSMAdaptor::PROXY || pt == pqSMAdaptor::PROXYLIST)
      {
      if(SMProperty == this->Proxy->GetProperty("Input"))
        {
        continue;
        }
      }

    if(pt == pqSMAdaptor::PROXY)
      {
      // create a combo box with list of proxies
      QComboBox* combo = new QComboBox(this->PanelLayout->parentWidget());
      combo->setObjectName(iter->GetKey());
      QLabel* label = new QLabel(this->PanelLayout->parentWidget());
      label->setText(iter->GetKey());
      this->PanelLayout->addWidget(label, rowCount, 0, 1, 1);
      this->PanelLayout->addWidget(combo, rowCount, 1, 1, 1);
      rowCount++;
      }
    else if(pt == pqSMAdaptor::PROXYLIST)
      {
      // create a list of selections of proxies
      }
    else if(pt == pqSMAdaptor::ENUMERATION)
      {
      QVariant enum_property = pqSMAdaptor::getEnumerationProperty(this->Proxy, 
        SMProperty);
      if(enum_property.type() == QVariant::Bool)
        {
        // check box for true/false
        QCheckBox* check = new QCheckBox(iter->GetKey(), this->PanelLayout->parentWidget());
        check->setObjectName(iter->GetKey());
        this->PanelLayout->addWidget(check, rowCount, 0, 1, 2);
        rowCount++;
        }
      else
        {
        // combo box with strings
        QComboBox* combo = new QComboBox(this->PanelLayout->parentWidget());
        combo->setObjectName(iter->GetKey());
        QLabel* label = new QLabel(this->PanelLayout->parentWidget());
        label->setText(iter->GetKey());

        this->PanelLayout->addWidget(label, rowCount, 0, 1, 1);
        this->PanelLayout->addWidget(combo, rowCount, 1, 1, 1);
        rowCount++;
        }
      }
    else if(pt == pqSMAdaptor::SELECTION)
      {
      QList<QList<QVariant> > items = pqSMAdaptor::getSelectionProperty(this->Proxy, SMProperty);
      QListWidget* lw = new QListWidget(this->PanelLayout->parentWidget());
      lw->setObjectName(iter->GetKey());
      this->PanelLayout->addWidget(lw, rowCount, 0, 1, 2);
      rowCount++;
      }
    else if(pt == pqSMAdaptor::SINGLE_ELEMENT)
      {
      QVariant elem_property = pqSMAdaptor::getElementProperty(this->Proxy, SMProperty);
      QList<QVariant> propertyDomain = pqSMAdaptor::getElementPropertyDomain(SMProperty);
      if(elem_property.type() == QVariant::String && propertyDomain.size())
        {
        // combo box with strings
        QComboBox* combo = new QComboBox(this->PanelLayout->parentWidget());
        combo->setObjectName(iter->GetKey());
        QLabel* label = new QLabel(this->PanelLayout->parentWidget());
        label->setText(iter->GetKey());
        
        this->PanelLayout->addWidget(label, rowCount, 0, 1, 1);
        this->PanelLayout->addWidget(combo, rowCount, 1, 1, 1);
        rowCount++;
        }
      else if(elem_property.type() == QVariant::Int && propertyDomain.size() == 2)
        {
        QLabel* label = new QLabel(this->PanelLayout->parentWidget());
        label->setText(iter->GetKey());
        QSlider* slider = new QSlider(Qt::Horizontal, this->PanelLayout->parentWidget());
        slider->setObjectName(QString(iter->GetKey()) + "Slider");
        slider->setRange(propertyDomain[0].toInt(), propertyDomain[1].toInt());

        QLineEdit* lineEdit = new QLineEdit(this->PanelLayout->parentWidget());
        lineEdit->setObjectName(iter->GetKey());
        
        this->PanelLayout->addWidget(label, rowCount, 0, 1, 1);
        QHBoxLayout* hlayout = new QHBoxLayout;
        hlayout->addWidget(slider);
        hlayout->addWidget(lineEdit);
        this->PanelLayout->addLayout(hlayout, rowCount, 1, 1, 1);
        slider->show();
        lineEdit->show();
        rowCount++;
        }
      else if(elem_property.type() == QVariant::Double && propertyDomain.size() == 2)
        {
        QLabel* label = new QLabel(this->PanelLayout->parentWidget());
        label->setText(iter->GetKey());
        QDoubleSpinBox* spinBox = new QDoubleSpinBox(this->PanelLayout->parentWidget());
        spinBox->setObjectName(iter->GetKey());
        spinBox->setRange(propertyDomain[0].toDouble(), propertyDomain[1].toDouble());

        this->PanelLayout->addWidget(label, rowCount, 0, 1, 1);
        this->PanelLayout->addWidget(spinBox, rowCount, 1, 1, 1);
        rowCount++;
        }
      else
        {
        QLineEdit* lineEdit = new QLineEdit(this->PanelLayout->parentWidget());
        lineEdit->setObjectName(iter->GetKey());
        QLabel* label = new QLabel(this->PanelLayout->parentWidget());
        label->setText(iter->GetKey());
        
        this->PanelLayout->addWidget(label, rowCount, 0, 1, 1);
        this->PanelLayout->addWidget(lineEdit, rowCount, 1, 1, 1);
        rowCount++;
        }
      }
    else if(pt == pqSMAdaptor::MULTIPLE_ELEMENTS)
      {
      QList<QVariant> list_property = pqSMAdaptor::getMultipleElementProperty(this->Proxy, SMProperty);
      
      QLabel* label = new QLabel(this->PanelLayout->parentWidget());
      label->setText(iter->GetKey());
      this->PanelLayout->addWidget(label, rowCount, 0, 1, 1);
      QGridLayout* glayout = new QGridLayout;
      glayout->setObjectName(iter->GetKey());
      this->PanelLayout->addLayout(glayout, rowCount, 1, 1, 1);

      int i=0;
      if(list_property.size() == 6)
        {
        // 3x2
        foreach(QVariant v, list_property)
          {
          QString num;
          num.setNum(i);
          QLineEdit* lineEdit = new QLineEdit(this->PanelLayout->parentWidget());
          lineEdit->setObjectName(QString(iter->GetKey()) + QString(":") + num);
          glayout->addWidget(lineEdit, i/2, i%2, 1, 1);
          lineEdit->show();
          i++;
          }
        }
      else
        {
        // all on one line
        foreach(QVariant v, list_property)
          {
          QString num;
          num.setNum(i);
          QLineEdit* lineEdit = new QLineEdit(this->PanelLayout->parentWidget());
          lineEdit->setObjectName(QString(iter->GetKey()) + QString(":") + num);
          glayout->addWidget(lineEdit, 0, i, 1, 1);
          lineEdit->show();
          i++;
          }
        }
        rowCount++;
      }
    else if(pt == pqSMAdaptor::FILE_LIST)
      {
      QLineEdit* lineEdit = new QLineEdit(this->PanelLayout->parentWidget());
      lineEdit->setObjectName(iter->GetKey());
      QLabel* label = new QLabel(this->PanelLayout->parentWidget());
      label->setText(iter->GetKey());
      
      this->PanelLayout->addWidget(label, rowCount, 0, 1, 1);
      this->PanelLayout->addWidget(lineEdit, rowCount, 1, 1, 1);
      rowCount++;
      }
    }
  iter->Delete();
  this->PanelLayout->addItem(new QSpacerItem(0,0,QSizePolicy::Expanding,QSizePolicy::Expanding), rowCount, 0, 1, 2);
  this->PanelLayout->invalidate();
  
}

static void pqObjectEditorDeleteWidgets(QLayoutItem* item)
{
  if(item->widget())
    {
    delete item->widget();
    }
  else if(item->layout())
    {
    QLayoutItem *child;
    while ((child = item->layout()->takeAt(0)) != 0) 
      {
      pqObjectEditorDeleteWidgets(child);
      delete child;
      }
    delete item->layout();
    }
}

void pqObjectEditor::deleteWidgets()
{
  // delete all child widgets
  QLayoutItem *child;
  while ((child = this->PanelLayout->takeAt(0)) != 0) 
    {
    pqObjectEditorDeleteWidgets(child);
    }
}

void pqObjectEditor::linkServerManagerProperties(pqSMProxy proxy, QWidget* widget)
{
  vtkSMPropertyIterator *iter = proxy->NewPropertyIterator();
  for(iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProperty* SMProperty = iter->GetProperty();
    if(SMProperty->GetInformationOnly())
      {
      continue;
      }

    QString regex = QString("^") + QString(iter->GetKey());
    QList<QObject*> foundObjects = widget->findChildren<QObject*>(QRegExp(regex));
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
              pqObjectEditor::PropertyManager.registerLink(le, "text", SIGNAL(textChanged(const QString&)),
                                                 proxy, SMProperty, j);
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
          pqObjectEditor::PropertyManager.registerLink(checkBox, "checked", SIGNAL(toggled(bool)),
                                             proxy, SMProperty);
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
          pqObjectEditor::PropertyManager.registerLink(adaptor, "currentText", SIGNAL(currentTextChanged(const QString&)),
                                             proxy, SMProperty);
          }
        else if(lineEdit)
          {
          pqObjectEditor::PropertyManager.registerLink(lineEdit, "text", SIGNAL(textChanged(const QString&)),
                                             proxy, SMProperty);
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
            pqObjectEditor::PropertyManager.registerLink(item, "checked", SIGNAL(checkedStateChanged(bool)),
                                               proxy, SMProperty, j);
            }
          }
        }
      else if(pt == pqSMAdaptor::PROXY)
        {
        QComboBox* comboBox = qobject_cast<QComboBox*>(foundObject);
        if(comboBox)
          {
          QList<pqSMProxy> propertyDomain = pqSMAdaptor::getProxyPropertyDomain(proxy, SMProperty);
          comboBox->clear();
          foreach(pqSMProxy v, propertyDomain)
            {
            pqPipelineObject* o = pqPipelineData::instance()->getObjectFor(v);
            if(o)
              {
              comboBox->addItem(o->GetProxyName());
              }
            }
          pqSignalAdaptorComboBox* comboAdaptor = new pqSignalAdaptorComboBox(comboBox);
          comboAdaptor->setObjectName("ComboBoxAdaptor");
          pqSignalAdaptorProxy* proxyAdaptor = new pqSignalAdaptorProxy(comboAdaptor, "currentText", 
                                                 SIGNAL(currentTextChanged(const QString&)));
          proxyAdaptor->setObjectName("ComboBoxProxyAdaptor");
          pqObjectEditor::PropertyManager.registerLink(proxyAdaptor, "proxy", SIGNAL(proxyChanged(const QVariant&)),
                                             proxy, SMProperty);
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
          pqObjectEditor::PropertyManager.registerLink(adaptor, "currentText", SIGNAL(currentTextChanged(const QString&)),
                                             proxy, SMProperty);
          }
        else if(lineEdit)
          {
          pqObjectEditor::PropertyManager.registerLink(lineEdit, "text", SIGNAL(textChanged(const QString&)),
                                             proxy, SMProperty);
          }
        else if(slider)
          {
          pqObjectEditor::PropertyManager.registerLink(slider, "value", SIGNAL(valueChanged(int)),
                                             proxy, SMProperty);
          }
        else if(doubleSpinBox)
          {
          pqObjectEditor::PropertyManager.registerLink(doubleSpinBox, "value", SIGNAL(valueChanged(double)),
                                             proxy, SMProperty);
          }
        }
      else if(pt == pqSMAdaptor::FILE_LIST)
        {
        QLineEdit* lineEdit = qobject_cast<QLineEdit*>(foundObject);
        if(lineEdit)
          {
          pqObjectEditor::PropertyManager.registerLink(lineEdit, "text", SIGNAL(textChanged(const QString&)),
                                             proxy, SMProperty);
          }
        }
      }
    }
  iter->Delete();

}

void pqObjectEditor::unlinkServerManagerProperties(pqSMProxy proxy, QWidget* widget)
{
  vtkSMPropertyIterator *iter = proxy->NewPropertyIterator();
  for(iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProperty* SMProperty = iter->GetProperty();
    if(SMProperty->GetInformationOnly())
      {
      continue;
      }

    QObject* foundObject = widget->findChild<QObject*>(iter->GetKey());
    if(!foundObject)
      {
      continue;
      }
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
            pqObjectEditor::PropertyManager.unregisterLink(le, "text", SIGNAL(textChanged(const QString&)),
                                             proxy, SMProperty, j);
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
        pqObjectEditor::PropertyManager.unregisterLink(checkBox, "checked", SIGNAL(toggled(bool)),
                                           proxy, SMProperty);
        }
      else if(comboBox)
        {
        pqSignalAdaptorComboBox* adaptor = comboBox->findChild<pqSignalAdaptorComboBox*>("ComboBoxAdaptor");
        pqObjectEditor::PropertyManager.unregisterLink(adaptor, "currentText", SIGNAL(currentTextChanged(const QString&)),
                                           proxy, SMProperty);
        delete adaptor;
        }
      else if(lineEdit)
        {
        pqObjectEditor::PropertyManager.unregisterLink(lineEdit, "text", SIGNAL(textChanged(const QString&)),
                                           proxy, SMProperty);
        }
      }
    else if(pt == pqSMAdaptor::SELECTION)
      {
      // selections can be list widgets
      QListWidget* listWidget = qobject_cast<QListWidget*>(foundObject);
      if(listWidget)
        {
        for(int i=0; i<listWidget->count(); i++)
          {
          pqListWidgetItemObject* item = static_cast<pqListWidgetItemObject*>(listWidget->item(i));
          pqObjectEditor::PropertyManager.unregisterLink(item, "checked", SIGNAL(checkedStateChanged(bool)),
                                             proxy, SMProperty, i);
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
        pqObjectEditor::PropertyManager.unregisterLink(proxyAdaptor, "proxy", SIGNAL(proxyChanged(const QVariant&)),
                                           proxy, SMProperty);
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
        pqObjectEditor::PropertyManager.unregisterLink(adaptor, "currentText", SIGNAL(currentTextChanged(const QString&)),
                                           proxy, SMProperty);
        delete adaptor;
        }
      else if(lineEdit)
        {
        pqObjectEditor::PropertyManager.unregisterLink(lineEdit, "text", SIGNAL(textChanged(const QString&)),
                                           proxy, SMProperty);
        }
      else if(slider)
        {
        pqObjectEditor::PropertyManager.unregisterLink(slider, "value", SIGNAL(valueChanged(int)),
                                           proxy, SMProperty);
        }
      else if(doubleSpinBox)
        {
        pqObjectEditor::PropertyManager.unregisterLink(doubleSpinBox, "value", SIGNAL(valueChanged(double)),
                                           proxy, SMProperty);
        }
      }
    else if(pt == pqSMAdaptor::FILE_LIST)
      {
      QLineEdit* lineEdit = qobject_cast<QLineEdit*>(foundObject);
      if(lineEdit)
        {
        pqObjectEditor::PropertyManager.unregisterLink(lineEdit, "text", SIGNAL(textChanged(const QString&)),
                                           proxy, SMProperty);
        }
      }
    }
  iter->Delete();
  proxy->UpdateVTKObjects();
}


