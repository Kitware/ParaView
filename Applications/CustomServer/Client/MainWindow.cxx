/*=========================================================================

   Program: ParaView
   Module:    $RCS $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

#include <vtkSmartPointer.h>
#include <vtkSMProxyManager.h>
#include <vtkSMXMLParser.h>

#include "pqApplicationCore.h"

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

//-----------------------------------------------------------------------------
MainWindow::MainWindow()
{
  // Create a bare-minimum user interface
  this->setWindowTitle("CustomServer Client");
  this->createStandardPipelineBrowser(false);
  this->connect(this, SIGNAL(serverChanged(pqServer*)), 
    this, SLOT(onServerChanged(pqServer*)));
  
  // Force the user to pick a server
  this->onServerConnect();
}

//-----------------------------------------------------------------------------
void MainWindow::onActiveServerChanged(pqServer* server)
{
  if(!server)
    {
    return;
    }

  // User picked a server, so create an instance of our custom object
  vtkSmartPointer<vtkSMXMLParser> parser = vtkSmartPointer<vtkSMXMLParser>::New();
  parser->Parse(custom_filters);
  parser->ProcessConfiguration(vtkSMProxyManager::GetProxyManager());

  pqApplicationCore::instance()->createSourceOnActiveServer("CustomSource");
}
