/*=========================================================================

   Program: ParaView
   Module:    ParticlesViewerStarter.cxx

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
#include "ParticlesViewerStarter.h"
#include "ui_ParticlesViewerMainWindow.h"

#include "pqCoreUtilities.h"
#include "pqApplicationCore.h"
#include "pqLoadDataReaction.h"
#include "pqParaViewBehaviors.h"
#include "pqParaViewMenuBuilders.h"
#include "ParticlesViewerDisplayPolicy.h"

//-----------------------------------------------------------------------------
ParticlesViewerStarter::ParticlesViewerStarter(QObject* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
void ParticlesViewerStarter::startApplication()
{
  QMainWindow* window =
    qobject_cast<QMainWindow*>(pqCoreUtilities::mainWidget());
  Q_ASSERT(window != 0);

  Ui::ParticlesViewerMainWindow ui;
  ui.setupUi(window);

  new pqLoadDataReaction(ui.action_Open_Dataset);
  QObject::connect(ui.action_Exit, SIGNAL(triggered()),
    pqApplicationCore::instance(), SLOT(quit()));

  pqApplicationCore::instance()->setDisplayPolicy(
    new ParticlesViewerDisplayPolicy(this));

  pqParaViewMenuBuilders::buildViewMenu(*ui.menu_View, *window);

  // Final step, define application behaviors. Since we want all ParaView
  // behaviors, we use this convenience method.
  new pqParaViewBehaviors(window, window);
}

//-----------------------------------------------------------------------------
void ParticlesViewerStarter::stopApplication()
{
}

//-----------------------------------------------------------------------------
