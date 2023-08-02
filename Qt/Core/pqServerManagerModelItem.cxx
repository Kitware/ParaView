// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

/// \file pqServerManagerModelItem.cxx
/// \date 4/14/2006

#include "pqServerManagerModelItem.h"

// ParaView includes.
#include "vtkEventQtSlotConnect.h"

// Qt includes.

//-----------------------------------------------------------------------------
pqServerManagerModelItem::pqServerManagerModelItem(QObject* _parent /*=nullptr*/)
  : QObject(_parent)
{
  this->Connector = nullptr;
}

//-----------------------------------------------------------------------------
pqServerManagerModelItem::~pqServerManagerModelItem()
{
  if (this->Connector)
  {
    this->Connector->Delete();
    this->Connector = nullptr;
  }
}

//-----------------------------------------------------------------------------
vtkEventQtSlotConnect* pqServerManagerModelItem::getConnector()
{
  if (!this->Connector)
  {
    this->Connector = vtkEventQtSlotConnect::New();
  }

  return this->Connector;
}
