/*=========================================================================

   Program: ParaView
   Module:    pqShellServerStartup.h

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

#ifndef _pqShellServerStartup_h
#define _pqShellServerStartup_h

#include "pqServerStartup.h"

#include <QProcess>

/////////////////////////////////////////////////////////////////////////////
// pqShellServerStartup

/// Concrete implementation of pqServerStartup that runs an external
/// shell command to start a remote server.
class pqShellServerStartup :
  public pqServerStartup
{
public:
  pqShellServerStartup(const QString& command_line, double delay);
  
  void execute(const pqServerResource& server, pqServerStartupContext& context);

  const QString CommandLine;
  const double Delay;
};

/// Private implementation detail
class pqShellServerStartupContextHelper :
  public QObject
{
  Q_OBJECT
  
signals:
  void succeeded();
  void failed();

private slots:
  void onFinished(int exitCode, QProcess::ExitStatus exitStatus);
  void onError(QProcess::ProcessError error);
  void onDelayComplete();
  
private:
  pqShellServerStartupContextHelper(double Delay, QObject* parent);
  
  friend class pqShellServerStartup;
  
  const double Delay;
};

#endif // !_pqShellServerStartup
