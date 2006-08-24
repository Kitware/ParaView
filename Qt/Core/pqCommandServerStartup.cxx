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

pqCommandServerStartupContextHelper::pqCommandServerStartupContextHelper(
    double delay, QObject* object_parent) :
  QObject(object_parent),
  Delay(delay)
{
}

void pqCommandServerStartupContextHelper::onFinished(int /*exitCode*/,
  QProcess::ExitStatus exitStatus)
{
  switch(exitStatus)
    {
    case QProcess::NormalExit:
      QTimer::singleShot(
        static_cast<int>(this->Delay * 1000), this, SLOT(onDelayComplete()));
      break;
    case QProcess::CrashExit:
      qWarning() << "The startup command crashed";
      emit this->failed();
      break;
    }
  
}

void pqCommandServerStartupContextHelper::onError(
  QProcess::ProcessError error)
{
  switch(error)
    {
    case QProcess::FailedToStart:
      qWarning() << "The startup command failed to start ... "
        "check your PATH and file permissions";
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

pqCommandServerStartup::pqCommandServerStartup(
    const QString& name,
    const pqServerResource& server,
    const QString& owner,
    const QDomDocument& configuration) :
  Name(name),
  Server(server.schemeHosts()),
  Owner(owner),
  Configuration(configuration)
{
}

const QString pqCommandServerStartup::name()
{
  return this->Name;
}

const pqServerResource pqCommandServerStartup::server()
{
  return this->Server;
}

const QString pqCommandServerStartup::owner()
{
  return this->Owner;
}

const QDomDocument pqCommandServerStartup::configuration()
{
  return this->Configuration;
}

void pqCommandServerStartup::execute(const OptionsT& user_options,
  pqServerStartupContext& context)
{
  // Generate predefined variables ...
  OptionsT options;
  options["PV_CONNECTION_URI"] =
    this->Server.toString();
  options["PV_CONNECTION_SCHEME"] =
    this->Server.scheme();
  options["PV_SERVER_HOST"] =
    this->Server.host();
  options["PV_SERVER_PORT"] =
    QString::number(this->Server.port(11111));
  options["PV_DATA_SERVER_HOST"] =
    this->Server.dataServerHost();
  options["PV_DATA_SERVER_PORT"] =
    QString::number(this->Server.dataServerPort(11111));
  options["PV_RENDER_SERVER_HOST"] =
    this->Server.renderServerHost();
  options["PV_RENDER_SERVER_PORT"] =
    QString::number(this->Server.renderServerPort(22221));
  options["PV_USERNAME"] =
    ""; // Unused at the moment
  
  // Merge user variables, allowing user variables to "override" the predefined variables ...
  for(OptionsT::const_iterator option = user_options.begin(); option != user_options.end(); ++option)
    {
    options.erase(options.find(option.key()));
    options.insert(option.key(), option.value());
    }

  // Setup the process environment ...
  QStringList environment = QProcess::systemEnvironment();
  for(
    OptionsT::const_iterator option = options.begin();
    option != options.end();
    ++option)
    {
    environment.push_back(option.key() + "=" + option.value());
    }
  
  // Setup the process arguments ...
  const QString executable = this->executable();
  const double timeout = this->timeout();
  const double delay = this->delay();
  QStringList arguments = this->arguments();

  // Do string-substitution on the process arguments ...
  QRegExp regex("[$]([^$].*)[$]");
  for(QStringList::iterator i = arguments.begin(); i != arguments.end(); )
    {
    QString& argument = *i;
    if(regex.indexIn(argument) > -1)
      {
      const QString before = regex.cap(0);
      const QString variable = regex.cap(1);
      const QString after = options[variable];
      argument.replace(before, after);
      }
      
    if(argument.isEmpty())
      {
      i = arguments.erase(i);
      }
    else
      {
      ++i;
      }
    }

  QProcess* const process = new QProcess();
  process->setEnvironment(environment);

  pqCommandServerStartupContextHelper* const helper =
    new pqCommandServerStartupContextHelper(delay, &context);
  QObject::connect(
    process,
    SIGNAL(error(QProcess::ProcessError)),
    helper,
    SLOT(onError(QProcess::ProcessError)));
  QObject::connect(
    process,
    SIGNAL(finished(int, QProcess::ExitStatus)),
    helper,
    SLOT(onFinished(int, QProcess::ExitStatus)));

  QObject::connect(
    helper,
    SIGNAL(succeeded()),
    &context,
    SLOT(onSucceeded()));
  QObject::connect(
    helper,
    SIGNAL(failed()),
    &context,
    SLOT(onFailed()));  
  
  process->start(executable, arguments);
}

const QString pqCommandServerStartup::executable()
{
  QString result;

  QDomElement xml = this->Configuration.documentElement();
  if(xml.nodeName() == "CommandStartup")
    {
    QDomElement xml_command = xml.firstChildElement("Command");
    if(!xml_command.isNull())
      {
      result = xml_command.attribute("exec");
      }
    }
    
  return result;
}

double pqCommandServerStartup::timeout()
{
  double result = 0.0;
  
  QDomElement xml = this->Configuration.documentElement();
  if(xml.nodeName() == "CommandStartup")
    {
    QDomElement xml_command = xml.firstChildElement("Command");
    if(!xml_command.isNull())
      {
      result = xml_command.attribute("timeout").toDouble();
      }
    }
    
  return result;
}

double pqCommandServerStartup::delay()
{
  double result = 0.0;
  
  QDomElement xml = this->Configuration.documentElement();
  if(xml.nodeName() == "CommandStartup")
    {
    QDomElement xml_command = xml.firstChildElement("Command");
    if(!xml_command.isNull())
      {
      result = xml_command.attribute("delay").toDouble();
      }
    }
    
  return result;
}

const QStringList pqCommandServerStartup::arguments()
{
  QStringList results;

  QDomElement xml = this->Configuration.documentElement();
  if(xml.nodeName() == "CommandStartup")
    {
    QDomElement xml_command = xml.firstChildElement("Command");
    if(!xml_command.isNull())
      {
      QDomElement xml_arguments = xml_command.firstChildElement("Arguments");
      if(!xml_arguments.isNull())
        {
        for(
          QDomNode xml_argument = xml_arguments.firstChild();
          !xml_argument.isNull();
          xml_argument = xml_argument.nextSibling())
          {
          if(xml_argument.isElement()
            && xml_argument.toElement().tagName() == "Argument")
            {
            const QString argument =
              xml_argument.toElement().attribute("value");
            if(!argument.isEmpty())
              {
              results.push_back(argument);
              }
            }
          }
        }
      }
    }
    
  return results;
}
