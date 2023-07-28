// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqKeyFrameTimeValidator_h
#define pqKeyFrameTimeValidator_h

#include "pqComponentsModule.h"
#include <QDoubleValidator>

class pqAnimationScene;
class vtkSMDomain;

class PQCOMPONENTS_EXPORT pqKeyFrameTimeValidator : public QDoubleValidator
{
  Q_OBJECT
  typedef QDoubleValidator Superclass;

public:
  pqKeyFrameTimeValidator(QObject* parent);
  ~pqKeyFrameTimeValidator() override;

  // Set the AnimationScene. The ClockTimeRange from the time
  // keeper is used to determine the scale factor for the
  // range for this validator.
  void setAnimationScene(pqAnimationScene* timekeeper);

  // Set the domain which for this key time. Domain provides
  // the normalized range for this validator.
  void setDomain(vtkSMDomain* domain);

protected Q_SLOTS:
  void onDomainModified();

private:
  class pqInternals;
  pqInternals* Internals;
};

#endif
