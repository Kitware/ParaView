/*=========================================================================

   Program: ParaView
   Module:    pqAdaptiveMainWindowCore.h

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

#ifndef _pqAdaptiveMainWindowCore_h
#define _pqAdaptiveMainWindowCore_h

#include <QObject>
#include "pqMainWindowCore.h"

/** \brief Provides a standardized main window for ParaView applications -
application authors can derive from pqMainWindowCore and call its member functions
to use as-much or as-little of the standardized functionality as desired */

class pqAdaptiveMainWindowCore :
  public pqMainWindowCore
{
  Q_OBJECT
  typedef pqMainWindowCore Superclass;

public:
  pqAdaptiveMainWindowCore();
  ~pqAdaptiveMainWindowCore();

  /// Setup a proxy tab widget, attaching it to the given dock
  virtual pqProxyTabWidget* setupProxyTabWidget(QDockWidget* parent);
  
signals:
  //called to put information messages on the UI. The main app wathces this
  void setMessage(const QString&);

protected slots:

  /// Called when the active view in the pqActiveView singleton changes.
  // this turns on automatic rerendering to drive streaming
  virtual void onActiveViewChanged(pqView* view);

  // this is called after every render to schedule the next, if needed
  void scheduleNextPass();

  // can accept signal tells streaming it stop what it is doing
  void stopAdaptive();
  virtual void onPostAccept();
  
  //disable auto show
  virtual void onRemovingSource(pqPipelineSource *source);
protected:

  int Pass;
};

#endif // !_pqAdaptiveMainWindowCore_h

