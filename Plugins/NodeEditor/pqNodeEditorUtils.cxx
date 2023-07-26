// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/*-------------------------------------------------------------------------
  ParaViewPluginsNodeEditor - BSD 3-Clause License - Copyright (C) 2021 Jonas Lukasczyk

  See the Copyright.txt file provided
  with ParaViewPluginsNodeEditor for license information.
-------------------------------------------------------------------------*/

#include <pqNodeEditorUtils.h>

#include <pqProxy.h>

#include <vtkSMProxy.h>

#include <chrono>
#include <iostream>

// ----------------------------------------------------------------------------
vtkIdType pqNodeEditorUtils::getID(pqProxy* proxy)
{
  if (proxy == nullptr)
  {
    return -1;
  }
  auto smProxy = proxy->getProxy();
  return smProxy ? smProxy->GetGlobalID() : -1;
};

// ----------------------------------------------------------------------------
std::string pqNodeEditorUtils::getLabel(pqProxy* proxy)
{
  if (!proxy)
  {
    return "nullptr Proxy";
  }

  return proxy->getSMName().toStdString() + "<" + std::to_string(getID(proxy)) + ">";
};
