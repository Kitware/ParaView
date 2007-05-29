/*=========================================================================

   Program: ParaView
   Module:    pqSignalAdaptorTreeWidget.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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
#ifndef __pqSignalAdaptorTreeWidget_h
#define __pqSignalAdaptorTreeWidget_h

#include <QObject>
#include <QList>
#include <QVariant>
#include "pqComponentsExport.h"

class QTreeWidget;
class pqTreeWidgetItemObject;

/// pqSignalAdaptorTreeWidget can be used to connect any property with 
/// repeat_command to a tree widget that displays the property value.
/// The TreeWidget must have exactly as many columns as the number of
/// elements in each command for the property 
/// (i.e. number_of_element_per_command).
/// Note that the adaptor does not force the repeat command or 
/// size requirements mentioned above. 
class PQCOMPONENTS_EXPORT pqSignalAdaptorTreeWidget : public QObject
{
  Q_OBJECT;
  Q_PROPERTY(QList<QVariant> values READ values WRITE setValues);

public:
  /// Constructor.
  /// \param treeWidget is the tree widget we are connecting.
  /// \param editable indicates if items in the widget can be edited by 
  /// the user.
  pqSignalAdaptorTreeWidget(QTreeWidget* treeWidget, bool editable);
  virtual ~pqSignalAdaptorTreeWidget();

  /// Returns a list of the values currently in the tree widget.
  QList<QVariant> values() const;

  /// Append an item to the tree.
  /// The size of values == this->TreeWidget->columnCount().
  /// Returns the newly created item, or 0 on failure.
  pqTreeWidgetItemObject* appendValue(const QList<QVariant>& values);
  
  /// Append an item to the tree.
  void appendItem(pqTreeWidgetItemObject* item);

signals:
  /// Fired when the tree widget is modified.
  void valuesChanged();

public slots:
  /// Set the values in the widget.
  void setValues(const QList<QVariant>&);

  /// Since the tree widget does not fire any signals
  /// when items are removed, if anyone removed elements from
  /// the tree widget, it should call this method.
  void itemsDeleted()
    { emit this->valuesChanged(); }

private:
  pqSignalAdaptorTreeWidget(const pqSignalAdaptorTreeWidget&); // Not implemented.
  void operator=(const pqSignalAdaptorTreeWidget&); // Not implemented.

  QTreeWidget* TreeWidget;
  bool Editable;
};

#endif

