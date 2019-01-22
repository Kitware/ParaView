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
#ifndef pqVRAddConnectionDialog_h
#define pqVRAddConnectionDialog_h

#include <QDialog>

#include "vtkPVVRConfig.h"

#if PARAVIEW_PLUGIN_VRPlugin_USE_VRPN
class pqVRPNConnection;
#endif
#if PARAVIEW_PLUGIN_VRPlugin_USE_VRUI
class pqVRUIConnection;
#endif

class pqVRAddConnectionDialog : public QDialog
{
  Q_OBJECT
  typedef QDialog Superclass;

public:
  pqVRAddConnectionDialog(QWidget* parent = 0, Qt::WindowFlags f = 0);
  virtual ~pqVRAddConnectionDialog();

#if PARAVIEW_PLUGIN_VRPlugin_USE_VRPN
  void setConnection(pqVRPNConnection* conn);
  pqVRPNConnection* getVRPNConnection();
  bool isVRPN();
#endif
#if PARAVIEW_PLUGIN_VRPlugin_USE_VRUI
  void setConnection(pqVRUIConnection* conn);
  pqVRUIConnection* getVRUIConnection();
  bool isVRUI();
#endif

  void updateConnection();

public slots:
  void accept();

protected:
  void keyPressEvent(QKeyEvent*);

private slots:
  void addInput();
  void removeInput();

  void connectionTypeChanged();

private:
  Q_DISABLE_COPY(pqVRAddConnectionDialog)

  class pqInternals;
  pqInternals* Internals;
};

#endif
