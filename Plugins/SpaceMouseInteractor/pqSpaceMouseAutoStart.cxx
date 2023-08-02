// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4201) // nonstandard extension, nameless union
#endif

#include "pqSpaceMouseAutoStart.h"
#if defined(_MSC_VER) || defined(__APPLE__)
#include "pqSpaceMouseImpl.h"
#else
#include "pqSpaceMouseImplLinux.h"
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "pqActiveObjects.h"

#include "vtkLogger.h"
#include "vtkObject.h"

#include <iostream>

//-----------------------------------------------------------------------------
pqSpaceMouseAutoStart::pqSpaceMouseAutoStart(QObject* parentObject)
  : Superclass(parentObject)
  , m_p(new pqSpaceMouseImpl())
{
}

//-----------------------------------------------------------------------------
pqSpaceMouseAutoStart::~pqSpaceMouseAutoStart()
{
  delete m_p;
}

//-----------------------------------------------------------------------------
void pqSpaceMouseAutoStart::startup()
{
  connect(&pqActiveObjects::instance(), &pqActiveObjects::viewChanged, m_p,
    &pqSpaceMouseImpl::setActiveView);
  m_p->setActiveView(pqActiveObjects::instance().activeView());
}

//-----------------------------------------------------------------------------
void pqSpaceMouseAutoStart::shutdown()
{
  m_p->setActiveView(nullptr);
}
