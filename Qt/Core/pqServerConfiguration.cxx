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

#include <QStringList>
#include <QTextStream>
#include <vtksys/ios/sstream>

//-----------------------------------------------------------------------------
pqServerConfiguration::pqServerConfiguration()
{
  vtkNew<vtkPVXMLParser> parser;
  parser->Parse(
    "<Server name=\"unknown\" configuration=\"\"><ManualStartup/></Server>");
  this->constructor(parser->GetRootElement());
}

//-----------------------------------------------------------------------------
pqServerConfiguration::pqServerConfiguration(vtkPVXMLElement* xml)
{
  this->constructor(xml);
}

//-----------------------------------------------------------------------------
void pqServerConfiguration::constructor(vtkPVXMLElement* xml)
{
  Q_ASSERT(xml && xml->GetName() && strcmp(xml->GetName(), "Server") == 0);
  this->XML = xml;
  this->Mutable = true;
}

//-----------------------------------------------------------------------------
pqServerConfiguration::~pqServerConfiguration()
{
}

//-----------------------------------------------------------------------------
void pqServerConfiguration::setName(const QString& arg_name)
{
  this->XML->SetAttribute("name", arg_name.toAscii().data());
}

//-----------------------------------------------------------------------------
QString pqServerConfiguration::name() const
{
  return this->XML->GetAttributeOrDefault("name", "");
}

//-----------------------------------------------------------------------------
pqServerResource pqServerConfiguration::resource() const
{
  return pqServerResource(this->XML->GetAttributeOrDefault("resource", ""));
}

//-----------------------------------------------------------------------------
void pqServerConfiguration::setResource(const pqServerResource& arg_resource)
{
  this->setResource(arg_resource.schemeHostsPorts().toURI());
}

//-----------------------------------------------------------------------------
void pqServerConfiguration::setResource(const QString& str)
{
  this->XML->SetAttribute("resource", str.toAscii().data());
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
  if (this->XML->GetNumberOfNestedElements() == 1)
    {
    return this->XML->GetNestedElement(0)->FindNestedElementByName("Options");
    }
  return NULL;
}

//-----------------------------------------------------------------------------
/// If startupType() == COMMAND, then this method can be used to obtain
/// the command for the startup. Note that this does not include any
/// information options etc. that may be specified in the startup.
QString pqServerConfiguration::command(double& timeout, double& delay) const
{
  timeout = 0; delay = 0;
  if (this->startupType() != COMMAND)
    {
    return QString();
    }

  vtkPVXMLElement* commandStartup =
    this->XML->FindNestedElementByName("CommandStartup");
  vtkPVXMLElement* commandXML = commandStartup->FindNestedElementByName("Command");

  if (commandXML == NULL)
    {
    // CommandStartup is missing the  <Command/> element. That's peculiar, but
    // not a critical error. So we return empty string.
    return QString();
    }

  commandXML->GetScalarAttribute("timeout", &timeout);
  commandXML->GetScalarAttribute("delay", &delay);

  QString reply;
  QTextStream stream(&reply);
  stream << commandXML->GetAttributeOrDefault("exec", "");
  vtkPVXMLElement* arguments = commandXML->GetNumberOfNestedElements()==1?
    commandXML->GetNestedElement(0) : NULL;
  if (arguments)
    {
    for (unsigned int cc=0; cc < arguments->GetNumberOfNestedElements(); cc++)
      {
      const char* value =
        arguments->GetNestedElement(cc)->GetAttribute("value");
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
  vtkPVXMLElement* startupElement = this->XML->GetNestedElement(0);
  if (startupElement)
    {
    startupElement->SetName("ManualStartup");
    }
  else
    {
    this->XML->RemoveAllNestedElements(); // for sanity 
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
  vtkPVXMLElement* startupElement = this->XML->GetNestedElement(0);
  if (startupElement)
    {
    startupElement->SetName("CommandStartup");
    }
  else
    {
    this->XML->RemoveAllNestedElements(); // for sanity 
    vtkNew<vtkPVXMLElement> child;
    child->SetName("CommandStartup");
    this->XML->AddNestedElement(child.GetPointer());
    startupElement = child.GetPointer();
    }

  // remove any existing command.
  vtkPVXMLElement* old_command = startupElement->FindNestedElementByName("Command");
  if (old_command)
    {
    startupElement->RemoveNestedElement(old_command);
    old_command = NULL;
    }

  vtkNew<vtkPVXMLElement> xml_command;
  xml_command->SetName("Command");
  startupElement->AddNestedElement(xml_command.GetPointer());

  QStringList commandList = command_str.split(" ", QString::SkipEmptyParts);
  Q_ASSERT(commandList.size() >= 1);

  xml_command->AddAttribute("exec", commandList[0].toAscii().data());
  xml_command->AddAttribute("timeout", timeout);
  xml_command->AddAttribute("delay", delay);

  vtkNew<vtkPVXMLElement> xml_arguments;
  xml_arguments->SetName("Arguments");
  xml_command->AddNestedElement(xml_arguments.GetPointer());

  for (int i = 1; i < commandList.size(); ++i)
    {
    vtkNew<vtkPVXMLElement> xml_argument;
    xml_argument->SetName("Argument");
    xml_arguments->AddNestedElement(xml_argument.GetPointer());
    xml_argument->AddAttribute("value", commandList[i].toAscii().data());
    }
}

//-----------------------------------------------------------------------------
QString pqServerConfiguration::toString(vtkIndent indent) const
{
  vtksys_ios::stringstream stream;
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
