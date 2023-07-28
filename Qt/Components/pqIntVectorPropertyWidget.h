// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqIntVectorPropertyWidget_h
#define pqIntVectorPropertyWidget_h

#include "pqPropertyWidget.h"

class vtkSMIntVectorProperty;

class PQCOMPONENTS_EXPORT pqIntVectorPropertyWidget : public pqPropertyWidget
{
  Q_OBJECT

public:
  pqIntVectorPropertyWidget(vtkSMProperty* property, vtkSMProxy* proxy, QWidget* parent = nullptr);
  ~pqIntVectorPropertyWidget() override;

  /**
   * Creates known pqPropertyWidget subclasses for vtkSMIntVectorProperty property.
   */
  static pqPropertyWidget* createWidget(
    vtkSMIntVectorProperty* smproperty, vtkSMProxy* smproxy, QWidget* parent);

private:
  Q_DISABLE_COPY(pqIntVectorPropertyWidget);
};

#endif // pqIntVectorPropertyWidget_h
