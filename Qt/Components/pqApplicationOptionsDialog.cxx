/*=========================================================================

   Program: ParaView
   Module:    pqApplicationOptionsDialog.cxx

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
#include "pqApplicationOptionsDialog.h"

#include "pqApplicationCore.h"
#include "pqApplicationOptions.h"
#include "pqGlobalRenderViewOptions.h"
#include "pqInterfaceTracker.h"
#include "pqViewOptionsInterface.h"
  
//-----------------------------------------------------------------------------
pqApplicationOptionsDialog::pqApplicationOptionsDialog(QWidget* p)
  : pqOptionsDialog(p)
{
  this->setWindowTitle("Settings");
  this->setApplyNeeded(true);

  pqApplicationOptions* appOptions = new pqApplicationOptions;
  this->addOptions(appOptions);
  
  pqGlobalRenderViewOptions* renOptions = new pqGlobalRenderViewOptions;
  this->addOptions(renOptions);
  
  QStringList pages = appOptions->getPageList();
  if(pages.size())
    {
    this->setCurrentPage(pages[0]);
    }

  /// Add panes as plugins are loaded.
  pqInterfaceTracker* tracker =
    pqApplicationCore::instance()->interfaceTracker();
  QObject::connect(tracker,
    SIGNAL(interfaceRegistered(QObject*)),
    this, SLOT(pluginLoaded(QObject*)));

  // Load panes from already loaded plugins.
  foreach (QObject* plugin_interface, tracker->interfaces())
    {
    this->pluginLoaded(plugin_interface);
    }
}

//-----------------------------------------------------------------------------
void pqApplicationOptionsDialog::pluginLoaded(QObject* iface)
{
  pqViewOptionsInterface* viewOptions =
    qobject_cast<pqViewOptionsInterface*>(iface);
  if (viewOptions)
    {
    foreach(QString viewtype, viewOptions->viewTypes())
      {
      // Try to create global view options
      pqOptionsContainer* globalOptions =
        viewOptions->createGlobalViewOptions(viewtype, this);
      if (globalOptions)
        {
        this->addOptions(globalOptions);
        }
      }
    }
}

