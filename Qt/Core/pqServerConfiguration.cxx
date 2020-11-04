/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
#include "pqServerConfiguration.h"

#include "pqServerResource.h"
#include "vtkNew.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStringList>
#include <QTextStream>

#include <cassert>
#include <sstream>

#define SERVER_CONFIGURATION_DEFAULT_NAME "unknown"

//-----------------------------------------------------------------------------
pqServerConfiguration::pqServerConfiguration()
{
  vtkNew<vtkPVXMLParser> parser;
  parser->Parse("<Server name='" SERVER_CONFIGURATION_DEFAULT_NAME
                "' configuration=''><ManualStartup/></Server>");
  this->constructor(parser->GetRootElement(), 60);
}

//-----------------------------------------------------------------------------
pqServerConfiguration::pqServerConfiguration(vtkPVXMLElement* xml, int connectionTimeout)
{
  this->constructor(xml, connectionTimeout);
}

//-----------------------------------------------------------------------------
void pqServerConfiguration::constructor(vtkPVXMLElement* xml, int connectionTimeout)
{
  assert(xml && xml->GetName() && strcmp(xml->GetName(), "Server") == 0);
  this->XML = xml;
  this->ConnectionTimeout = connectionTimeout;
  this->Mutable = true;
  this->parseSshPortForwardingXML();
}

//-----------------------------------------------------------------------------
pqServerConfiguration::~pqServerConfiguration()
{
}

//-----------------------------------------------------------------------------
void pqServerConfiguration::setName(const QString& arg_name)
{
  this->XML->SetAttribute("name", arg_name.toLocal8Bit().data());
}

//-----------------------------------------------------------------------------
QString pqServerConfiguration::name() const
{
  return this->XML->GetAttributeOrDefault("name", "");
}

//-----------------------------------------------------------------------------
bool pqServerConfiguration::isNameDefault() const
{
  return this->name() == SERVER_CONFIGURATION_DEFAULT_NAME;
}

//-----------------------------------------------------------------------------
pqServerResource pqServerConfiguration::actualResource() const
{
  return pqServerResource(this->ActualURI, *this);
}

//-----------------------------------------------------------------------------
pqServerResource pqServerConfiguration::resource() const
{
  return pqServerResource(this->XML->GetAttributeOrDefault("resource", ""), *this);
}

//-----------------------------------------------------------------------------
QString pqServerConfiguration::URI() const
{
  return this->resource().toURI();
}

//-----------------------------------------------------------------------------
void pqServerConfiguration::setResource(const pqServerResource& arg_resource)
{
  this->setResource(arg_resource.schemeHostsPorts().toURI());
}

//-----------------------------------------------------------------------------
void pqServerConfiguration::setResource(const QString& str)
{
  this->XML->SetAttribute("resource", str.toLocal8Bit().data());

  // Make sure this->ActualURI is correctly updated if needed
  this->parseSshPortForwardingXML();
}

//-----------------------------------------------------------------------------
int pqServerConfiguration::connectionTimeout() const
{
  return this->ConnectionTimeout;
}

//-----------------------------------------------------------------------------
void pqServerConfiguration::setConnectionTimeout(int connectionTimeout)
{
  this->ConnectionTimeout = connectionTimeout;
}

//-----------------------------------------------------------------------------
pqServerConfiguration::StartupType pqServerConfiguration::startupType() const
{
  if (this->XML->FindNestedElementByName("ManualStartup"))
  {
    return MANUAL;
  }
  else if (this->XML->FindNestedElementByName("CommandStartup"))
  {
    return COMMAND;
  }

  return INVALID;
}

//-----------------------------------------------------------------------------
vtkPVXMLElement* pqServerConfiguration::optionsXML() const
{
  vtkPVXMLElement* startup = this->startupXML();
  if (startup != NULL)
  {
    return startup->FindNestedElementByName("Options");
  }
  return NULL;
}

//-----------------------------------------------------------------------------
vtkPVXMLElement* pqServerConfiguration::hintsXML() const
{
  return this->XML->FindNestedElementByName("Hints");
}

