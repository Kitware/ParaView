/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

#include "AboutDialog.h"
#include "MainWindow.h"

#include <pqConnect.h>
#include <pqSetName.h>

#include <QMenu>

#include <vtkPQConfig.h>

MainWindow::MainWindow()
{
  this->setWindowTitle(QByteArray("ParaQ Client") + QByteArray(" ") + QByteArray(PARAQ_VERSION_FULL));

  this->createStandardFileMenu();
  this->createStandardViewMenu();
  this->createStandardServerMenu();
  this->createStandardSourcesMenu();
  this->createStandardFiltersMenu();
  this->createStandardToolsMenu();
  
  QMenu* const help_menu = this->helpMenu();
  help_menu->addAction(tr("&About ParaQ"))
    << pqSetName("About")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onHelpAbout()));
  
  this->createStandardPipelineBrowser();
  this->createStandardObjectInspector();
  this->createStandardElementInspector(false);
  
  this->createStandardVCRToolBar();
  this->createStandardVariableToolBar();
  this->createStandardCompoundProxyToolBar();
}

void MainWindow::onHelpAbout()
{
  AboutDialog* const dialog = new AboutDialog(this);
  dialog->show();
}
