// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (C) 2021 Jonas Lukasczyk
// SPDX-License-Identifier: BSD-3-Clause

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
