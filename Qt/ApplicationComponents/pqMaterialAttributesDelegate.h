// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqMaterialAttributesDelegate_h
#define pqMaterialAttributesDelegate_h

#include "pqApplicationComponentsModule.h"
#include <QStyledItemDelegate>

/**
 * pqMaterialAttributesDelegate is used to customize material attributes table view.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqMaterialAttributesDelegate : public QStyledItemDelegate
{
  Q_OBJECT
  typedef QStyledItemDelegate Superclass;

public:
  pqMaterialAttributesDelegate(QObject* parent = nullptr);
  ~pqMaterialAttributesDelegate() override = default;

  void paint(
    QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

  /**
   * Create the editor with two columns : one for the property selection and one to modify the
   * value of the property.
   */
  QWidget* createEditor(
    QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

  /**
   * do nothing, everything is handled in createEditor method
   */
  void setEditorData(QWidget* editor, const QModelIndex& index) const override
  {
    Q_UNUSED(editor);
    Q_UNUSED(index);
  };

  /**
   * Gets data from the editor widget and stores it in the specified model at the item index
   */
  void setModelData(
    QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;

protected:
  /**
   * Generate a custom dialog with appropriate widget from the QVariantList.
   */
  QWidget* createPropertiesEditor(QVariantList list, QWidget* parent) const;

  /**
   * From the properties editor, create a QVariantList containing each value from widgets.
   */
  QVariantList getPropertiesFromEditor(QWidget* editor) const;

private:
  Q_DISABLE_COPY(pqMaterialAttributesDelegate)
};

#endif
