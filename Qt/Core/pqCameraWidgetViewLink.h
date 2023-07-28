// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCameraWidgetViewLink_h
#define pqCameraWidgetViewLink_h

#include "pqCoreModule.h"
#include <QObject>

#include <memory> // for unique_ptr

class pqRenderView;
class vtkPVXMLElement;

/**
 * pqCameraWidgetViewLink is used by pqLinksModel to create
 * camera widget view links, which are ViewLink without property
 * synchronization. This allows render views to call render when
 * interacting with the camera widget or the camera itself.
 */
class PQCORE_EXPORT pqCameraWidgetViewLink : public QObject
{
  Q_OBJECT;
  typedef QObject Superclass;

public:
  pqCameraWidgetViewLink(pqRenderView* displayView, pqRenderView* linkedView);
  ~pqCameraWidgetViewLink() override;

  // Save this camera widget view link in xml node
  virtual void saveXMLState(vtkPVXMLElement* xml);

private:
  struct pqInternal;
  std::unique_ptr<pqInternal> Internal;
};

#endif
