// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqPythonQtMethodHelpers_h
#define pqPythonQtMethodHelpers_h

#include "pqProxy.h"
#include "pqServerManagerModel.h"

class pqPythonQtMethodHelpers : public QObject
{
  Q_OBJECT

public:
  static pqProxy* findProxyItem(pqServerManagerModel* model, vtkSMProxy* proxy)
  {
    return model->findItem<pqProxy*>(proxy);
  }
};

#endif
