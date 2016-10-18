/*=========================================================================

   Program: ParaView
   Module:    pqManageCustomFiltersReaction.cxx

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
#include "pqManageCustomFiltersReaction.h"

#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqCustomFilterManager.h"
#include "pqCustomFilterManagerModel.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerObserver.h"

//-----------------------------------------------------------------------------
pqManageCustomFiltersReaction::pqManageCustomFiltersReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  this->Model = new pqCustomFilterManagerModel(this);

  // Listen for compound proxy register events.
  pqServerManagerObserver* observer = pqApplicationCore::instance()->getServerManagerObserver();
  this->connect(observer, SIGNAL(compoundProxyDefinitionRegistered(QString)), this->Model,
    SLOT(addCustomFilter(QString)));
  this->connect(observer, SIGNAL(compoundProxyDefinitionUnRegistered(QString)), this->Model,
    SLOT(removeCustomFilter(QString)));

  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(
    smmodel, SIGNAL(serverAdded(pqServer*)), this->Model, SLOT(importCustomFiltersFromSettings()));
  QObject::connect(smmodel, SIGNAL(aboutToRemoveServer(pqServer*)), this->Model,
    SLOT(exportCustomFiltersToSettings()));
}

//-----------------------------------------------------------------------------
void pqManageCustomFiltersReaction::manageCustomFilters()
{
  pqCustomFilterManager dialog(this->Model, pqCoreUtilities::mainWidget());
  dialog.exec();
}
