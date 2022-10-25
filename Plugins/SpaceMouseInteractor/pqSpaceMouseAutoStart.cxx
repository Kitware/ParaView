/*=========================================================================

   Program: ParaView
   Module:  pqSpaceMouseAutoStart.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
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
