/*=========================================================================

   Program: ParaView
   Module:    pqPythonQtPlugin.cxx

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

#include "vtkPython.h"

#include "pqPluginDecorators.h"
#include "pqPythonQtPlugin.h"
#include "pqPythonQtWrapperFactory.h"

#include <pqPVApplicationCore.h>
#include <pqPythonDialog.h>
#include <pqPythonManager.h>
#include <pqPythonShell.h>

#include <PythonQt.h>
#include <PythonQt_QtBindings.h>

#include "vtkPythonInterpreter.h"

//-----------------------------------------------------------------------------
class pqPythonQtPlugin::pqInternal
{
public:
};

//-----------------------------------------------------------------------------
pqPythonQtPlugin::pqPythonQtPlugin(QWidget* p)
  : QObject(p)
{
  this->Internal = new pqInternal;

  /*
    pqPythonManager* pythonManager = pqPVApplicationCore::instance()->pythonManager();
    this->connect(pythonManager, SIGNAL(paraviewPythonModulesImported()), SLOT(initialize()));

    if (pythonManager->interpreterIsInitialized())
      {
      this->initialize();
      }
  */
  this->initialize();
}

//-----------------------------------------------------------------------------
pqPythonQtPlugin::~pqPythonQtPlugin()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqPythonQtPlugin::startup()
{
}

//-----------------------------------------------------------------------------
void pqPythonQtPlugin::shutdown()
{
}

//-----------------------------------------------------------------------------
void pqPythonQtPlugin::initialize()
{
  // pqPythonManager* pythonManager = pqPVApplicationCore::instance()->pythonManager();
  // this->disconnect(pythonManager, SIGNAL(paraviewPythonModulesImported()), this,
  // SLOT(initialize()));

  // pqPythonShell* shell = pythonManager->pythonShellDialog()->shell();

  vtkPythonInterpreter::Initialize();

  PythonQt::init(PythonQt::PythonAlreadyInitialized);
  PythonQt_init_QtBindings();
  PythonQt::self()->addWrapperFactory(new pqPythonQtWrapperFactory);
  PythonQt::self()->addDecorators(new pqPluginDecorators());
}
