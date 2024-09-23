// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqServerConfiguration.h"

#include "pqQtDeprecated.h"
#include "pqServerResource.h"
#include "vtkNew.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>

#include <cassert>
#include <sstream>

static constexpr const char* SERVER_CONFIGURATION_DEFAULT_NAME = "unknown";

//-----------------------------------------------------------------------------
pqServerConfiguration::pqServerConfiguration()
{
  this->constructor(SERVER_CONFIGURATION_DEFAULT_NAME);
}

//-----------------------------------------------------------------------------
pqServerConfiguration::pqServerConfiguration(const QString& name)
{
  this->constructor(name);
}

//-----------------------------------------------------------------------------
pqServerConfiguration::pqServerConfiguration(vtkPVXMLElement* xml)
{
  this->constructor(xml);
}

//-----------------------------------------------------------------------------
void pqServerConfiguration::constructor(const QString& name)
{
  QString xml = QString("<Server name='") + name + "' configuration=''><ManualStartup/></Server>";
  vtkNew<vtkPVXMLParser> parser;
  parser->Parse(xml.toUtf8().data());
  this->constructor(parser->GetRootElement());
}

//-----------------------------------------------------------------------------
void pqServerConfiguration::constructor(vtkPVXMLElement* xml)
{
  assert(xml && xml->GetName() && strcmp(xml->GetName(), "Server") == 0);
  this->XML = xml;
}

//-----------------------------------------------------------------------------
pqServerConfiguration::~pqServerConfiguration() = default;

//-----------------------------------------------------------------------------
void pqServerConfiguration::setName(const QString& arg_name)
{
  this->XML->SetAttribute("name", arg_name.toUtf8().data());
}

//-----------------------------------------------------------------------------
QString pqServerConfiguration::name() const
{
  return this->XML->GetAttributeOrDefault("name", "");
}

//-----------------------------------------------------------------------------
QString pqServerConfiguration::defaultName()
{
  return SERVER_CONFIGURATION_DEFAULT_NAME;
}

//-----------------------------------------------------------------------------
bool pqServerConfiguration::isNameDefault() const
{
  return this->name() == SERVER_CONFIGURATION_DEFAULT_NAME;
}

//-----------------------------------------------------------------------------
pqServerResource pqServerConfiguration::actualResource()
{
  // Update the actual URI
  this->parseSshPortForwardingXML();

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
  return this->resource().schemeHostsPorts().toURI();
}

//-----------------------------------------------------------------------------
void pqServerConfiguration::setResource(const pqServerResource& arg_resource)
{
  this->setResource(arg_resource.schemeHostsPorts().toURI());
}

//-----------------------------------------------------------------------------
void pqServerConfiguration::setResource(const QString& str)
{
  this->XML->SetAttribute("resource", str.toUtf8().data());
}

//-----------------------------------------------------------------------------
int pqServerConfiguration::connectionTimeout(int defaultTimeout) const
{
  return QString(
    this->XML->GetAttributeOrDefault("timeout", QString::number(defaultTimeout).toUtf8().data()))
    .toInt();
}

//-----------------------------------------------------------------------------
void pqServerConfiguration::setConnectionTimeout(int connectionTimeout)
{
  this->XML->SetAttribute("timeout", QString::number(connectionTimeout).toUtf8().data());
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
  if (startup != nullptr)
  {
    return startup->FindNestedElementByName("Options");
  }
  return nullptr;
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
      return nullptr;
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

  for (const auto& term : termNames)
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

  for (const auto& sshName : sshNames)
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
  this->ActualURI = resource.schemeHostsPorts().toURI();
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

            resource.setHost("localhost");
            resource.setPort(this->PortForwardingLocalPort.toInt());
            this->ActualURI = resource.schemeHostsPorts().toURI();
          }
        }
      }
    }
  }
}

//-----------------------------------------------------------------------------
QString pqServerConfiguration::command(double& processWait, double& delay) const
{
  if (this->startupType() != COMMAND)
  {
    return QString();
  }

  QString reply;
  QTextStream stream(&reply);

  // Recover exec command
  QString execCommand = this->execCommand(processWait, delay);

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
  else { stream << execCommand; }
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
    if (this->resource().isReverse())
    {
      // Reverse tunnelling
      sshStream << "-R " << QString::number(this->resource().port())
                << ":localhost:" << this->PortForwardingLocalPort << " ";
    }
    else
    {
      // Forward tunnelling
      sshStream << "-L " << this->PortForwardingLocalPort
                << ":localhost:" << QString::number(this->resource().port()) << " ";
    }
  }

  QString sshUser = sshConfigXML->GetAttributeOrDefault("user", "");
  if (!sshUser.isEmpty())
  {
    sshStream << sshUser << "@";
  }
  sshStream << this->resource().host() << " ";
  return sshFullCommand;
}

//-----------------------------------------------------------------------------
QString pqServerConfiguration::execCommand(double& processWait, double& delay) const
{
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

  processWait = 0;
  delay = 0;

  if (commandXML->GetScalarAttribute("timeout", &processWait))
  {
    qWarning("timeout attribute in Command and SSHCommand element has been deprecated, please use "
             "process_wait attribute instead");
  }

  commandXML->GetScalarAttribute("process_wait", &processWait);
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
        if (!QRegularExpression("\\s").match(value).hasMatch())
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
  double processWait, double delay, const QString& command_str)
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
  if (!xmlCommand)
  {
    vtkNew<vtkPVXMLElement> child;
    child->SetName(CommandXMLString.toUtf8().data());
    startupElement->AddNestedElement(child.GetPointer());
    xmlCommand = child.GetPointer();
  }

  // attributes
  xmlCommand->SetAttribute("process_wait", QString::number(processWait).toUtf8().data());
  xmlCommand->SetAttribute("delay", QString::number(delay).toUtf8().data());

  // clean up arguments
  vtkPVXMLElement* oldArguments = xmlCommand->FindNestedElementByName("Arguments");
  if (oldArguments)
  {
    xmlCommand->RemoveNestedElement(oldArguments);
  }

  vtkNew<vtkPVXMLElement> xmlArguments;
  xmlArguments->SetName("Arguments");
  xmlCommand->AddNestedElement(xmlArguments.GetPointer());

  // exec and arguments
  QStringList commandList = command_str.split(" ", PV_QT_SKIP_EMPTY_PARTS);
  if (!commandList.empty())
  {
    xmlCommand->SetAttribute("exec", commandList[0].toUtf8().data());
    for (int i = 1; i < commandList.size(); ++i)
    {
      vtkNew<vtkPVXMLElement> xmlArgument;
      xmlArgument->SetName("Argument");
      xmlArguments->AddNestedElement(xmlArgument.GetPointer());
      xmlArgument->AddAttribute("value", commandList[i].toUtf8().data());
    }
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
  return pqServerConfiguration(xml.GetPointer());
}
