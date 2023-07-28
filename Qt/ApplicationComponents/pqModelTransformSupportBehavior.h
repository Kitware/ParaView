// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqModelTransformSupportBehavior_h
#define pqModelTransformSupportBehavior_h

#include "pqApplicationComponentsModule.h"
#include "vtkTuple.h"
#include <QObject>

class pqView;
class vtkSMSourceProxy;

/**
 * @ingroup Behaviors
 * pqModelTransformSupportBehavior is designed for supporting ChangeOfBasis
 * matrix for MantId.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqModelTransformSupportBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqModelTransformSupportBehavior(QObject* parent = nullptr);
  ~pqModelTransformSupportBehavior() override;

  static vtkTuple<double, 16> getChangeOfBasisMatrix(
    vtkSMSourceProxy*, int outputPort = 0, bool* isvalid = nullptr);
  static vtkTuple<double, 6> getBoundingBoxInModelCoordinates(
    vtkSMSourceProxy*, int outputPort = 0, bool* isvalid = nullptr);
  static vtkTuple<std::string, 3> getAxisTitles(
    vtkSMSourceProxy*, int outputPort = 0, bool* isvalid = nullptr);

protected Q_SLOTS:
  virtual void viewAdded(pqView*);
  virtual void viewUpdated();

protected: // NOLINT(readability-redundant-access-specifiers)
  virtual void enableModelTransform(pqView*, vtkSMSourceProxy*);
  virtual void disableModelTransform(pqView*);

private:
  Q_DISABLE_COPY(pqModelTransformSupportBehavior)
};

#endif
