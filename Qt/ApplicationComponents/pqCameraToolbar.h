// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCameraToolbar_h
#define pqCameraToolbar_h

#include "pqApplicationComponentsModule.h"
#include <QToolBar>

class pqPipelineSource;
class pqDataRepresentation;

/**
 * pqCameraToolbar is the toolbar that has icons for resetting camera
 * orientations as well as zoom-to-data and zoom-to-box.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqCameraToolbar : public QToolBar
{
  Q_OBJECT
  typedef QToolBar Superclass;

public:
  pqCameraToolbar(const QString& title, QWidget* parentObject = nullptr)
    : Superclass(title, parentObject)
  {
    this->constructor();
  }
  pqCameraToolbar(QWidget* parentObject = nullptr)
    : Superclass(parentObject)
  {
    this->constructor();
  }

private Q_SLOTS:

  void onSourceChanged(pqPipelineSource*);
  void updateEnabledState();
  void onRepresentationChanged(pqDataRepresentation*);

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqCameraToolbar)
  void constructor();
  QAction* ZoomToDataAction;
  QAction* ZoomClosestToDataAction;

  // Currently bound connection to an active source, used to
  // disable ZoomToData actions if the current source
  // is not visible
  QMetaObject::Connection SourceVisibilityChangedConnection;
  // Currently bound connection to an active representation, used to
  // disable ZoomToData actions if the current representation
  // is not visible
  QMetaObject::Connection RepresentationDataUpdatedConnection;

  /**
   * A source is visible if its representation is visible
   * and at least one of its block is visible in case of a
   * composite dataset.
   */
  bool isRepresentationVisible(pqDataRepresentation* repr);
};

#endif
