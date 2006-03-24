/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

#ifndef _pqServer_h
#define _pqServer_h

class pqOptions;
class vtkProcessModule;
class vtkSMApplication;
class vtkSMProxyManager;
class vtkSMMultiViewRenderModuleProxy;

#include "QtWidgetsExport.h"
#include <QObject>
#include <QString>

/// Abstracts the concept of a "server connection" so that ParaQ clients may: have more than one connect at a time / open and close connections at-will
class QTWIDGETS_EXPORT pqServer : public QObject
{
public:
  /// Constructs a standalone or "built-in" server connection, returns NULL on failure
  static pqServer* CreateStandalone();
  /// Constructs a server connection to a remote host, returns NULL on failure
  static pqServer* CreateConnection(const char* const hostName, const int portNumber);
  virtual ~pqServer();

  QString getAddress() const;
  const QString& getFriendlyName() const {return this->FriendlyName;}
  void setFriendlyName(const QString& name);

  vtkProcessModule* GetProcessModule();
  static vtkSMProxyManager* GetProxyManager();
  vtkSMMultiViewRenderModuleProxy* GetRenderModule();

private:
  pqServer(pqOptions*, vtkProcessModule*, vtkSMApplication*, vtkSMMultiViewRenderModuleProxy*);
  pqServer(const pqServer&);
  pqServer& operator=(const pqServer&);

  QString FriendlyName;
  pqOptions* const Options;
  vtkProcessModule* const ProcessModule;
  static vtkSMApplication* ServerManager;
  vtkSMMultiViewRenderModuleProxy* const RenderModule;
};

#endif // !_pqServer_h