//-----------------------------------------------------------------------------
QString pqServerConfiguration::portForwardingLocalPort() const
{
  if (this->PortForwarding)
  {
    return this->PortForwardingLocalPort;
  }
  else
  {
    return QString::number(this->resource().port(11111));
  }
}

//-----------------------------------------------------------------------------
vtkPVXMLElement* pqServerConfiguration::startupXML() const
{
  switch (this->startupType())
  {
    case (MANUAL):
    {
      return this->XML->FindNestedElementByName("ManualStartup");
      break;
    }
    case (COMMAND):
    {
      return this->XML->FindNestedElementByName("CommandStartup");
      break;
    }
    default:
    {
      return NULL;
      break;
    }
  }
}

//-----------------------------------------------------------------------------
QString pqServerConfiguration::termCommand()
{
#if defined(__linux__)
  // Based on i3 code
  // https://github.com/i3/i3/blob/next/i3-sensible-terminal
  QStringList termNames = { qgetenv("TERMINAL"), "x-terminal-emulator", "urxvt", "rxvt", "termit",
    "terminator", "Eterm", "aterm", "uxterm", "xterm", "gnome-terminal", "roxterm",
    "xfce4-terminal", "termite", "lxterminal", "mate-terminal", "terminology", "st", "qterminal",
    "lilyterm", "tilix", "terminix", "konsole", "kitty", "guake", "tilda", "alacritty", "hyper" };
#elif defined(__APPLE__)
  QStringList termNames = {}; // No default term command on mac
#elif defined(_WIN32)
  // Only cmd is supported to be detected automatically on Windows for now
  QStringList termNames = { "cmd" };
#endif

  for (auto term : termNames)
  {
    QString termCommand = pqServerConfiguration::lookForCommand(term);
    if (!termCommand.isEmpty())
    {
      return termCommand;
    }
  }
  qCritical("Could not find a terminal command, make sure to set a $TERMINAL environement variable "
            "pointing to a valid terminal command");
  return QString();
}

//-----------------------------------------------------------------------------
QString pqServerConfiguration::sshCommand()
{
#if defined(_WIN32)
  QStringList sshNames = { "plink", "ssh" };
#else
  QStringList sshNames = { "ssh" };
#endif

  for (auto sshName : sshNames)
  {
    QString sshCommand = pqServerConfiguration::lookForCommand(sshName);
    if (!sshCommand.isEmpty())
    {
#if defined(_WIN32)
      if (sshName == "ssh")
      {
        qWarning(
          "Native ssh support on windows is experimental and may not work as expected, especially "
          "with rc connection. Please try with Putty if you encounter any issue.");
      }
#endif
      return sshCommand;
    }
  }
  qCritical("Could not find a ssh command, make sure it is in your path. "
#if defined(_WIN32)
            "In Windows 10, ssh is available since Spring 2018 update,"
            "Alternativaly, Putty can be installed to provide a ssh implementation.");
#else
    );
#endif
  return QString();
}

//-----------------------------------------------------------------------------
QString pqServerConfiguration::lookForCommand(QString command)
{
  if (command.isEmpty())
  {
    return QString();
  }

  QProcess lookForProcess;

#if defined(_WIN32)
  QString whichCommand = "where";
#else
  QString whichCommand = "which";
#endif

  lookForProcess.start(whichCommand, QStringList());
  if (!lookForProcess.waitForFinished())
  {
    qCritical() << "Could not find \"" << whichCommand
                << "\" command, make sure it is in your path";
    return QString();
  }

  QStringList arguments;
  arguments << command;
  lookForProcess.start(whichCommand, arguments);
  lookForProcess.setReadChannel(QProcess::ProcessChannel::StandardOutput);
  if (lookForProcess.waitForFinished())
  {
    QString retStr(lookForProcess.readAll());
    retStr = retStr.trimmed();
    QFile file(retStr);
    QFileInfo checkFile(file);
    if (checkFile.exists() && checkFile.isFile() && checkFile.isExecutable())
    {
      return QString("\"") + QDir::toNativeSeparators(checkFile.absoluteFilePath()) + "\"";
    }
  }
  return QString();
}

