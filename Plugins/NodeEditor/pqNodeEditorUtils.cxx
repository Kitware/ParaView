/*=========================================================================

  Program:   ParaView
  Plugin:    NodeEditor

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
int pqNodeEditorUtils::getID(pqProxy* proxy)
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

// ----------------------------------------------------------------------------
unsigned long pqNodeEditorUtils::getTimeDelta()
{
  using namespace std::chrono;
  static unsigned long t0 = 0;

  unsigned long t1 = high_resolution_clock::now().time_since_epoch().count();
  unsigned long delta = t1 - t0;
  t0 = t1;
  return delta;
};

// ----------------------------------------------------------------------------
bool pqNodeEditorUtils::isDoubleClick()
{
  return getTimeDelta() < pqNodeEditorUtils::CONSTS::DOUBLE_CLICK_DELAY;
};
