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
#ifndef pqServerConfiguration_h
#define pqServerConfiguration_h

#include "pqCoreModule.h"
#include "pqServerResource.h"
#include "vtkPVXMLElement.h" // needed for ivar
#include "vtkSmartPointer.h"
#include <QObject>

class vtkPVXMLElement;
class pqServerResource;
class vtkIndent;

/**
* pqServerConfiguration corresponds to a server connection configuration.
* These are typically read from pvsc files.
*/
class PQCORE_EXPORT pqServerConfiguration
{
public:
  pqServerConfiguration();
  ~pqServerConfiguration();

  /**
   * Create a server configuration with the provided xml and timeout.
   * the timeout is in seconds, 0 means no retry and -1 means infinite retries.
   */
  pqServerConfiguration(vtkPVXMLElement* xml, int connectionTimeout = 60);

  /**
  * Get/Set whether the configuration is mutable. This variable is not
  * serialized.
  */
  bool isMutable() const { return this->Mutable; }
  void setMutable(bool val) { this->Mutable = val; }

  /**
  * Get/Set the name for the configuration.
  */
  void setName(const QString& name);
  QString name() const;

  /**
  * Returns true if the name for this configuration is the default one i.e.
  * the one that gets set when none is specified. Useful to determine "empty"
  * configurations.
  */
  bool isNameDefault() const;

  /**
  * Get/Set the resource that describes the server scheme, hostname(s) and port(s).
  */
  pqServerResource resource() const;
  void setResource(const QString&);
  void setResource(const pqServerResource&);

  /**
  * Get the actual resource that describes the server scheme, hostname(s) and port(s).
  * Can be different from resource() when using SSH Port Forwarding as it will point to
  * ip and port actually used for the tcp connection,
  * which can be different than the server where the pvserver is running.
  * eg. it will give you localhost:8080, instead of serverip:serverport when using port forwarding.
  * Using this method is needed only when using low level tcp api.
  * ressource() method should be used in any other cases.
  */
  pqServerResource actualResource() const;

  /**
   * get the resource URI
   */
  QString URI() const;

  /**
  * Get/Set the timeout in seconds that will be used when connecting
  * 0 means no retry and -1 means infinite retries.
  */
  int connectionTimeout() const;
  void setConnectionTimeout(int connectionTimeout);

  /**
  * Types of start
  */
  enum StartupType
  {
    INVALID,
    MANUAL,
    COMMAND
  };

  /**
  * returns the startup type for this configuration. There are 3 types of
  * startup: manual, simple-command and custom-command.
  */
  StartupType startupType() const;

  /**
  * If startupType() == COMMAND, then this method can be used to obtain
  * the command for the startup, from the client side.
  * Note that this does not include any information options etc.
  * that may be specified in the startup.
  * This is the full command to be executed on the client,
  * which includes xterm, ssh...
  */
  QString command(double& timeout, double& delay) const;

  /**
  * If startupType() == COMMAND, then this method can be used to obtain
  * the command for the startup, on the remote server,
  * contained in the exec attributes.
  * Note that this does not include any information options etc.
  * that may be specified in the startup. This also
  * recovers timeout and delay attributes.
  */
  QString execCommand(double& timeout, double& delay) const;

  /**
  * changes the startup type to manual.
  */
  void setStartupToManual();

  /**
  * changes the startup type to command.
  */
  void setStartupToCommand(double timeout, double delay, const QString& command);

  /**
  * serialize to a string.
  */
  QString toString(vtkIndent indent) const;

  /**
  * Create a new clone (deep copying the vtkPVXMLElement).
  */
  pqServerConfiguration clone() const;

  /**
  * returns the \<Options\> element, if any.
  */
  vtkPVXMLElement* optionsXML() const;

  /**
  * returns the \<Hints\> element, if any.
  */
  vtkPVXMLElement* hintsXML() const;

  /**
   * Get if this is a port forwarding configuration.
   * PortForwardingConfiguration uses SSH tunneling
   * and is configured directly in xml server file
   */
  bool isPortForwarding() const { return this->PortForwarding; };

  /**
   * Get the port forwarding local port.
   * Initialized by the server xml if port forwarding is used.
   * Equal to the ressource port if port forwarding is not used.
   */
  QString portForwardingLocalPort() const;

protected:
  vtkPVXMLElement* startupXML() const;

  void parseSshPortForwardingXML();
  QString sshFullCommand(QString sshCommand, vtkPVXMLElement* sshConfigXML) const;

  static QString termCommand();
  static QString sshCommand();
  static QString lookForCommand(QString command);

private:
  void constructor(vtkPVXMLElement*, int connectionTimeout = 60);
  bool Mutable;
  int ConnectionTimeout;
  vtkSmartPointer<vtkPVXMLElement> XML;
  bool PortForwarding;
  bool SSHCommand;
  QString PortForwardingLocalPort;
  QString ActualURI;
};

#endif
