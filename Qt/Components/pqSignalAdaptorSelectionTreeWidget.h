/*=========================================================================

   Program: ParaView
   Module:    pqSignalAdaptorSelectionTreeWidget.h

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
#ifndef pqSignalAdaptorSelectionTreeWidget_h
#define pqSignalAdaptorSelectionTreeWidget_h

#include "pqComponentsModule.h"
#include <QList>
#include <QObject>
#include <QVariant>

class QTreeWidget;
class vtkSMProperty;
class QTreeWidgetItem;

/**
* pqSignalAdaptorSelectionTreeWidget has two roles.
* \li It is used to connect a selection property with a
* QTreeWidget (typically pqTreeWidget)
* It also updates the list of available values every time
* the domain changes.
* Example use of this adaptor is to connect :
* \li "AddVolumeArrayName" property of a CTHPart filter to a
*     pqTreeWidget widget.
* \li "Functions" property of "P3DReader" with a pqTreeWidget.
*/

class PQCOMPONENTS_EXPORT pqSignalAdaptorSelectionTreeWidget : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QList<QList<QVariant> > values READ values WRITE setValues)
public:
  /**
  * Constructor.
  * \param treeWidget The QTreeWidget controlled by this adaptor.
  * \param property   The property linked to the widget.
  */
  pqSignalAdaptorSelectionTreeWidget(QTreeWidget* treeWidget, vtkSMProperty* property);

  ~pqSignalAdaptorSelectionTreeWidget() override;

  /**
  * Returns a list of strings which  correspond to the currently
  * selected values. i.e. only the strings for items currently
  * selected by the user are returned.
  */
  QList<QList<QVariant> > values() const;

  /**
  * This adaptor create QTreeWidgetItem instances by default when new
  * entries are to be shown in the widget. To change the type of
  * QTreeWidgetItem subclass created, simply set a function pointer to
  * a callback which will be called every time a new item is needed.
  * The signature for the callback is:
  * QTreeWidgetItem* callback(QTreeWidget* parent, const QStringList& val)
  */
  void setItemCreatorFunction(QTreeWidgetItem*(fptr)(QTreeWidget*, const QStringList&))
  {
    this->ItemCreatorFunctionPtr = fptr;
  }

Q_SIGNALS:
  /**
  * Fired whenever the values change.
  */
  void valuesChanged();

public Q_SLOTS:
  /**
  * Set the selected value on the widget. All the strings in the
  * \param values are set as selected in the tree widget.
  * If a string is present that is not in the domain, it will
  * be ignored.
  */
  void setValues(const QList<QList<QVariant> >& values);

private Q_SLOTS:
  /**
  * Called when vtkSMStringListDomain changes. We update the
  * tree widget to show the user the new set of strings he can choose.
  */
  void domainChanged();

private:
  Q_DISABLE_COPY(pqSignalAdaptorSelectionTreeWidget)

  class pqInternal;
  pqInternal* Internal;

  QTreeWidgetItem* (*ItemCreatorFunctionPtr)(QTreeWidget*, const QStringList&);
};

#endif
