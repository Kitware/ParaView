/*=========================================================================

   Program: ParaView
   Module:    pqDataQueryReaction.cxx

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
#include "pqDataQueryReaction.h"

#include "pqActiveObjects.h"
#include "pqCoreUtilities.h"
#include "pqFiltersMenuReaction.h"
#include "pqHelpReaction.h"
#include "pqPVApplicationCore.h"
#include "pqQueryDialog.h"
#include "pqSelectionManager.h"
#include "pqServerManagerModel.h"
#include "vtkPVConfig.h"

#include <QEventLoop>
#include <QMessageBox>

//-----------------------------------------------------------------------------
pqDataQueryReaction::pqDataQueryReaction(QAction* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
pqDataQueryReaction::~pqDataQueryReaction()
{
}

//-----------------------------------------------------------------------------
void pqDataQueryReaction::onExtractSelection()
{
  pqFiltersMenuReaction::createFilter("filters", "ExtractSelection");
}

//-----------------------------------------------------------------------------
void pqDataQueryReaction::onExtractSelectionOverTime()
{
  pqFiltersMenuReaction::createFilter("filters", "ExtractSelectionOverTime");
}

//-----------------------------------------------------------------------------
void pqDataQueryReaction::showHelp()
{
  pqHelpReaction::showHelp("qthelp://paraview.org/paraview/Book/Book_Chapter6.html");
}

//-----------------------------------------------------------------------------
void pqDataQueryReaction::showQueryDialog()
{
#ifdef PARAVIEW_ENABLE_PYTHON
  pqQueryDialog dialog(
    pqActiveObjects::instance().activePort(),
    pqCoreUtilities::mainWidget());

  // We want to make the query the active application wide selection, so we
  // hookup the query action to selection manager so that the application
  // realizes a new selection has been made.
  pqSelectionManager* selManager =
    pqPVApplicationCore::instance()->selectionManager();
  if (selManager)
    {
    QObject::connect(&dialog, SIGNAL(selected(pqOutputPort*)),
      selManager, SLOT(select(pqOutputPort*)));
    }
  dialog.show();
  QEventLoop loop;
  QObject::connect(&dialog, SIGNAL(finished(int)),
                   &loop,   SLOT(quit()));
  QObject::connect(&dialog, SIGNAL(extractSelection()),
                   this,    SLOT(onExtractSelection()));
  QObject::connect(&dialog, SIGNAL(extractSelectionOverTime()),
                   this,    SLOT(onExtractSelectionOverTime()));
  QObject::connect(&dialog, SIGNAL(helpRequested()),
                   this,    SLOT(showHelp()));
  loop.exec();
#else
  QMessageBox::warning(0,
                       "Selection Not Supported",
                       "Error: Find Data requires that ParaView be built with "
                       "Python enabled. To enable Python set the CMake flag '"
                       "PARAVIEW_ENABLE_PYTHON' to True.");
#endif // PARAVIEW_ENABLE_PYTHON
}
