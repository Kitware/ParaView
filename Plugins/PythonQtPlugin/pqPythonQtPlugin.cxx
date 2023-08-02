// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#define QT_NO_KEYWORDS

#include "pqPythonQtPlugin.h"

#include "vtkPython.h"
#include "vtkPythonInterpreter.h"

#include "pqPluginDecorators.h"
#include "pqPythonQtWrapperFactory.h"

#include "pqPythonShell.h"

#include <PythonQt.h>
#if WITH_QtAll
#include <PythonQt_QtAll.h>
#else
#include <PythonQt_QtBindings.h>
#endif

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
  this->initialize();
}

//-----------------------------------------------------------------------------
pqPythonQtPlugin::~pqPythonQtPlugin()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqPythonQtPlugin::startup() {}

//-----------------------------------------------------------------------------
void pqPythonQtPlugin::shutdown() {}

//-----------------------------------------------------------------------------
void pqPythonQtPlugin::initialize()
{
  vtkPythonInterpreter::Initialize();

  PythonQt::init(PythonQt::PythonAlreadyInitialized);
#if WITH_QtAll
  PythonQt_QtAll::init();
#else
  PythonQt_init_QtBindings();
#endif
  PythonQt::self()->addWrapperFactory(new pqPythonQtWrapperFactory);
  PythonQt::self()->addDecorators(new pqPluginDecorators());
}
