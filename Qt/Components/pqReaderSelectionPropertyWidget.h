// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqReaderSelectionPropertyWidget_h
#define pqReaderSelectionPropertyWidget_h

#include "pqPropertyWidget.h"
#include <QScopedPointer>

class vtkObject;
class vtkPVXMLElement;
class vtkSMPropertyGroup;

/**
 * @class pqReaderSelectionPropertyWidget
 * @brief Used to select readers to show or hide.
 *
 * This widget is used in the settings to show a checklist of available readers,
 * letting the user exclude some of them from the list shown in the File .. Open dialog.
 *
 * @sa pqArraySelectorPropertyWidget
 */
class PQCOMPONENTS_EXPORT pqReaderSelectionPropertyWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;

public:
  pqReaderSelectionPropertyWidget(
    vtkSMProxy* proxy, vtkSMProperty* property, QWidget* parent = nullptr);
  ~pqReaderSelectionPropertyWidget() override;

private Q_SLOTS:
  void readerListChanged();

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqReaderSelectionPropertyWidget);
  class pqInternals;
  friend class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
