/*=========================================================================

   Program: ParaView
   Module:    $RCS $

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

#include "MainWindow.h"

#include "ui_MainWindow.h"

#include <pqApplicationCore.h>
#include <pqMainWindowCore.h>
#include <pqMultiView.h>
#include <pqObjectBuilder.h>
#include <pqPendingDisplayManager.h>
#include <pqViewManager.h>

#include <vtkSmartPointer.h>
#include <vtkSMProxyManager.h>
#include <vtkSMXMLParser.h>

/// Embeds the ParaView server XML that describes the interface to the custom functionality
const char* const custom_filters =

"<ServerManagerConfiguration>"
"  <ProxyGroup name=\"sources\">"
"   <SourceProxy name=\"CustomSource\" class=\"vtkCustomSource\">"
"      <DoubleVectorProperty"
"         name=\"XLength\""
"         command=\"SetXLength\""
"         number_of_elements=\"1\""
"         animateable=\"1\""
"         default_values=\"1.0\" >"
"        <DoubleRangeDomain name=\"range\" min=\"0\" />"
"      </DoubleVectorProperty>"
"      <DoubleVectorProperty"
"         name=\"YLength\""
"         command=\"SetYLength\""
"         number_of_elements=\"1\""
"         animateable=\"1\""
"         default_values=\"1.0\" >"
"        <DoubleRangeDomain name=\"range\" min=\"0\" />"
"      </DoubleVectorProperty>"
"      <DoubleVectorProperty"
"         name=\"ZLength\""
"         command=\"SetZLength\""
"         number_of_elements=\"1\""
"         animateable=\"1\""
"         default_values=\"1.0\" >"
"        <DoubleRangeDomain name=\"range\" min=\"0\" />"
"      </DoubleVectorProperty>"
"      <DoubleVectorProperty"
"         name=\"Center\""
"         command=\"SetCenter\""
"         number_of_elements=\"3\""
"         animateable=\"1\""
"         default_values=\"0.0 0.0 0.0\" >"
"        <DoubleRangeDomain name=\"range\"/>"
"      </DoubleVectorProperty>"
"   </SourceProxy>"
"  </ProxyGroup>"
"</ServerManagerConfiguration>";

class MainWindow::pqImplementation
{
public:
  pqImplementation(QWidget* parent) :
    Core(parent)
  {
  }
  
  Ui::MainWindow UI;
  pqMainWindowCore Core;
};

//-----------------------------------------------------------------------------
MainWindow::MainWindow() :
  Implementation(new pqImplementation(this))
{
  this->Implementation->UI.setupUi(this);

  connect(this->Implementation->UI.actionFileExit,
    SIGNAL(triggered()), QApplication::instance(), SLOT(quit()));  

  this->Implementation->Core.setupPipelineBrowser(
    this->Implementation->UI.pipelineBrowserDock);

  this->connect(
    pqApplicationCore::instance(), SIGNAL(finishedAddingServer(pqServer*)),
    this, SLOT(onServerCreationFinished(pqServer*)));
   
  pqPendingDisplayManager* pdm = 
    qobject_cast<pqPendingDisplayManager*>(
      pqApplicationCore::instance()->manager("PENDING_DISPLAY_MANAGER"));
  if (pdm)
    {
    this->connect(pdm, SIGNAL(pendingDisplays(bool)),
        pdm, SLOT(createPendingDisplays()), Qt::QueuedConnection);
    }

  // Setup the multiview render window ...
  this->setCentralWidget(&this->Implementation->Core.multiViewManager());

  // Setup the statusbar ...
  this->Implementation->Core.setupProgressBar(this->statusBar());
  
  // Now that we're ready, initialize everything ...
  this->Implementation->Core.initializeStates();

  // Force the user to pick a server
  this->Implementation->Core.onServerConnect();
}

MainWindow::~MainWindow()
{
  delete this->Implementation;
}

//-----------------------------------------------------------------------------
void MainWindow::onServerCreationFinished(pqServer* server)
{
  if(!server)
    {
    return;
    }

  // User picked a server, so create an instance of our custom object
  vtkSmartPointer<vtkSMXMLParser> parser = vtkSmartPointer<vtkSMXMLParser>::New();
  parser->Parse(custom_filters);
  parser->ProcessConfiguration(vtkSMProxyManager::GetProxyManager());

  pqApplicationCore::instance()->getObjectBuilder()->createSource("sources",
    "CustomSource", server);
}


