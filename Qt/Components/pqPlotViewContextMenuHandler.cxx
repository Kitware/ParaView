/*=========================================================================

   Program: ParaView
   Module:    pqPlotViewContextMenuHandler.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/

/// \file pqPlotViewContextMenuHandler.cxx
/// \date 9/19/2007

#include "pqPlotViewContextMenuHandler.h"

#include "pqActiveViewOptionsManager.h"
#include "pqView.h"
#include "pqPlotView.h"
#include "pqPlotViewContextMenu.h"


pqPlotViewContextMenuHandler::pqPlotViewContextMenuHandler(
    QObject *parentObject)
  : pqViewContextMenuHandler(parentObject)
{
  this->Manager = 0;
}

void pqPlotViewContextMenuHandler::setOptionsManager(
    pqActiveViewOptionsManager *manager)
{
  this->Manager = manager;
}

void pqPlotViewContextMenuHandler::setupContextMenu(pqView *view)
{
  pqPlotView *plotView = qobject_cast<pqPlotView *>(view);
  if(plotView)
    {
    // See if the view has a context menu object already.
    pqPlotViewContextMenu *context =
        plotView->findChild<pqPlotViewContextMenu *>("ContextMenuSetup");
    if(!context)
      {
      context = new pqPlotViewContextMenu(plotView, this->Manager);
      context->setObjectName("ContextMenuSetup");
      this->connect(context, SIGNAL(screenshotRequested()),
          this, SIGNAL(screenshotRequested()));
      }
    }
}

void pqPlotViewContextMenuHandler::cleanupContextMenu(pqView *view)
{
  pqPlotView *plotView = qobject_cast<pqPlotView *>(view);
  if(plotView)
    {
    pqPlotViewContextMenu *context =
        plotView->findChild<pqPlotViewContextMenu *>("ContextMenuSetup");
    if(context)
      {
      delete context;
      }
    }
}


