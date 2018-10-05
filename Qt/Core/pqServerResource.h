/*=========================================================================

   Program: ParaView
   Module:    pqServerResource.h

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

#ifndef _pqServerResource_h
#define _pqServerResource_h

#include "pqCoreModule.h"

#include <QString>
class pqServerConfiguration;

/**
* pqServerResource encapsulates a resource in ParaView. A resource can be anything,
* a data file, a list of data files, a state file, or a connection to a server.
* The resource specification is based on URL-like syntax:
* \verbatim
* <scheme>://<scheme-specific-parameters>
* \endverbatim
*
* To specify a data file, the following syntax is used:
* \verbatim
* <connection-scheme>:[//<server-details>]/<path-to-data-file>
* \endverbatim
*
* \c connection-scheme can be
* \li \c builtin - for builtin connections
* \li \c cs - for client-server connections (pvserver)
* \li \c csrc - for client-server connections with reverse-connect (pvserver rc)
* \li \c cdsrs - for client-data-server-render-server connections (pvdataserver, pvrenderserver)
* \li \c cdsrsrc - for cdsrs with reverse-connect.
*
* \c server-details are of the form \c \<serverhost-name\>:\<port\> or
* \c \<dataserver-hostname\>:\<dataserver-port\>/\<renderserver-hostname\>:\<render-server-port\>
* as applicable. Port numbers are always optional.
*
* Examples:
* \verbatim
* builtin:/home/user/foo.vtk
* cs://amber1:11112/C:\Users\User\foo.vtk
* cdsrsrc://amber2:11111/amber3:22222/home/user/foo.vtk
* \endverbatim
*
* To specify a state file, the following syntax is used:
* \verbatim
* session:/<path-to-state-file>
* \endverbatim
*
* Session files are not associated with any connection.
*
* To specify a server-connection, without pointing to any data file(s), the
* following syntax may be used:
* \verbatim
* <connection-scheme>:[//<server-details>]
* \endverbatim
*
* Examples:
* \verbatim
* builtin:
* cs://amber1:11112
* cdsrsrc://amber2:11111/amber3:22222
* \endverbatim
*
* As with data-files, port numbers are always optional in when specifying
* server-details.
*
* Arbitrary data can be added to a resource. ParaView leverages this mechanism
* to save additional files in a file series when referring to a data file, or
* details about how to connect to the server when referring to a
* server-connection.
*
* \sa pqServerResources, pqServer
*/
class PQCORE_EXPORT pqServerResource
{
public:
  pqServerResource();
  pqServerResource(const QString&);
  pqServerResource(const QString&, const pqServerConfiguration&);
  pqServerResource(const pqServerResource&);
  pqServerResource& operator=(const pqServerResource&);
  ~pqServerResource();

  /**
  * Returns the pqServerConfiguration from which this resource was created,
  * if any.
  */
  const pqServerConfiguration& configuration() const;

  /**
  * Returns a compact string representation of the resource in URI format
  * Prefer using configuration->URI() if a configuration is available
  * and not default.
  */
  const QString toURI() const;

  /**
  * Returns a compact string representation of the resource including extra
  * data
  */
  const QString serializeString() const;

  /** Returns the resource scheme -
  builtin, cs, csrc, cdsrs, cdsrsrc, or session */
  const QString scheme() const;

  /**
  * Sets the resource scheme
  */
  void setScheme(const QString&);

  /**
   * Returns if the connection scheme is a reverse one
   */
  bool isReverse() const;

  /** Returns the resource host, or empty string for builtin, session,
  cdsrs, and cdsrsrc schemes */
  const QString host() const;
  /**
  * Sets the resource host
  */
  void setHost(const QString&);

  int port() const;
  int port(int default_port) const;
  void setPort(int);

  const QString dataServerHost() const;
  void setDataServerHost(const QString&);

  int dataServerPort() const;
  int dataServerPort(int default_port) const;
  void setDataServerPort(int);

  const QString renderServerHost() const;
  void setRenderServerHost(const QString&);

  int renderServerPort() const;
  int renderServerPort(int default_port) const;
  void setRenderServerPort(int);

  const QString path() const;
  void setPath(const QString&);

  const pqServerResource sessionServer() const;
  void setSessionServer(const pqServerResource&);

  // add extra data to this resource
  void addData(const QString& key, const QString& value);
  // get extra data from this resource
  const QString data(const QString& key) const;
  const QString data(const QString& key, const QString& default_value) const;
  bool hasData(const QString& key) const;

  /** Returns a copy of this resource containing only server information -
  scheme, host, and port numbers */
  const pqServerResource schemeHostsPorts() const;
  /** Returns a copy of this resource containing a subset of server information -
  scheme and host (no port numbers */
  const pqServerResource schemeHosts() const;
  /** Returns a copy of this resource containing only host and path information -
  scheme, port numbers, and server session are excluded */
  const pqServerResource hostPath() const;

  bool operator==(const pqServerResource&) const;
  bool operator!=(const pqServerResource&) const;
  bool operator<(const pqServerResource&) const;

private:
  class pqImplementation;
  pqImplementation* const Implementation;
};

#endif
