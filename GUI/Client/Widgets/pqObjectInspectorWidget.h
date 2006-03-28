/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

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

/// \file pqObjectInspectorWidget.h
/// \brief
///   The pqObjectInspectorWidget class is used to display the properties
///   of an object in an editable form.
///
/// \date 11/25/2005

#ifndef _pqObjectInspectorWidget_h
#define _pqObjectInspectorWidget_h

#include "QtWidgetsExport.h"
#include <QWidget>
#include <QListWidgetItem>

//class pqObjectInspector;
//class pqObjectInspectorDelegate;
//class QTreeView;
class pqObjectEditor;
class QTabWidget;
class vtkSMProxy;
class QListWidgetItem;


/// \class pqObjectInspectorWidget
/// \brief
///   The pqObjectInspectorWidget class is used to display the properties
///   of an object in an editable form.
class QTWIDGETS_EXPORT pqObjectInspectorWidget : public QWidget
{
  Q_OBJECT
public:
  pqObjectInspectorWidget(QWidget *parent=0);
  virtual ~pqObjectInspectorWidget();

  //pqObjectInspector *getObjectModel() const {return this->Inspector;}
  //QTreeView *getTreeView() const {return this->TreeView;}

public slots:
  void setProxy(vtkSMProxy *proxy);


protected:
  void setupCustomForm(vtkSMProxy*, QWidget* w);

protected slots:
  // slot to help with custom forms
  void updateDisplayForPropertyChanged();

private:
  //pqObjectInspector *Inspector;
  //pqObjectInspectorDelegate *Delegate;
  //QTreeView *TreeView;
  pqObjectEditor* ObjectEditor;
  QTabWidget* TabWidget;
};


// internal class only  -- TODO support more than check states?
class pqCustomFormListItem : public QObject, public QListWidgetItem
{
  Q_OBJECT
  Q_PROPERTY(QVariant value READ value WRITE setValue)
public:
  pqCustomFormListItem(const QString& t, QListWidget* p)
    : QListWidgetItem(t, p), Value(QVariant::Invalid) {}

  void setData(int role, const QVariant& v)
    {
    QListWidgetItem::setData(role, v);
    if(Qt::CheckStateRole == role)
      {
      if(Qt::Checked == v)
        {
        this->Value = true;
        }
      else if(Qt::Unchecked == v)
        {
        this->Value = false;
        }
      emit this->valueChanged();
      }
    }
  QVariant value() const
    {
    return this->Value;
    }
  void setValue(const QVariant& v)
    {
    // only set if it is different then what we have
    if((v.toBool() == true && this->checkState() == Qt::Checked ||
       v.toBool() == false && this->checkState() == Qt::Unchecked) &&
      Value.type() != QVariant::Invalid)
      {
      return;
      }
    if(v.toBool())
      {
      this->setCheckState(Qt::Checked);
      }
    else
      {
      this->setCheckState(Qt::Unchecked);
      }
    }
signals:
  void valueChanged();

private:
  QVariant Value;

};

#endif
