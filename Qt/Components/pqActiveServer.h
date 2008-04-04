/*=========================================================================

   Program: ParaView
   Module:    pqActiveServer.h

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
#ifndef __pqActiveServer_h
#define __pqActiveServer_h


#include <QObject>
#include "pqComponentsExport.h"

class pqServer;

/// pqActiveServer keeps track of the active server.
/// It proivdes API to set/get the active server.
/// A signal is fired when the active server changes.
class PQCOMPONENTS_EXPORT pqActiveServer : public QObject
{
  Q_OBJECT
public:
  pqActiveServer(QObject* parent=0);
  virtual ~pqActiveServer();

  /// Returns the active server, may return NULL if 
  /// no active server is present.
  pqServer* current() const
    { return this->Current; }

public slots:
  /// Called to set the active server. Fires
  /// \c changed().
  void setCurrent(pqServer*);
    
signals:
  /// Fired when the active server changes.
  void changed(pqServer*);

private:
  pqServer* Current;
};

#endif

