/*=========================================================================

   Program: ParaView
   Module:    pqCommandServerStartup.cxx

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

#include "pqServerResource.h"
#include "pqServerStartupContext.h"
#include "pqCommandServerStartup.h"

#include <QProcess>
#include <QTimer>
#include <QtDebug>

/////////////////////////////////////////////////////////////////////////////
// pqCommandServerStartupContextHelper

pqCommandServerStartupContextHelper::pqCommandServerStartupContextHelper(double delay, QObject* object_parent) :
  QObject(object_parent),
  Delay(delay)
{
}

void pqCommandServerStartupContextHelper::onFinished(int /*exitCode*/, QProcess::ExitStatus exitStatus)
{
  switch(exitStatus)
    {
    case QProcess::NormalExit:
      QTimer::singleShot(static_cast<int>(this->Delay * 1000), this, SLOT(onDelayComplete()));
      break;
    case QProcess::CrashExit:
      qWarning() << "The startup command crashed";
      emit this->failed();
      break;
    }
  
}

void pqCommandServerStartupContextHelper::onError(QProcess::ProcessError error)
{
  switch(error)
    {
    case QProcess::FailedToStart:
      qWarning() << "The startup command failed to start ... check your PATH and file permissions";
      break;
    case QProcess::Crashed:
      qWarning() << "The startup command crashed";
      break;
    default:
      qWarning() << "Unknown error running startup command";
      break;
    }
  
  emit this->failed();
}

void pqCommandServerStartupContextHelper::onDelayComplete()
{
  emit this->succeeded();
}

/////////////////////////////////////////////////////////////////////////////
// pqCommandServerStartup

pqCommandServerStartup::pqCommandServerStartup(const QString& command_line, double delay) :
  CommandLine(command_line),
  Delay(delay)
{
}

void pqCommandServerStartup::execute(
  const pqServerResource& server, pqServerStartupContext& context)
{
  QStringList environment = QProcess::systemEnvironment();
  environment.push_back("PV_CONNECTION_URI=" + server.toString());
  environment.push_back("PV_CONNECTION_SCHEME=" + server.scheme());
  environment.push_back("PV_SERVER_HOST=" + server.host());
  environment.push_back("PV_SERVER_PORT=" + QString::number(server.port(11111)));
  environment.push_back("PV_DATA_SERVER_HOST=" + server.dataServerHost());
  environment.push_back("PV_DATA_SERVER_PORT=" + QString::number(server.dataServerPort(11111)));
  environment.push_back("PV_RENDER_SERVER_HOST=" + server.renderServerHost());
  environment.push_back("PV_RENDER_SERVER_PORT=" + QString::number(server.renderServerPort(22221)));
  environment.push_back("PV_USERNAME=");

  QProcess* const process = new QProcess(&context);
  process->setEnvironment(environment);

  pqCommandServerStartupContextHelper* const helper = new pqCommandServerStartupContextHelper(this->Delay, &context);
  QObject::connect(process, SIGNAL(error(QProcess::ProcessError)), helper, SLOT(onError(QProcess::ProcessError)));
  QObject::connect(process, SIGNAL(finished(int, QProcess::ExitStatus)), helper, SLOT(onFinished(int, QProcess::ExitStatus)));

  QObject::connect(helper, SIGNAL(succeeded()), &context, SLOT(onSucceeded()));
  QObject::connect(helper, SIGNAL(failed()), &context, SLOT(onFailed()));  
  
  process->start(this->CommandLine);
}
