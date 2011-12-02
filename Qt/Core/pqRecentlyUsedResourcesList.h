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
#ifndef __pqRecentlyUsedResourcesList_h 
#define __pqRecentlyUsedResourcesList_h

#include <QObject>
#include <QList>
#include "pqServerResource.h"

class pqSettings;
/// pqRecentlyUsedResourcesList encapsulates a persistent collection of
/// recently-used resources (data files or state files).
///
/// \sa pqServerResource
class PQCORE_EXPORT pqRecentlyUsedResourcesList : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;
public:
  pqRecentlyUsedResourcesList(QObject* parent=0);
  virtual ~pqRecentlyUsedResourcesList();

  /// convenience typedef.
  typedef QList<pqServerResource> ListT;

  /// Add a resource to the collection. Moves the resource to the beginning of
  /// the list.
  void add(const pqServerResource& resource);

  /// Returns the contents of the collection ordered from most-recently-used to
  /// least-recently-used.
  const QList<pqServerResource>& list() const
    { return this->ResourceList; }

  /// Load the collection (from local user preferences)
  void load(pqSettings&);

  /// Save the collection (to local user preferences)
  void save(pqSettings&) const;

signals:
  /// Signal emitted whenever the collection is changed
  void changed();

private:
  QList<pqServerResource> ResourceList;

  Q_DISABLE_COPY(pqRecentlyUsedResourcesList)
};

#endif
