/*=========================================================================

  Program: ParaView
  Module:  vtkVRConnectionManager.h

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

=========================================================================*/
#ifndef __vtkVRConnectionManager_h
#define __vtkVRConnectionManager_h
// --------------------------------------------------------------------includes
#include <QObject>

// -----------------------------------------------------------------pre-defines
class vtkVRQueue;
class vtkPVXMLElement;
class vtkSMProxyLocator;
class vtkVRPNConnection;
class vtkVRUIConnection;

// -----------------------------------------------------------------------class
class vtkVRConnectionManager: public QObject
{
  Q_OBJECT
  typedef QObject Superclass;
public:
  // ............................................................public-methods
  vtkVRConnectionManager(vtkVRQueue* quque, QObject* parent=0);
  virtual ~vtkVRConnectionManager();

  void add( vtkVRPNConnection* conn );
  void remove ( vtkVRPNConnection *conn );
  void add( vtkVRUIConnection* conn );
  void remove ( vtkVRUIConnection *conn );

  void clear();

public slots:
  /// start/stop connections
  void start();
  void stop();

  /// Clears current connections and loads a new set of connections from the XML
  /// Configuration
  void configureConnections( vtkPVXMLElement* xml, vtkSMProxyLocator* locator );

  // save the connection configuration
  void saveConnectionsConfiguration( vtkPVXMLElement* root );

protected:
  // ...........................................................protected-ivars

protected:
//BTX
  // .......................................................................BTX
private:
  Q_DISABLE_COPY(vtkVRConnectionManager);
  class pqInternals;
  pqInternals* Internals;

//ETX
  // .......................................................................ETX
};

#endif // __vtkVRConnectionManager_h
