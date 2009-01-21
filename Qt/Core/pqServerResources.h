/*=========================================================================

   Program: ParaView
   Module:    pqServerResources.h

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

#ifndef _pqServerResources_h
#define _pqServerResources_h

#include "pqCoreExport.h"
#include "pqServerResource.h"

#include <QObject>
#include <QVector>

class pqSettings;
class pqServer;

/**
Encapsulates a persistent collection of recently-used resources (files)
that are located on specific servers.

\sa pqServerResource, pqServer */
class PQCORE_EXPORT pqServerResources :
  public QObject
{
  Q_OBJECT

public:
  pqServerResources(QObject* p);
  ~pqServerResources();

  /// Defines an ordered collection of resources
  typedef QVector<pqServerResource> ListT;

  /** Add a resource to the collection / 
  move the resource to the beginning of the list */
  virtual void add(const pqServerResource& resource);
  /** Returns the contents of the collection, ordered from
  most-recently-used to least-recently-used */
  const ListT list() const;

  /// Load the collection (from local user preferences)
  virtual void load(pqSettings&);
  /// Save the collection (to local user preferences)
  virtual void save(pqSettings&);

  /// Open a resource on the given server
  virtual void open(pqServer* server, const pqServerResource& resource);

signals:
  /// Signal emitted whenever the collection is changed
  void changed();
  /// Signal emitted whenever a new server connection is made
  void serverConnected(pqServer*);
  
private:
  pqServerResources(const pqServerResources&);
  pqServerResources& operator=(const pqServerResources&);
  
  class pqImplementation;
  pqImplementation* const Implementation;
  
  class pqMatchHostPath;
};

#endif
