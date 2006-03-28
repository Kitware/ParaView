/*=========================================================================

   Program:   ParaQ
   Module:    pqObjectInspectorWidget.cxx

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

#include "pqObjectInspectorWidget.h"

//#include "pqObjectInspector.h"
//#include "pqObjectInspectorDelegate.h"
#include "pqObjectEditor.h"
#include "pqPipelineData.h"
#include "pqPipelineObject.h"
#include "pqPipelineWindow.h"
#include "pqSMAdaptor.h"

#include <QHeaderView>
#include <QTreeView>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QFile>
#include <QFormBuilder>
#include <QScrollArea>
#include <QLineEdit>
#include <QCheckBox>
#include <QSlider>
#include <QListWidget>
#include <QListWidgetItem>

#include <vtkSMProxy.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMProperty.h>
#include <vtkSMPropertyIterator.h>


pqObjectInspectorWidget::pqObjectInspectorWidget(QWidget *p)
  : QWidget(p)
{
#if 0
  this->Inspector = 0;
  this->Delegate = 0;
  this->TreeView = 0;
#endif
  this->TabWidget = 0;

#if 0
  // Create the object inspector model.
  this->Inspector = new pqObjectInspector(this);
  if(this->Inspector)
    this->Inspector->setObjectName("Inspector");

  // Create the delegate to work with the model.
  this->Delegate = new pqObjectInspectorDelegate(this);

  // Create the tree view to display the model.
  this->TreeView = new QTreeView(this);
  if(this->TreeView)
    {
    this->TreeView->setObjectName("InspectorView");
    this->TreeView->setAlternatingRowColors(true);
    this->TreeView->header()->hide();
    this->TreeView->setModel(this->Inspector);
    if(this->Delegate)
      this->TreeView->setItemDelegate(this->Delegate);
    }
#else

  this->ObjectEditor = new pqObjectEditor(this);
  this->ObjectEditor->setObjectName("Object Editor");

#endif

  this->TabWidget = new QTabWidget(this);
  QScrollArea* s = new QScrollArea();
  s->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  s->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
#if 0
  s->setWidgetResizable(true);
#else
  s->setWidgetResizable(false);
#endif
  s->setObjectName("InspectorScrollView");
  QVBoxLayout *boxLayout = new QVBoxLayout(s);
  boxLayout->setMargin(0);

  this->TabWidget->addTab(s, "");
  this->TabWidget->setObjectName("InspectorTabWidget");
  this->TabWidget->hide();

  // Add the tree view to the layout.
  boxLayout = new QVBoxLayout(this);
  if(boxLayout)
    {
    boxLayout->setMargin(0);
#if 0
    boxLayout->addWidget(this->TreeView);
#else
    boxLayout->addWidget(this->ObjectEditor);
#endif
    }
}

pqObjectInspectorWidget::~pqObjectInspectorWidget()
{
#if 0
  if(this->TreeView)
    this->TreeView->setModel(0);
#endif
}

void pqObjectInspectorWidget::setProxy(vtkSMProxy *proxy)
{

  QWidget* customForm = NULL;

  if(proxy)
    {
    QString proxyui = QString(":/pqWidgets/") + QString(proxy->GetXMLName()) + QString(".ui");
    QFile file(proxyui);
    // if we have a custom form, use it
    if(file.open(QFile::ReadOnly))
      {
      QFormBuilder builder;
      customForm = builder.load(&file, NULL);
      file.close();
      }
    }

  // set up layout for with or without custom form
  if(customForm && this->TabWidget->isHidden())
    {
#if 0
    this->layout()->removeWidget(this->TreeView);
    this->TreeView->setParent(NULL);
    this->TabWidget->addTab(this->TreeView, "Advanced");
#else
    this->layout()->removeWidget(this->ObjectEditor);
    this->ObjectEditor->setParent(NULL);
    this->TabWidget->addTab(this->ObjectEditor, "Advanced");
#endif

    this->layout()->addWidget(this->TabWidget);
    QScrollArea* s = qobject_cast<QScrollArea*>(this->TabWidget->widget(0));
    s->setWidget(customForm);
    this->TabWidget->setTabText(0, proxy->GetXMLName());
    this->TabWidget->show();
    }
  else if(customForm && !this->TabWidget->isHidden())
    {
    QScrollArea* s = qobject_cast<QScrollArea*>(this->TabWidget->widget(0));
    QWidget* lastform = s->takeWidget();
    delete lastform;
    s->setWidget(customForm);
    this->TabWidget->setTabText(0, proxy->GetXMLName());
    }
  else if(!customForm && !this->TabWidget->isHidden())
    {
    // we don't have a custom form, make sure we don't show one, if we did previously
    QScrollArea* s = qobject_cast<QScrollArea*>(this->TabWidget->widget(0));
    QWidget* lastform = s->takeWidget();
    delete lastform;
    this->layout()->removeWidget(this->TabWidget);
    this->TabWidget->removeTab(1);
    this->TabWidget->hide();
#if 0
    this->TreeView->setParent(NULL);
    this->layout()->addWidget(this->TreeView);
    this->TreeView->show();
#else
    this->ObjectEditor->setParent(NULL);
    this->layout()->addWidget(this->ObjectEditor);
    this->ObjectEditor->show();
#endif
    }

  if(customForm)
    {
    this->setupCustomForm(proxy, customForm);
    }

#if 0
  if(this->Inspector)
    {
    // remember expanded items
    int count = this->Inspector->rowCount();
    QList<bool> expanded;
    for(int i=0; i<count; i++)
      {
      if(this->TreeView->isExpanded(this->Inspector->index(i,0)))
        expanded.append(true);
      else
        expanded.append(false);
      }

    this->Inspector->setProxy(proxy);
    
    // if less rows than before (was empty), make all expanded
    int newcount = this->Inspector->rowCount();
    for(; count < newcount; count++)
      expanded.append(true);

    for(int i=0; i<newcount; i++)
      {
      this->TreeView->setExpanded(this->Inspector->index(i,0), expanded[i]);
      }
    }
#else
  this->ObjectEditor->setProxy(proxy);
#endif
}

void pqObjectInspectorWidget::setupCustomForm(vtkSMProxy* proxy, QWidget* w)
{
  vtkSMSourceProxy* sp = vtkSMSourceProxy::SafeDownCast(proxy);

  // TODO, in future support more than source proxies
  if(!sp)
    return;

  sp->UpdatePipelineInformation();
  
  pqSMAdaptor *adapter = pqSMAdaptor::instance();
  
  vtkSMPropertyIterator *iter = proxy->NewPropertyIterator();
  for(iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProperty *prop = iter->GetProperty();
    QWidget* widgetProperty = w->findChild<QWidget*>(iter->GetKey());
    if(widgetProperty)
      {
      QLineEdit* le = qobject_cast<QLineEdit*>(widgetProperty);
      if(le)
        {
        adapter->linkPropertyTo(proxy, prop, 0, le, "text");
        adapter->linkPropertyTo(le, "text", SIGNAL(textChanged(const QString&)), proxy, prop, 0);
        this->connect(le, SIGNAL(textChanged(const QString&)), SLOT(updateDisplayForPropertyChanged()), Qt::QueuedConnection);
        }
      QCheckBox* cb = qobject_cast<QCheckBox*>(widgetProperty);
      if(cb)
        {
        adapter->linkPropertyTo(proxy, prop, 0, cb, "checked");
        adapter->linkPropertyTo(cb, "checked", SIGNAL(stateChanged(int)), proxy, prop, 0);
        this->connect(cb, SIGNAL(stateChanged(int)), SLOT(updateDisplayForPropertyChanged()), Qt::QueuedConnection);
        }
      QSlider* sb = qobject_cast<QSlider*>(widgetProperty);
      if(sb)
        {
        // TODO: handle domains changes, during timesteps
        adapter->linkPropertyTo(proxy, prop, 0, sb, "value");
        adapter->linkPropertyTo(sb, "value", SIGNAL(valueChanged(int)), proxy, prop, 0);
        QList<QVariant> range = adapter->getElementPropertyDomain(prop);
        sb->setMinimum(range[0].toInt());
        sb->setMaximum(range[1].toInt());
        this->connect(sb, SIGNAL(valueChanged(int)), SLOT(updateDisplayForPropertyChanged()), Qt::QueuedConnection);
        }
      QListWidget* lw = qobject_cast<QListWidget*>(widgetProperty);
      if(lw)
        {
        QList<QList<QVariant> > items = adapter->getSelectionProperty(proxy, prop);
        for(int i=0; i<items.size(); i++)
          {
          QList<QVariant> item = items[i];
          QString name = item[0].toString();
          pqCustomFormListItem* lwItem = new pqCustomFormListItem(name, lw);
          adapter->linkPropertyTo(proxy, prop, i, lwItem, "value");
          adapter->linkPropertyTo(lwItem, "value", SIGNAL(valueChanged()), proxy, prop, i);
          this->connect(lwItem, SIGNAL(valueChanged()), SLOT(updateDisplayForPropertyChanged()), Qt::QueuedConnection);
          }
        }
      }
    }
  iter->Delete();

}

void pqObjectInspectorWidget::updateDisplayForPropertyChanged()
{
#if 0
  vtkSMProxy* proxy = this->Inspector->proxy();
#else
  vtkSMProxy* proxy = this->ObjectEditor->proxy();
#endif
  if(!proxy)
    {
    return;
    }
  pqPipelineObject* obj = pqPipelineData::instance()->getObjectFor(proxy);
  if(obj)
    {
    QWidget* widget = obj->GetParent()->GetWidget();
    widget->update();
    }
}

