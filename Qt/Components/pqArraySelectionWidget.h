/*=========================================================================

   Program: ParaView
   Module:  pqArraySelectionWidget.h

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
#ifndef pqArraySelectionWidget_h
#define pqArraySelectionWidget_h

#include "pqComponentsModule.h" // for exports
#include "pqTreeView.h"
#include <QMap>     // for QMap.
#include <QPointer> // for QPointer

class pqTimer;

/**
 * @class pqArraySelectionWidget
 * @brief pqArraySelectionWidget is a widget used to allow user to select arrays.
 *
 * pqArraySelectionWidget shows a list of items to select (or check) using
 * check boxes next to each of the items.
 *
 * @section ImplementationDetails Implementation Details
 *
 * pqArraySelectionWidget uses pqTreeView as the view and internally build a QAbstractItemModel
 * subclass to track the state.
 *
 * For several readers, e.g. ExodusIIReader, there could be multiple properties that
 * control array status. We want a single widget to show and control the selection states for
 * all those properties. To support that, we use dynamic properties. Simply hook up the same
 * pqArraySelectionWidget with multiple properties via pqPropertyLinks, using any name of the
 * Qt property name when setting up the connection.
 *
 * @section LegacyInformation Legacy Information
 *
 * pqArraySelectionWidget is based on a class known as pqExodusIIVariableSelectionWidget.
 * pqExodusIIVariableSelectionWidget, however was a pqTreeWidget subclass. This, however,
 * uses pqTreeView which is the recommended approach since it provides a better user experience
 * when dealing with large lists.
 *
 * @section ImprovingUserExperience Improving user experience
 *
 * To help the user with checking multiple items, filtering, and sorting, you
 * may want to connect pqArraySelectionWidget with a pqTreeViewSelectionHelper.
 *
 * @section AdditionalColumns Additional columns
 *
 * In certain case, it's helpful to show additional columns that provide
 * additional information about each of the row e.g. for exodus files, we may
 * want to show exodus block ids. Such columns can be added by using
 * `setColumnData` to provide a map with the mapping. In that case, the
 * pqArraySelectionWidget must be created by passing the target column count in
 * the constructor.
 */
class PQCOMPONENTS_EXPORT pqArraySelectionWidget : public pqTreeView
{
  Q_OBJECT
  typedef pqTreeView Superclass;

public:
  pqArraySelectionWidget(int numColumns, QWidget* parent = nullptr);
  pqArraySelectionWidget(QWidget* parent = nullptr);
  ~pqArraySelectionWidget() override;

  /**
   * overridden to handle QDynamicPropertyChangeEvent events.
   */
  bool event(QEvent* e) override;

  ///@{
  /**
   * Specify a label to use for the horizontal header for the view.
   */
  void setHeaderLabel(const QString& label) { this->setHeaderLabel(0, label); }
  void setHeaderLabel(int column, const QString& label);
  QString headerLabel(int column = 0) const;
  ///@}

  ///@{
  /**
   * Specify an icon type to use for a property with the given name.
   * Supported icon types are point, cell, field, vertex, edge, row,
   * face, side-set, node-set, face-set, edge-set, cell-set,
   * node-map, edge-map, face-map.
   */
  void setIconType(const QString& pname, const QString& icon_type);
  const QString& iconType(const QString& pname) const;
  ///@}

  ///@{
  /**
   * Add additional column with meta-data for each row.
   */
  void setColumnData(int column, const QString& pname, QMap<QString, QString>&& mapping);
  ///@}

  /**
   * API to add custom item data for each column.
   */
  void setColumnItemData(int column, int role, const QVariant& data);

Q_SIGNALS:
  /**
   * fired whenever the check state has been modified.
   */
  void widgetModified();

private:
  Q_DISABLE_COPY(pqArraySelectionWidget);

  /**
   * called in `event()` to handle change in a dynamic property with the given name.
   */
  void propertyChanged(const QString& pname);

  void updateProperty(const QString& pname, const QVariant& value);

  /**
   * Eventually resize sections.
   */
  void resizeSectionsEventually();

  bool UpdatingProperty;
  QPointer<pqTimer> Timer;

  class Model;
  friend class Model;

  Model* realModel() const;
};

#endif
