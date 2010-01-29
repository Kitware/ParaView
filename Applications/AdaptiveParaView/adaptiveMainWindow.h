/*=========================================================================

   Program: ParaView
   Module:    adaptiveMainWindow.h

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
#ifndef __adaptiveMainWindow_h 
#define __adaptiveMainWindow_h

#include <QMainWindow>

class pqView;

/// MainWindow for the default ParaView application.
class adaptiveMainWindow : public QMainWindow
{
  Q_OBJECT
  typedef QMainWindow Superclass;
public:
  adaptiveMainWindow();
  ~adaptiveMainWindow();

signals:
  void setMessage(const QString&);

protected slots:
  void showHelpForProxy(const QString& proxyname);

  /// Called when the active view in the pqActiveView singleton changes.
  virtual void onActiveViewChanged(pqView* view);

  // Every render has a chance to schedule additional renders if multipass is 
  // needed
  void scheduleNextPass();
  void stopAdaptive();

protected:

  bool StopAdaptive;
  int Pass;

private:
  adaptiveMainWindow(const adaptiveMainWindow&); // Not implemented.
  void operator=(const adaptiveMainWindow&); // Not implemented.

  class pqInternals;
  pqInternals* Internals;
};

#endif


