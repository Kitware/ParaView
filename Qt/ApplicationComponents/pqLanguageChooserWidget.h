// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqLanguageChooserWidget_h
#define pqLanguageChooserWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidget.h"
#include <QPointer>

class QComboBox;

/**
 * pqLanguageChooserWidget is a property widget that shows a combo-box with
 * values equal to the currently available translation languages.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqLanguageChooserWidget : public pqPropertyWidget
{
  Q_OBJECT;
  typedef pqPropertyWidget Superclass;
  Q_PROPERTY(QString value READ value WRITE setValue);

public:
  pqLanguageChooserWidget(vtkSMProxy* proxy, vtkSMProperty* smproperty, QWidget* parent = nullptr);
  ~pqLanguageChooserWidget() override;

  /**
   * get the current selected locale.
   */
  QString value() const;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * set the default value for the widget among the current items. If the value
   * is not in the items, add it, make it default and re-sort the list alphabetically.
   */
  void setValue(QString& value);

Q_SIGNALS:
  void valueChanged();

private:
  Q_DISABLE_COPY(pqLanguageChooserWidget)
  QPointer<QComboBox> ComboBox;
};

#endif
