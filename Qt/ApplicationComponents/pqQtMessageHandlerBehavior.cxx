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

#if QT_VERSION >= 0x050000

static void QtMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
  QByteArray localMsg = msg.toLocal8Bit();
  QString dispMsg = QString("%1 (%2:%3, %4)").arg(localMsg.constData()).
    arg(context.file).arg(context.line).arg(context.function);
  switch(type)
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
    }
}

//-----------------------------------------------------------------------------
pqQtMessageHandlerBehavior::pqQtMessageHandlerBehavior(QObject* parentObject)
  : Superclass(parentObject)
{
  qInstallMessageHandler(::QtMessageOutput);
}
pqQtMessageHandlerBehavior::~pqQtMessageHandlerBehavior()
{
  qInstallMessageHandler(0);
}

#else

static void QtMessageOutput(QtMsgType type, const char *msg)
{
  switch(type)
    {
  case QtDebugMsg:
    vtkOutputWindow::GetInstance()->DisplayText(msg);
    break;
  case QtWarningMsg:
    vtkOutputWindow::GetInstance()->DisplayErrorText(msg);
    break;
  case QtCriticalMsg:
    vtkOutputWindow::GetInstance()->DisplayErrorText(msg);
    break;
  case QtFatalMsg:
    vtkOutputWindow::GetInstance()->DisplayErrorText(msg);
    break;
    }
}

//-----------------------------------------------------------------------------
pqQtMessageHandlerBehavior::pqQtMessageHandlerBehavior(QObject* parentObject)
  : Superclass(parentObject)
{
  qInstallMsgHandler(::QtMessageOutput);
}
pqQtMessageHandlerBehavior::~pqQtMessageHandlerBehavior()
{
  qInstallMsgHandler(0);
}

#endif
