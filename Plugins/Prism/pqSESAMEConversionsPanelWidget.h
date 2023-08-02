// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   pqSESAMEConversionsPanelWidget
 * @brief   Widget for SESAME conversions.
 *
 * This widget is used to set the conversions values and names for the SESAME reader.
 */
#ifndef pqSESAMEConversionsPanelWidget_h
#define pqSESAMEConversionsPanelWidget_h

#include "pqPropertyWidget.h"

class QItemSelection;

class pqSESAMEConversionsPanelWidget : public pqPropertyWidget
{
private:
  Q_OBJECT
  typedef pqPropertyWidget Superclass;

public:
  pqSESAMEConversionsPanelWidget(
    vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject = nullptr);
  ~pqSESAMEConversionsPanelWidget() override;

  QVector<QPair<QString, double>> getConversionOptions() const;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void onConversionVariableChanged(int index);
private Q_SLOTS:
  void onRestoreDefaultConversionsFile();
  void onConversionFileChanged();
  void onTableIdChanged(const QString&);
  void onSESAME();
  void onSI();
  void onCGS();
  void onCustom();
  void onTableChanged(const QModelIndex&, const QModelIndex&);

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqSESAMEConversionsPanelWidget);

  class pqUI;
  pqUI* UI;
};

#endif // pqSESAMEConversionsPanelWidget_h
