/*=========================================================================

   Program: ParaView
   Module:    pqPersistentMainWindowStateBehavior.cxx

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
#include "pqPersistentMainWindowStateBehavior.h"

#include "pqApplicationCore.h"
#include "pqSettings.h"

#include <QCoreApplication>
#include <QMainWindow>
#include <QTimer>

#include <cassert>

//-----------------------------------------------------------------------------
pqPersistentMainWindowStateBehavior::pqPersistentMainWindowStateBehavior(QMainWindow* parentWindow)
  : Superclass(parentWindow)
{
  assert(parentWindow != NULL);
  QObject::connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(saveState()));

  // This is done after a slight delay so that any GUI elements that get created
  // as a consequence of loading of the configuration files will have their
  // state restored as well.
  QTimer::singleShot(10, this, SLOT(restoreState()));

  this->restoreState();
}

//-----------------------------------------------------------------------------
pqPersistentMainWindowStateBehavior::~pqPersistentMainWindowStateBehavior()
{
}

//-----------------------------------------------------------------------------
void pqPersistentMainWindowStateBehavior::restoreState(QMainWindow* window)
{
  pqApplicationCore::instance()->settings()->restoreState("MainWindow", *window);
}

//-----------------------------------------------------------------------------
void pqPersistentMainWindowStateBehavior::saveState(QMainWindow* window)
{
  pqApplicationCore::instance()->settings()->saveState(*window, "MainWindow");
}

//-----------------------------------------------------------------------------
void pqPersistentMainWindowStateBehavior::restoreState()
{
  QMainWindow* window = qobject_cast<QMainWindow*>(this->parent());
  pqPersistentMainWindowStateBehavior::restoreState(window);
}

//-----------------------------------------------------------------------------
void pqPersistentMainWindowStateBehavior::saveState()
{
  QMainWindow* window = qobject_cast<QMainWindow*>(this->parent());
  pqPersistentMainWindowStateBehavior::saveState(window);
}
