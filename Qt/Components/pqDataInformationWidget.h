// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqDataInformationWidget_h
#define pqDataInformationWidget_h

#include "pqComponentsModule.h"
#include <QWidget>

class pqDataInformationModel;
class QTableView;

/**
 * Widget for the DataInformation(or Statistics View).
 * It creates the model and the view and connects them.
 */
class PQCOMPONENTS_EXPORT pqDataInformationWidget : public QWidget
{
  Q_OBJECT
public:
  pqDataInformationWidget(QWidget* parent = nullptr);
  ~pqDataInformationWidget() override;

protected:
  /**
   * Filters events received by the View.
   */
  bool eventFilter(QObject* object, QEvent* event) override;

private Q_SLOTS:
  void showHeaderContextMenu(const QPoint&);
  void showBodyContextMenu(const QPoint&);

private: // NOLINT(readability-redundant-access-specifiers)
  pqDataInformationModel* Model;
  QTableView* View;
};

#endif
