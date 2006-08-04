/*=========================================================================

   Program: ParaView
   Module:    pqServerStartups.h

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

#ifndef _pqServerStartups_h
#define _pqServerStartups_h

#include "pqCoreExport.h"

#include <QObject>

class pqServerResource;
class pqServerStartup;
class pqSettings;

/** Manages a persistent collection of server "startups" - instructions
on how to start a local / remote server prior to connection */
class PQCORE_EXPORT pqServerStartups :
  public QObject
{
  Q_OBJECT

public:
  pqServerStartups();
  ~pqServerStartups();

  /** Returns "true" if the given server requires startup.  As an example,
  the "builtin" server never requires startup */
  bool startupRequired(const pqServerResource& server);
  /** Returns "true" if a startup has already been configured for the given
  sever.  If not, the caller will likely want to prompt the user for startup
  information, or cancel any attempt to start/connect to the given server. */
  bool startupAvailable(const pqServerResource& server);
  /** Returns the configured startup for the given server, or NULL */
  pqServerStartup* getStartup(const pqServerResource& server);

  /// Defines a collection of servers
  typedef QVector<pqServerResource> ServersT;
  /// Returns the set of servers that have startups configured.
  const ServersT servers() const;
  
  /// Configures the given server for manual (i.e do nothing) startup.
  void setManualStartup(const pqServerResource& server);
  /** Configure the given server for shell startup - the given command-line
  will be executed whenever the server needs to be started. */
  void setShellStartup(const pqServerResource& server, const QString& command_line, double delay);
  /// Removes startup configuration for the given servers
  void deleteStartups(const ServersT& servers);
  
  /// Loads startup configurations from user preferences
  void load(pqSettings&);
  /// Saves startup configurations to user preferences
  void save(pqSettings&);

signals:
  /// Signal emitted whenever the collection changes
  void changed();

private:
  pqServerStartups(const pqServerStartups&);
  pqServerStartups& operator=(const pqServerStartups&);
  
  class pqImplementation;
  pqImplementation* const Implementation;
};

#endif