//-----------------------------------------------------------------------------
void pqServerConfiguration::parseSshPortForwardingXML()
{
  pqServerResource resource = this->resource();
  this->ActualURI = resource.toURI();
  this->PortForwarding = false;
  this->SSHCommand = false;
  vtkPVXMLElement* commandStartup = this->XML->FindNestedElementByName("CommandStartup");
  if (commandStartup)
  {
    vtkPVXMLElement* sshCommandXML = commandStartup->FindNestedElementByName("SSHCommand");
    if (sshCommandXML)
    {
      vtkPVXMLElement* sshConfigXML = sshCommandXML->FindNestedElementByName("SSHConfig");
      if (sshConfigXML)
      {
        this->SSHCommand = true;
        vtkPVXMLElement* sshForwardXML = sshConfigXML->FindNestedElementByName("PortForwarding");
        if (sshForwardXML)
        {
          QString scheme = resource.scheme();
          if (scheme != "cs" && scheme != "csrc")
          {
            qCritical() << "SSH PortForwarding is not compatible with" << scheme
                        << "server. It is only compatible with cs and csrc servers."
                        << "Please set up the port forwarding manually in your case.";
            this->PortForwarding = false;
          }
          else
          {
            this->PortForwarding = true;
            this->PortForwardingLocalPort = QString(sshForwardXML->GetAttributeOrDefault(
              "local", QString::number(this->resource().port()).toUtf8().data()));

            // rc connection do not need to set the actual URI
            // as connection is managed server side with pvserver arguments
            if (!resource.isReverse())
            {
              resource.setHost("localhost");
              resource.setPort(this->PortForwardingLocalPort.toInt());
              this->ActualURI = resource.toURI();
            }
          }
        }
      }
    }
  }
}

