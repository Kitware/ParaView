/*=========================================================================

   Program: ParaView
   Module:    pqServerStartups.h

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

#ifndef _pqServerStartups_h
#define _pqServerStartups_h

#include "pqCoreExport.h"

#include <QObject>

class pqServerResource;
class pqServerStartup;
class vtkPVXMLElement;

/** Manages a persistent collection of server "startups" - instructions
on how to start a server prior to connection */
class PQCORE_EXPORT pqServerStartups :
  public QObject
{
  Q_OBJECT

public:
  pqServerStartups(QObject* p);
  ~pqServerStartups();

  /// Defines a collection of startups
  typedef QStringList StartupsT;
  /// Returns the set of all configured startups that are configured
  const StartupsT getStartups() const;
  /** Returns the set of startups configured for a specific server
  (could return zero-to-many results) */
  const StartupsT getStartups(const pqServerResource& server) const;
  /// Returns a configured startup by name, or NULL
  pqServerStartup* getStartup(const QString& name) const;

  /// Configures the given startup for manual (i.e do nothing) startup.
  void setManualStartup(
    const QString& name,
    const pqServerResource& server);
  /** Configures the given startup for command startup - the given command
  will be executed whenever the server needs to be started. */
  void setCommandStartup(
    const QString& name,
    const pqServerResource& server,
    const QString& binary,
    double timeout,
    double delay,
    const QStringList& arguments);
    
  /// Removes startup configuration for the given servers
  void deleteStartups(const StartupsT& startups);
  
  /// Saves startup configurations to user preferences
  void save(const QString& file, bool userPrefs) const;
  /// Loads startup configurations from user preferences
  void load(const QString& file, bool userPrefs);

signals:
  /// Signal emitted whenever the collection changes
  void changed();

private:
  void save(vtkPVXMLElement*, bool userPrefs) const;
  void load(vtkPVXMLElement*, bool userPrefs);

  pqServerStartups(const pqServerStartups&);
  pqServerStartups& operator=(const pqServerStartups&);
  
  class pqImplementation;
  pqImplementation* const Implementation;
};

#endif
