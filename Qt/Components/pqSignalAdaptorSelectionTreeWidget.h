/*=========================================================================

   Program: ParaView
   Module:    pqSignalAdaptorSelectionTreeWidget.h

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
#ifndef __pqSignalAdaptorSelectionTreeWidget_h
#define __pqSignalAdaptorSelectionTreeWidget_h


#include <QObject>
#include <QList>
#include <QVariant>
#include "pqComponentsExport.h"

class QTreeWidget;
class vtkSMEnumerationDomain;
class vtkSMStringListDomain;

/// pqSignalAdaptorSelectionTreeWidget has two roles.
/// \li It can be used to connect a vtkSMStringVectorProperty with a
/// QTreeWidget (typically pqSelectionTreeWidget) where the SMProperty
/// must have the values of only the selected items in the tree widget.
/// Alternatively, it can be used to connect a vtkSMIntVectorProperty
/// having a vtkSMEnumerationDomain with a QTreeWidget 
/// (typically pqSelectionTreeWidget). Here too the vtkSMIntVectorProperty
/// is an expandable property which is filled with values for all
/// the selected items.
/// \li Since the SMProperty contanis only the selected item, this adaptor
/// requires a vtkSMStringListDomain (or vtkSMEnumerationDomain) from which 
/// it can get the list of possible values. 
/// It also updates the list of available values every time
/// the domain changes.
/// Example use of this adaptor is to connect :
/// \li "AddVolumeArrayName" property of a CTHPart filter to a 
///     pqSelectionTreeWidget widget.
/// \li "Functions" property of "P3DReader" with a pqSelectionTreeWidget.


class PQCOMPONENTS_EXPORT pqSignalAdaptorSelectionTreeWidget : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QList<QVariant> values READ values WRITE setValues)
  Q_PROPERTY(QList<QVariant> DUMMY READ values WRITE setValues)
public:
  /// Constructor.
  /// \param domain The StringListDomain from which the adaptor
  ///        can obtain the list of possible values for this widget.
  /// \param treeWidget The QTreeWidget controlled by this adaptor.
  pqSignalAdaptorSelectionTreeWidget(vtkSMStringListDomain* domain,
    QTreeWidget* treeWidget);

  /// Constructor.
  /// \param domain The EnumerationDomain from which the adaptor
  ///        can obtain the list of possible values for this widget.
  /// \param treeWidget The QTreeWidget controlled by this adaptor.
  pqSignalAdaptorSelectionTreeWidget(vtkSMEnumerationDomain* domain,
    QTreeWidget* treeWidget);

  virtual ~pqSignalAdaptorSelectionTreeWidget();

  /// Returns a list of strings which  correspond to the currently
  /// selected values. i.e. only the strings for items currently
  /// selected by the user are returned.
  QList<QVariant> values() const;

signals:
  /// Fired whenever the values change.
  void valuesChanged();

public slots:
  /// Set the selected value on the widget. All the strings in the 
  /// \values are set as selected in the tree widget.
  /// If a string is present that is not in the domain, it will
  /// be ignored.
  void setValues(const QList<QVariant>& values);

private slots:
  /// Called when vtkSMStringListDomain changes. We update the
  /// tree widget to show the user the new set of strings he can choose.
  void domainChanged();

private:
  pqSignalAdaptorSelectionTreeWidget(const pqSignalAdaptorSelectionTreeWidget&); // Not implemented.
  void operator=(const pqSignalAdaptorSelectionTreeWidget&); // Not implemented.

  class pqInternal;
  pqInternal* Internal;
};

#endif