//-----------------------------------------------------------------------------
QString pqServerConfiguration::command(double& timeout, double& delay) const
{
  timeout = 0;
  delay = 0;
  if (this->startupType() != COMMAND)
  {
    return QString();
  }

  QString reply;
  QTextStream stream(&reply);

  // Recover exec command
  QString execCommand = this->execCommand(timeout, delay);

  if (this->SSHCommand)
  {
    vtkPVXMLElement* commandStartup = this->XML->FindNestedElementByName("CommandStartup");
    vtkPVXMLElement* commandXML = commandStartup->FindNestedElementByName("SSHCommand");
    if (commandXML == nullptr)
    {
      // SSHCommand should always have a SSHCommandXML
      qCritical("Missing SSHCommand in server configuration");
      return QString();
    }

    vtkPVXMLElement* sshConfigXML = commandXML->FindNestedElementByName("SSHConfig");
    if (sshConfigXML == nullptr)
    {
      // SSHCommand should always have a SSHConfig xml
      qCritical("Missing SSHConfig in server configuration");
      return QString();
    }

    // Recover a sssh command
    // It can be specified in the XML.
    // If not we look for default ssh names.
    vtkPVXMLElement* sshCommandExecXML = sshConfigXML->FindNestedElementByName("SSH");
    QString sshCommand;
    if (sshCommandExecXML)
    {
      sshCommand = sshCommandExecXML->GetAttributeOrDefault("exec", "");
    }
    if (sshCommand.isEmpty())
    {
      sshCommand = pqServerConfiguration::sshCommand();
    }

    if (sshCommand.isEmpty())
    {
      qCritical("No ssh command or capabilities found, cannot use SSHCommand.");
      return QString();
    }
    else
    {
      // Recover full ssh command
      QString sshFullCommand = this->sshFullCommand(sshCommand, sshConfigXML);

#if defined(__linux__)
      // Simple askpass support
      vtkPVXMLElement* sshAskpassXML = sshConfigXML->FindNestedElementByName("Askpass");
      if (sshAskpassXML)
      {
        stream << "setsid ";
      }
#endif

      // Recover a terminal command
      // It can be specified in the XML.
      // If not we look for default terminal names
      vtkPVXMLElement* sshTermXML = sshConfigXML->FindNestedElementByName("Terminal");
      QString termCommand;
      if (sshTermXML)
      {
        termCommand = sshTermXML->GetAttributeOrDefault("exec", "");
        if (termCommand.isEmpty())
        {
#if defined(__APPLE__)
          termCommand = "Terminal.app"; // On MacOS , Terminal Management is specific
#else
          termCommand = pqServerConfiguration::termCommand();
        }
        if (!termCommand.isEmpty())
        {
#if defined(__linux__)
          stream << termCommand << " -e ";
#elif defined(_WIN32)
          stream << "cmd /C start \"SSH Terminal\" " << termCommand << " /C ";
#endif
        }
        else
        {
          // A terminal have beeen requested but none have been found
          qCritical(
            "A Terminal was requested in the server XML, but no Terminal have been found on your "
            "system, "
            "we will still try to connect through ssh but you may not be able to input a password");
#endif // (__APPLE__)
        }
      }

#if defined(__APPLE__)
      if (sshTermXML && termCommand == "Terminal.app")
      {
        /* Command explanation :
           /bin/sh : because QProcess can run only a single command at a time
           echo sshCommand remoteCommand into a temporary shell script :
           because MacOS Terminal can only run a single command or a single script
           ps, pid and kill : because MacOS do not close Terminal app after running the script
           chmod Saved State : When killing an application, MacOS will try to restore its state next
           time it runs. We ensure that this does not happen by changing permissions.
           rm script to clean up at the end.
         */
        stream << "/bin/sh -c \"tmpFile=`mktemp`; echo \'#!/bin/sh\n"
               << sshFullCommand << " " << execCommand
               << ";pid=`ps -o ppid= -p $PPID`; ppid=`ps -o ppid= -p $pid`; kill -2 $ppid; exit\'"
                  "> $tmpFile; chmod +x $tmpFile; chmod -rw ~/Library/Saved\\ Application\\ "
                  "State/com.apple.Terminal.savedState/; open -W -n -a Terminal $tmpFile; rm "
                  "$tmpFile; chmod +rw ~/Library/Saved\\ Application\\ "
                  "State/com.apple.Terminal.savedState/;\"";
      }
      else
      {
        // MacOS support for standard terminal emulator like xterm
        if (sshTermXML && !termCommand.isEmpty())
        {
          stream << termCommand << " -e ";
        }
#else
      {
#endif
        stream << sshFullCommand << " " << execCommand;
      }
    }
  }
  else
  {
    stream << execCommand;
  }
  return reply;
}

//-----------------------------------------------------------------------------
QString pqServerConfiguration::sshFullCommand(
  QString sshCommand, vtkPVXMLElement* sshConfigXML) const
{
  QString sshFullCommand;
  QTextStream sshStream(&sshFullCommand);
  sshStream << sshCommand << " ";

  vtkPVXMLElement* sshForwardXML = sshConfigXML->FindNestedElementByName("PortForwarding");
  if (sshForwardXML)
  {
    QString sshFlag = "-L ";
    if (this->resource().isReverse())
    {
      sshFlag = "-R ";
    }
    sshStream << sshFlag << this->PortForwardingLocalPort
              << ":localhost:" << QString::number(this->resource().port()) << " ";
  }

  QString sshUser = sshConfigXML->GetAttributeOrDefault("user", "");
  if (!sshUser.isEmpty())
  {
    sshStream << sshUser << "@";
  }
  sshStream << this->resource().host();
  return sshFullCommand;
}

//-----------------------------------------------------------------------------
QString pqServerConfiguration::execCommand(double& timeout, double& delay) const
{
  timeout = 0;
  delay = 0;
  if (this->startupType() != COMMAND)
  {
    return QString();
  }

  QString CommandXMLString = "Command";
  if (this->SSHCommand)
  {
    CommandXMLString = "SSHCommand";
  }
  vtkPVXMLElement* commandStartup = this->XML->FindNestedElementByName("CommandStartup");
  vtkPVXMLElement* commandXML =
    commandStartup->FindNestedElementByName(CommandXMLString.toUtf8().data());

  if (commandXML == nullptr)
  {
    // CommandStartup is missing the  <Command/> and <SSHCommand/> element. That's peculiar, but
    // not a critical error. So we return empty string.
    return QString();
  }

  QString reply;
  QTextStream stream(&reply);

  commandXML->GetScalarAttribute("timeout", &timeout);
  commandXML->GetScalarAttribute("delay", &delay);

  stream << commandXML->GetAttributeOrDefault("exec", "");
  vtkPVXMLElement* argumentsXML = commandXML->FindNestedElementByName("Arguments");
  if (argumentsXML)
  {
    for (unsigned int cc = 0; cc < argumentsXML->GetNumberOfNestedElements(); cc++)
    {
      const char* value = argumentsXML->GetNestedElement(cc)->GetAttribute("value");
      if (value)
      {
        // if value contains space, quote it.
        if (QRegExp("\\s").indexIn(value) == -1)
        {
          stream << " " << value;
        }
        else
        {
          stream << " \"" << value << "\"";
        }
      }
    }
  }
  return reply;
}

//-----------------------------------------------------------------------------
void pqServerConfiguration::setStartupToManual()
{
  vtkPVXMLElement* startupElement = this->startupXML();
  if (startupElement)
  {
    startupElement->SetName("ManualStartup");
  }
  else
  {
    vtkNew<vtkPVXMLElement> child;
    child->SetName("ManualStartup");
    this->XML->AddNestedElement(child.GetPointer());
  }
}

//-----------------------------------------------------------------------------
void pqServerConfiguration::setStartupToCommand(
  double timeout, double delay, const QString& command_str)
{
  // we try to preserve any existing options.
  vtkPVXMLElement* startupElement = this->startupXML();
  if (startupElement)
  {
    startupElement->SetName("CommandStartup");
  }
  else
  {
    vtkNew<vtkPVXMLElement> child;
    child->SetName("CommandStartup");
    this->XML->AddNestedElement(child.GetPointer());
    startupElement = child.GetPointer();
  }

  // remove any existing command.
  QString CommandXMLString = "Command";
  if (this->SSHCommand)
  {
    CommandXMLString = "SSHCommand";
  }

  vtkPVXMLElement* xmlCommand =
    startupElement->FindNestedElementByName(CommandXMLString.toUtf8().data());

  QStringList commandList = command_str.split(" ", QString::SkipEmptyParts);
  assert(commandList.size() >= 1);

  xmlCommand->SetAttribute("exec", commandList[0].toLocal8Bit().data());
  xmlCommand->SetAttribute("timeout", QString::number(timeout).toUtf8().data());
  xmlCommand->SetAttribute("delay", QString::number(delay).toUtf8().data());

  vtkPVXMLElement* oldArguments = xmlCommand->FindNestedElementByName("Arguments");
  if (oldArguments)
  {
    xmlCommand->RemoveNestedElement(oldArguments);
  }

  vtkNew<vtkPVXMLElement> xmlArguments;
  xmlArguments->SetName("Arguments");
  xmlCommand->AddNestedElement(xmlArguments.GetPointer());

  for (int i = 1; i < commandList.size(); ++i)
  {
    vtkNew<vtkPVXMLElement> xmlArgument;
    xmlArgument->SetName("Argument");
    xmlArguments->AddNestedElement(xmlArgument.GetPointer());
    xmlArgument->AddAttribute("value", commandList[i].toLocal8Bit().data());
  }
}

//-----------------------------------------------------------------------------
QString pqServerConfiguration::toString(vtkIndent indent) const
{
  std::stringstream stream;
  this->XML->PrintXML(stream, indent);
  return stream.str().c_str();
}

//-----------------------------------------------------------------------------
pqServerConfiguration pqServerConfiguration::clone() const
{
  vtkNew<vtkPVXMLElement> xml;
  this->XML->CopyTo(xml.GetPointer());
  return pqServerConfiguration(xml.GetPointer(), this->ConnectionTimeout);
}
