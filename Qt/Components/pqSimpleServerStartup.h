/*=========================================================================

   Program: ParaView
   Module:    pqSimpleServerStartup.h

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
#ifndef __pqSimpleServerStartup_h
#define __pqSimpleServerStartup_h

#include "pqComponentsExport.h"

#include <QDialog>

class pqServer;
class pqServerResource;
class pqServerStartups;
class pqSettings;

/**
Convenience class that handles the entire process of connecting to a server ...
callers should create an instance of pqSimpleServerStartup, and
call the startServer() method to begin the connection process.  Note that
startServer() is asynchronous - the client must wait until serverCancelled(),
serverFailed(), or serverStarted() is emitted to know whether startup
was successful.  It is the caller's responsibility to ensure that the
pqSimpleServerStartup object does not go out of scope until one of the three
signals is emitted.

If necessary, the user will be prompted to configure how to start the server,
and optionally prompted for site-specific runtime parameters.

A modal dialog will be displayed while the server starts.
*/
class PQCOMPONENTS_EXPORT pqSimpleServerStartup :
  public QObject
{
  typedef QObject Superclass;
  
  Q_OBJECT
  
public:
  pqSimpleServerStartup(
    pqSettings& settings,
    pqServerStartups& startups,
    QObject* parent = 0);
    
  ~pqSimpleServerStartup();

  void startServer(const pqServerResource& resource);

signals:
  /// Signal emitted if the user cancels startup
  void serverCancelled();
  /// Signal emitted if the server fails to start
  void serverFailed();
  /// Signal emitted if the server successfully starts
  void serverStarted(pqServer*);

private slots:
  void forwardConnectServer();
  void monitorReverseConnections();
  void reverseConnection(pqServer*);

private:
  class pqImplementation;
  pqImplementation* const Implementation;
  
  void startBuiltinConnection();
  void startForwardConnection();
  void startReverseConnection();
};

#endif
