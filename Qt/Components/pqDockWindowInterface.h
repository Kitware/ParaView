/*=========================================================================

   Program: ParaView
   Module:    pqDockWindowInterface.h

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

#ifndef _pqDockWindowInterface_h
#define _pqDockWindowInterface_h

#include "pqComponentsModule.h"
#include <QString>
#include <QtPlugin>
class QDockWidget;
class QWidget;

/**
* interface class for plugins that add a QDockWindow
*/
class PQCOMPONENTS_EXPORT pqDockWindowInterface
{
public:
  pqDockWindowInterface();
  virtual ~pqDockWindowInterface();

  virtual QString dockArea() const = 0;

  /**
  * Creates a dock window with the given parent
  */
  virtual QDockWidget* dockWindow(QWidget* p) = 0;

private:
  Q_DISABLE_COPY(pqDockWindowInterface)
};

Q_DECLARE_INTERFACE(pqDockWindowInterface, "com.kitware/paraview/dockwindow")

#endif
