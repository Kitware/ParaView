// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqServerManagerModelItem_h
#define pqServerManagerModelItem_h

#include "pqCoreModule.h"
#include <QObject>

class vtkEventQtSlotConnect;

/**
 * pqServerManagerModelItem is a element maintained by pqServerManagerModel.
 * pqServerManagerModel creates instances of pqServerManagerModelItem (and its
 * subclasses) for every signification Server-Manager object such as a session
 * (pqServer), source proxy (pqPipelineSource), filter proxy
 * (pqPipelineFilter), view proxy (pqView) and so on.
 */
class PQCORE_EXPORT pqServerManagerModelItem : public QObject
{
  Q_OBJECT

public:
  pqServerManagerModelItem(QObject* parent = nullptr);
  ~pqServerManagerModelItem() override;

protected:
  /**
   * All subclasses generally need some vtkEventQtSlotConnect instance to
   * connect to VTK events. This provides access to a vtkEventQtSlotConnect
   * instance which all subclasses can use for listening to events.
   */
  vtkEventQtSlotConnect* getConnector();

private:
  vtkEventQtSlotConnect* Connector;
};

#endif
