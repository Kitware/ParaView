/*=========================================================================

   Program: ParaView
  Module:    main.cxx

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

#include <QApplication>
#include <QDir>
#include "ProcessModuleGUIHelper.h"
#include "pqMain.h"
#include "pqComponentsInit.h"

#ifdef Q_WS_X11
#include <QCleanlooksStyle>
#include <QMotifStyle>
#endif

int main(int argc, char* argv[])
{
  QApplication app(argc, argv);
#ifdef Q_WS_X11
  // There are lots of valid styles for X11.  For many systems the default
  // is Motif, which is ugly and has been giving dashboard errors.  Rather
  // than fix the problem, I am just forcing the style to Cleanlooks.
  if(qobject_cast<QMotifStyle*>(QApplication::style()))
    {
    QApplication::setStyle(new QCleanlooksStyle);
    }
#endif

  pqComponentsInit();

  QDir dir(QApplication::applicationDirPath());
  dir.cdUp();
  dir.cd("Plugins");
  QApplication::setLibraryPaths(QStringList(dir.absolutePath()));
  return pqMain::Run(app, ProcessModuleGUIHelper::New());
}

