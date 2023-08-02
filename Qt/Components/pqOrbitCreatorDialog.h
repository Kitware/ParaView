// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqOrbitCreatorDialog_h
#define pqOrbitCreatorDialog_h

#include "pqComponentsModule.h"
#include <QDialog>
#include <QList>
#include <QVariant>

/**
 * pqOrbitCreatorDialog is used by pqAnimationViewWidget to request the orbit
 * parameters from the user when the user want to create a camera animation track
 * that orbits some object(s). It's a simple dialog with a bunch of entries for
 * normal/center/radius of the orbit.
 */
class PQCOMPONENTS_EXPORT pqOrbitCreatorDialog : public QDialog
{
  Q_OBJECT
  typedef QDialog Superclass;

public:
  pqOrbitCreatorDialog(QWidget* parent = nullptr);
  ~pqOrbitCreatorDialog() override;

  /**
   * Returns the points the orbit based on the user chosen options.
   */
  QList<QVariant> orbitPoints(int resolution) const;

  /**
   * Returns the normal of the orbit.
   */
  QVector3D normal() const;

  void setNormal(double xyz[3]);
  void setCenter(double xyz[3]);
  void setOrigin(double xyz[3]);

protected Q_SLOTS:
  void resetCenter();

private:
  Q_DISABLE_COPY(pqOrbitCreatorDialog)

  class pqInternals;
  pqInternals* Internals;
};

#endif
