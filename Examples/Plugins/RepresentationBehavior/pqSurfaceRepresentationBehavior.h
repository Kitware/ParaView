// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSurfaceRepresentationBehavior_h
#define pqSurfaceRepresentationBehavior_h

#include <QObject>

class pqRepresentation;
class pqView;

/// @ingroup Behaviors
/// pqSurfaceRepresentationBehavior ensures that any created representation
/// switch to Surface rendering if available and chose a given data array.
class pqSurfaceRepresentationBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqSurfaceRepresentationBehavior(QObject* parent = nullptr);

protected Q_SLOTS:
  void onRepresentationAdded(pqRepresentation*);
  void onViewAdded(pqView*);

private:
  Q_DISABLE_COPY(pqSurfaceRepresentationBehavior)
};

#endif
