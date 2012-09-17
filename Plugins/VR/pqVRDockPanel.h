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
#ifndef __pqVRDockPanel_h
#define __pqVRDockPanel_h

#include <QtGui/QDockWidget>

class pqView;
class QListWidgetItem;
class vtkSMProxy;

class pqVRDockPanel : public QDockWidget
{
  Q_OBJECT
  typedef QDockWidget Superclass;
public:
  pqVRDockPanel(const QString &t, QWidget* p = 0, Qt::WindowFlags f=0):
    Superclass(t, p, f) { this->constructor(); }
  pqVRDockPanel(QWidget *p=0, Qt::WindowFlags f=0):
    Superclass(p, f) { this->constructor(); }
  virtual ~pqVRDockPanel();

protected slots:
  void addConnection();
  void removeConnection();
  void updateConnections();
  void connectionDoubleClicked(QListWidgetItem *);

  void addStyle();
  void removeStyle();
  void updateStyles();
  void styleDoubleClicked(QListWidgetItem *);

  void proxyChanged(vtkSMProxy*);
  void setActiveView(pqView*);

  void saveState();
  void restoreState();

  void updateDebugLabel();

private:
  Q_DISABLE_COPY(pqVRDockPanel)

  class pqInternals;
  pqInternals* Internals;
  void constructor();
};

#endif
