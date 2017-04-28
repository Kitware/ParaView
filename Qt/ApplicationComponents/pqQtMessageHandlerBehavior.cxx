/*=========================================================================

   Program: ParaView
   Module:    pqQtMessageHandlerBehavior.cxx

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
#include "pqQtMessageHandlerBehavior.h"

#include "vtkOutputWindow.h"
#include "vtkSetGet.h" // for VTK_LEGACY

#if QT_VERSION >= 0x050000
#if !defined(VTK_LEGACY_REMOVE)
static void QtMessageOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
  QByteArray localMsg = msg.toLocal8Bit();
  QString dispMsg = QString("%1 (%2:%3, %4)")
                      .arg(localMsg.constData())
                      .arg(context.file)
                      .arg(context.line)
                      .arg(context.function);
  switch (type)
  {
    case QtDebugMsg:
      vtkOutputWindow::GetInstance()->DisplayText(dispMsg.toLocal8Bit().constData());
      break;
    case QtWarningMsg:
      vtkOutputWindow::GetInstance()->DisplayErrorText(dispMsg.toLocal8Bit().constData());
      break;
    case QtCriticalMsg:
      vtkOutputWindow::GetInstance()->DisplayErrorText(dispMsg.toLocal8Bit().constData());
      break;
    case QtFatalMsg:
      vtkOutputWindow::GetInstance()->DisplayErrorText(dispMsg.toLocal8Bit().constData());
      break;
    default:
      break;
  }
}
#endif

//-----------------------------------------------------------------------------
pqQtMessageHandlerBehavior::pqQtMessageHandlerBehavior(QObject* parentObject)
  : Superclass(parentObject)
{
#if !defined(VTK_LEGACY_REMOVE)
  auto oldHandler = qInstallMessageHandler(::QtMessageOutput);
  // don't replace handler setup by pqOutputWidget as that's the newer/better code.
  // this class is deprecated.
  if (oldHandler != nullptr)
  {
    qInstallMessageHandler(oldHandler);
  }
#endif
}
pqQtMessageHandlerBehavior::~pqQtMessageHandlerBehavior()
{
#if !defined(VTK_LEGACY_REMOVE)
  qInstallMessageHandler(0);
#endif
}

#else

#if !defined(VTK_LEGACY_REMOVE)
static void QtMessageOutput(QtMsgType type, const char* msg)
{
  switch (type)
  {
    case QtDebugMsg:
      vtkOutputWindow::GetInstance()->DisplayText(msg);
      break;
    case QtWarningMsg:
      vtkOutputWindow::GetInstance()->DisplayWarningText(msg);
      break;
    case QtCriticalMsg:
      vtkOutputWindow::GetInstance()->DisplayErrorText(msg);
      break;
    case QtFatalMsg:
      vtkOutputWindow::GetInstance()->DisplayErrorText(msg);
      break;
  }
}
#endif

//-----------------------------------------------------------------------------
pqQtMessageHandlerBehavior::pqQtMessageHandlerBehavior(QObject* parentObject)
  : Superclass(parentObject)
{
#if !defined(VTK_LEGACY_REMOVE)
  auto oldHandler = qInstallMsgHandler(::QtMessageOutput);
  // don't replace handler setup by pqOutputWidget as that's the newer code.
  // this class is deprecated.
  if (oldHandler != nullptr)
  {
    qInstallMsgHandler(oldHandler);
  }
#endif
}
pqQtMessageHandlerBehavior::~pqQtMessageHandlerBehavior()
{
#if !defined(VTK_LEGACY_REMOVE)
  qInstallMsgHandler(0);
#endif
}

#endif
