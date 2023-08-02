// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
  /**
   * Create an empty server configuration with the default name.
   */
  pqServerConfiguration();

  /**
   * Default destructor
   */
  ~pqServerConfiguration();

  /**
   * Create an empty server configuration with a provided name.
   */
  pqServerConfiguration(const QString& name);

  /**
   * Create a server configuration with the provided xml.
   */
  pqServerConfiguration(vtkPVXMLElement* xml);

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
   * Returns the default name of a server configuration.
   */
  static QString defaultName();

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
   * resource() method should be used in any other cases.
   */
  pqServerResource actualResource();

  /**
   * get the resource URI, does not contain the server name
   */
  QString URI() const;

  /**
   * Get/Set the timeout in seconds that will be used when connecting
   * 0 means no retry and -1 means infinite retries.
   * If not set in the XML, the defaultTimeout is retuned.
   */
  int connectionTimeout(int defaultTimeout = 60) const;
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
   * This also recovers processWait and delay attributes.
   * processWait is the amount of time to wait for the process to start in seconds.
   * delay is the amount of time wait for the process to finish, -1 means infinite wait.
   */
  QString command(double& processWait, double& delay) const;

  /**
   * If startupType() == COMMAND, then this method can be used to obtain
   * the command for the startup, on the remote server,
   * contained in the exec attributes.
   * Note that this does not include any information options etc.
   * that may be specified in the startup.
   * This also recovers processWait and delay attributes.
   * processWait is the amount of time to wait for the process to start in seconds.
   * delay is the amount of time wait for the process to finish, -1 means infinite wait.
   */
  QString execCommand(double& processWait, double& delay) const;

  /**
   * changes the startup type to manual.
   */
  void setStartupToManual();

  /**
   * changes the startup type to command.
   */
  void setStartupToCommand(double processWait, double delay, const QString& command);

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
   * Equal to the resource port if port forwarding is not used.
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
  void constructor(const QString& name);
  void constructor(vtkPVXMLElement*);
  bool Mutable = true;
  vtkSmartPointer<vtkPVXMLElement> XML;
  bool PortForwarding = false;
  bool SSHCommand = false;
  QString PortForwardingLocalPort;
  QString ActualURI;
};

#endif
