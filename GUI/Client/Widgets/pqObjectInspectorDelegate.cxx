/*=========================================================================

   Program:   ParaQ
   Module:    pqObjectInspectorDelegate.cxx

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

#include "pqObjectInspectorDelegate.h"


#include <QAbstractItemModel>
#include <QComboBox>
#include <QLineEdit>
#include <QList>
#include <QModelIndex>
#include <QSpinBox>
#include <QString>
#include <QStringList>
#include <QVariant>

#include "pqObjectInspector.h"
#include "pqSMProxy.h"
#include "pqPipelineData.h"
#include "pqPipelineObject.h"

pqObjectInspectorDelegate::pqObjectInspectorDelegate(QObject *p)
  : QItemDelegate(p)
{
}

pqObjectInspectorDelegate::~pqObjectInspectorDelegate()
{
}

QWidget *pqObjectInspectorDelegate::createEditor(QWidget *p,
    const QStyleOptionViewItem &, const QModelIndex &index) const
{
  if(!index.isValid())
    return 0;

  // Make sure the model is the correct type.
  const pqObjectInspector *model = qobject_cast<const pqObjectInspector *>(
      index.model());
  if(!model)
    return 0;

  // Get the data for the given index. Use the data type and domain
  // information to create the correct widget type.
  QWidget *editor = 0;
  QVariant data = model->data(index, Qt::EditRole);
  QVariant domain = model->domain(index);
  if(data.type() == QVariant::Bool)
    {
    // Use a combo box with true/false in it.
    QComboBox *combo = new QComboBox(p);
    combo->addItem("false");
    combo->addItem("true");
    editor = combo;
    }
  else if(data.type() == QVariant::Int)
    {
    // Use a spin box for an int with a min and max. Use a combo
    // box for an int with enum names or values. Otherwise, use
    // a line edit.
    if(domain.type() == QVariant::List)
      {
      QList<QVariant> list = domain.toList();
      if(list.size() == 2 && list[0].isValid() && list[1].isValid())
        {
        QSpinBox *spin = new QSpinBox(p);
        spin->setMinimum(list[0].toInt());
        spin->setMaximum(list[1].toInt());
        editor = spin;
        }
      }
    else if(domain.type() == QVariant::StringList)
      {
      QStringList names = domain.toStringList();
      QComboBox *combo = new QComboBox(p);
      for(QStringList::Iterator it = names.begin(); it != names.end(); ++it)
        combo->addItem(*it);
      editor = combo;
      }
    }
  else if(data.type() == QVariant::String && 
         (domain.canConvert(QVariant::StringList)))
    {
    QStringList names = domain.toStringList();
    QComboBox *combo = new QComboBox(p);
    for(QStringList::Iterator it = names.begin(); it != names.end(); ++it)
      combo->addItem(*it);
    editor = combo;
    }
  else if(data.value<pqSMProxy>())
    {
    QList<QVariant> proxies = domain.toList();
    QComboBox *combo = new QComboBox(p);
    foreach(QVariant v, proxies)
      {
      pqSMProxy proxy = v.value<pqSMProxy>();
      pqPipelineObject* o = pqPipelineData::instance()->getObjectFor(proxy);
      if(o)
        {
        combo->addItem(o->GetProxyName());
        }
      else
        {
        combo->addItem("No Name");
        }
      }
    editor = combo;
    }

  // Use a line edit for the default case.
  if(!editor)
    editor = new QLineEdit(p);

  if(editor)
    editor->installEventFilter(const_cast<pqObjectInspectorDelegate *>(this));
  return editor;
        
}

void pqObjectInspectorDelegate::setEditorData(QWidget *editor,
    const QModelIndex &index) const
{
  QVariant data = index.model()->data(index, Qt::EditRole);
  QComboBox *combo = qobject_cast<QComboBox *>(editor);
  if(combo)
    {
    if(data.type() == QVariant::Bool)
      {
      combo->setCurrentIndex(data.toBool() ? 1 : 0);
      }
    else if(data.type() == QVariant::Int)
      {
      combo->setCurrentIndex(data.toInt());
      }
    else if(data.type() == QVariant::String)
      {
      combo->setCurrentIndex(combo->findText(data.toString()));
      }
    else if(data.value<pqSMProxy>())
      {
      pqPipelineObject* o = pqPipelineData::instance()->getObjectFor(data.value<pqSMProxy>());
      if(o)
        {
        combo->setCurrentIndex(combo->findText(o->GetProxyName()));
        }
      }
    return;
    }

  QSpinBox *spin = qobject_cast<QSpinBox *>(editor);
  if(spin)
    {
    spin->setValue(data.toInt());
    return;
    }

  QLineEdit *line = qobject_cast<QLineEdit *>(editor);
  if(line)
    {
    line->setText(data.toString());
    return;
    }
}

void pqObjectInspectorDelegate::setModelData(QWidget *editor,
    QAbstractItemModel *model, const QModelIndex &index) const
{
  const pqObjectInspector *realmodel = qobject_cast<const pqObjectInspector *>(
      index.model());

  QVariant value;
  QVariant data = model->data(index, Qt::EditRole);
  QComboBox *combo = qobject_cast<QComboBox *>(editor);
  if(combo)
    {
    if(data.type() == QVariant::String)
      {
      value = combo->currentText();
      }
    else if(data.value<pqSMProxy>())
      {
      QList<QVariant> domain = realmodel->domain(index).toList();
      int currentIndex = combo->currentIndex();
      value = domain[currentIndex];
      }
    else
      {
      value = combo->currentIndex();
      }
    model->setData(index, value);
    return;
    }

  QSpinBox *spin = qobject_cast<QSpinBox *>(editor);
  if(spin)
    {
    value = spin->value();
    model->setData(index, value);
    return;
    }

  QLineEdit *line = qobject_cast<QLineEdit *>(editor);
  if(line)
    {
    value = line->text();
    model->setData(index, value);
    return;
    }
}


