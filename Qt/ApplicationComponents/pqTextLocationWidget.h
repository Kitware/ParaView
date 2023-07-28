// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqTextLocationWidget_h
#define pqTextLocationWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidget.h"

class vtkSMPropertyGroup;

/**
 * pqTextLocationWidget is a pqPropertyWidget that can be used to set
 * the location of the a text representation relative to the viewport.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqTextLocationWidget : public pqPropertyWidget
{
  Q_OBJECT
  Q_PROPERTY(QString windowLocation READ windowLocation WRITE setWindowLocation)

  typedef pqPropertyWidget Superclass;

public:
  pqTextLocationWidget(vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent = nullptr);
  ~pqTextLocationWidget() override;

  QString windowLocation() const;

Q_SIGNALS:
  void windowLocationChanged(QString&);

protected:
  void setWindowLocation(QString&);

protected Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void radioButtonLocationClicked();
  void radioButtonPositionClicked();
  void updateUI();

private:
  Q_DISABLE_COPY(pqTextLocationWidget)
  class pqInternals;
  pqInternals* Internals;
};

#endif // pqTextLocationWidget_h
