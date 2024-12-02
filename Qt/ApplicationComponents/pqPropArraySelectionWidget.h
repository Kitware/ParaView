// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqPropArraySelectionWidget_h
#define pqPropArraySelectionWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidget.h"

/**
 *
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqPropArraySelectionWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;

public:
  pqPropArraySelectionWidget(
    vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent = nullptr);
  ~pqPropArraySelectionWidget() override;

  /**
   *
   */
  bool event(QEvent* e) override;

Q_SIGNALS:
  void widgetModified();

private:
  Q_DISABLE_COPY(pqPropArraySelectionWidget);

  /**
   *
   */
  void propertyChanged(const char* pname);

  /**
   *
   */
  void updateProperties();

  /**
   *
   */
  void pointDomainChanged();

  void cellDomainChanged();

  class pqInternals;
  pqInternals* Internals;
};

#endif
