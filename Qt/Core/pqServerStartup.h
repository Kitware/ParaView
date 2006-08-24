/*=========================================================================

   Program: ParaView
   Module:    pqServerStartup.h

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

#ifndef _pqServerStartup_h
#define _pqServerStartup_h

#include "pqServerResource.h"

#include <QDomDocument>
#include <QMap>

class pqServerStartupContext;

/////////////////////////////////////////////////////////////////////////////
// pqServerStartup

/// Abstract interface for an object that can start a remote server
class pqServerStartup
{
public:
  virtual ~pqServerStartup() {}

  /// Returns the name of this startup
  virtual const QString name() = 0;
  /// Returns the server for this startup
  virtual const pqServerResource server() = 0;
  /// Returns the user who owns this startup
  virtual const QString owner() = 0;
  /// Returns an XML description of the configuration for this startup
  virtual const QDomDocument configuration() = 0;
  
  /// Defines a generic collection of name-value-pair "options" that will be
  /// set by the user prior to server startup
  typedef QMap<QString, QString> OptionsT;
    
  /** Begins (asynchronous) execution of the startup procedure.  Callers should
  create a pqServerStartupContext object to pass to this method, and connect
  to its startupSucceed() and startupFailed() signals to receive notification
  that the startup procedure has been completed. */
  virtual void execute(const OptionsT& options, pqServerStartupContext& context) = 0;
  
protected:
  pqServerStartup() {}
  pqServerStartup(const pqServerStartup&) {}
  pqServerStartup& operator=(const pqServerStartup&) { return *this; }
};

#endif // !_pqServerStartup_h

