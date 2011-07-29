/*=========================================================================

   Program: ParaView
   Module:    pqCommandServerStartup.cxx

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

#include "pqCommandServerStartup.h"

#include "vtkPVConfig.h"

#include <QTimer>
#include <QtDebug>

#if defined(Q_WS_WIN)
# include <winsock2.h>
#else
# include <unistd.h>
#endif

#include "vtkPVXMLElement.h"

#include "pqServerResource.h"

class pqCommandServerStartupWSAInit
{
public:
  pqCommandServerStartupWSAInit()
    {
#if defined(Q_WS_WIN)
    WSAData wsadata;
    WSAStartup(MAKEWORD(2,0), &wsadata);
#endif
    }
  ~pqCommandServerStartupWSAInit()
    {
#if defined(Q_WS_WIN)
    WSACleanup();
#endif
    }
};

// TODO maybe move this to kwsys or something
static QString localHostName()
{
  pqCommandServerStartupWSAInit init;
  char hostName[512];
  if (gethostname(hostName, sizeof(hostName)) == -1)
    return QString();
  hostName[sizeof(hostName) - 1] = '\0';
  return QString::fromLocal8Bit(hostName);
}

/////////////////////////////////////////////////////////////////////////////
// pqCommandServerStartup

pqCommandServerStartup::pqCommandServerStartup(
    const QString& name,
    const pqServerResource& server,
    bool save,
    vtkPVXMLElement* configuration) :
  pqServerStartup(save),
  Name(name),
  Server(server.schemeHostsPorts()),
  Configuration(configuration),
  Process(NULL)
{
}

const QString pqCommandServerStartup::getName()
{
  return this->Name;
}

const pqServerResource pqCommandServerStartup::getServer()
{
  return this->Server;
}

vtkPVXMLElement* pqCommandServerStartup::getConfiguration()
{
  return this->Configuration;
}

void pqCommandServerStartup::execute(const OptionsT& user_options)
{
  // Generate predefined variables ...
  OptionsT options;
  options["PV_CONNECTION_URI"] =
    this->Server.toURI();
  options["PV_CONNECTION_SCHEME"] =
    this->Server.scheme();
  options["PV_CLIENT_HOST"] =
    localHostName();
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
  options["PV_CONNECT_ID"] = "";
  options["PV_USERNAME"] = ""; // Unused at the moment
  options["PV_VERSION_MAJOR"] =
    QString::number(PARAVIEW_VERSION_MAJOR);
  options["PV_VERSION_MINOR"] =
    QString::number(PARAVIEW_VERSION_MINOR);
  options["PV_VERSION_PATCH"] =
    QString::number(PARAVIEW_VERSION_PATCH);
  options["PV_VERSION"] = PARAVIEW_VERSION;
  options["PV_VERSION_FULL"] = PARAVIEW_VERSION_FULL;
  
  // Merge user variables, allowing user variables to "override" the predefined variables ...
  for(OptionsT::const_iterator option = user_options.begin(); option != user_options.end(); ++option)
    {
    const QString key = option.key();
    const QString value = option.value();
    
    options.erase(options.find(key));
    options.insert(key, value);
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
  QString executable = this->getExecutable();
  // const double timeout = this->getTimeout();
  QStringList arguments = this->getArguments();

  QRegExp regex("\\$([^$]*)\\$");
  // Do string-substitution on the executable
  while (regex.indexIn(executable) > -1)
    {
    QString before = regex.cap(0);
    QString variable = regex.cap(1);
    QString after = options[variable];
    executable.replace(before, after);
    }

  // Do string-substitution on the process arguments ...
  for (QStringList::iterator i = arguments.begin(); i != arguments.end(); )
    {
    QString& argument = *i;
    while (regex.indexIn(argument) > -1)
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

  this->Process = new QProcess;
  QObject::connect(this->Process, SIGNAL(finished(int, QProcess::ExitStatus)),
                   this->Process, SLOT(deleteLater()));

  this->Process->setEnvironment(environment);

  QObject::connect(
    this->Process, SIGNAL(readyReadStandardError()),
    this, SLOT(onReadyReadStandardError()));
    
  QObject::connect(
    this->Process, SIGNAL(readyReadStandardOutput()),
    this, SLOT(onReadyReadStandardOutput()));
    
  QObject::connect(
    this->Process, SIGNAL(error(QProcess::ProcessError)),
    this, SLOT(onError(QProcess::ProcessError)));

  QObject::connect(
    this->Process, SIGNAL(started()),
    this, SLOT(onStarted()));
  
  QObject::connect(
    &this->Timer, SIGNAL(timeout()),
    this, SLOT(onDelayComplete()));
    
  this->Process->start(executable, arguments);
}

const QString pqCommandServerStartup::getExecutable()
{
  QString result;

  vtkPVXMLElement* xml = this->Configuration;
  if(QString(xml->GetName()) == "CommandStartup")
    {
    vtkPVXMLElement* xml_command = xml->FindNestedElementByName("Command");
    if(xml_command)
      {
      result = xml_command->GetAttribute("exec");
      }
    }
    
  return result;
}

double pqCommandServerStartup::getTimeout()
{
  double result = 0.0;
  
  vtkPVXMLElement* xml = this->Configuration;
  if(QString(xml->GetName()) == "CommandStartup")
    {
    vtkPVXMLElement* xml_command = xml->FindNestedElementByName("Command");
    if(xml_command)
      {
      result = QString(xml_command->GetAttribute("timeout")).toDouble();
      }
    }
    
  return result;
}

double pqCommandServerStartup::getDelay()
{
  double result = 0.0;
  
  vtkPVXMLElement* xml = this->Configuration;
  if(QString(xml->GetName()) == "CommandStartup")
    {
    vtkPVXMLElement* xml_command = xml->FindNestedElementByName("Command");
    if(xml_command)
      {
      result = QString(xml_command->GetAttribute("delay")).toDouble();
      }
    }
    
  return result;
}

const QStringList pqCommandServerStartup::getArguments()
{
  QStringList results;

  vtkPVXMLElement* xml = this->Configuration;
  if(QString(xml->GetName()) == "CommandStartup")
    {
    vtkPVXMLElement* xml_command = xml->FindNestedElementByName("Command");
    if(xml_command)
      {
      vtkPVXMLElement* xml_arguments = xml_command->FindNestedElementByName("Arguments");
      if(xml_arguments)
        {
        int num = xml_arguments->GetNumberOfNestedElements();
        for(int i=0; i<num; i++)
          {
          vtkPVXMLElement* xml_argument = xml_arguments->GetNestedElement(i);
          if(QString(xml_argument->GetName()) == "Argument")
            {
            QString argument = xml_argument->GetAttribute("value");
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

void pqCommandServerStartup::onReadyReadStandardOutput()
{
  qDebug() << this->Process->readAllStandardOutput().data();
}

void pqCommandServerStartup::onReadyReadStandardError()
{
  qWarning() << this->Process->readAllStandardError().data();
}

void pqCommandServerStartup::onStarted()
{
  this->Timer.setSingleShot(true);
  this->Timer.setInterval(static_cast<int>(this->getDelay() * 1000));
  this->Timer.start();
}

void pqCommandServerStartup::onError(
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
  this->Timer.stop();
  
  emit this->failed();
}

void pqCommandServerStartup::onDelayComplete()
{
  if(this->Process && this->Process->state() == QProcess::NotRunning)
    {
    if(this->Process->exitStatus() == QProcess::CrashExit)
      {
      qWarning() << "The startup command crashed";
      emit this->failed();
      }
    }
  emit this->succeeded();
}

