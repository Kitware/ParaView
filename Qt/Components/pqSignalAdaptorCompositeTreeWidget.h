/*=========================================================================

   Program: ParaView
   Module:    pqSignalAdaptorCompositeTreeWidget.h

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

========================================================================*/
#ifndef __pqSignalAdaptorCompositeTreeWidget_h 
#define __pqSignalAdaptorCompositeTreeWidget_h

#include <QObject>
#include <QVariant>
#include "pqComponentsExport.h"


class pqTreeWidgetItemObject;
class QTreeWidget;
class vtkPVDataInformation;
class vtkSMIntVectorProperty;

/// pqSignalAdaptorCompositeTreeWidget is used to connect a property with
/// vtkSMCompositeTreeDomain as its domain to a Tree widget. It updates the tree
/// to show composite data tree.
class PQCOMPONENTS_EXPORT pqSignalAdaptorCompositeTreeWidget : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

  Q_PROPERTY(QList<QVariant> values READ values WRITE setValues)
public:
  /// Constructor. \c domain is used to build the tree layout to show in the
  /// widget.
  pqSignalAdaptorCompositeTreeWidget(QTreeWidget*, vtkSMIntVectorProperty* smproperty);
  ~pqSignalAdaptorCompositeTreeWidget();

  /// Returns the current value.
  /// This is a QList of unsigned ints.
  QList<QVariant> values() const;

public slots:
  /// Set the values.
  void setValues(const QList<QVariant>& values);

  /// Called when domain changes.
  void domainChanged();

signals:
  /// Fired when the widget value changes.
  void valuesChanged();

private slots:
  void updateCheckState(bool checked);
  
private:
  pqSignalAdaptorCompositeTreeWidget(const pqSignalAdaptorCompositeTreeWidget&); // Not implemented.
  void operator=(const pqSignalAdaptorCompositeTreeWidget&); // Not implemented.


  void buildTree(pqTreeWidgetItemObject* item, 
    vtkPVDataInformation* info);

  class pqInternal;
  pqInternal* Internal;

  int InUpdateCheckState;

  enum IndexModes
    {
    INDEX_MODE_FLAT,
    INDEX_MODE_LEVEL_INDEX, // this mode works only for 1 level deep trees.
    INDEX_MODE_LEVEL,       // this mode works only for 1 level deep trees. 
    };

  enum MetaData
    {
    FLAT_INDEX = Qt::UserRole,
    LEVEL_NUMBER = Qt::UserRole+1,
    DATASET_INDEX = Qt::UserRole+2,
    NODE_TYPE = Qt::UserRole+3,
    };

  enum NodeTypes
    {
    LEAF = 21,
    NON_LEAF = 22,
    };
  IndexModes IndexMode;

  // These are used by buildTree() to determin indices for the nodes.
  unsigned int FlatIndex;
  unsigned int LevelNo;
};

#endif


