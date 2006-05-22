/*=========================================================================

   Program:   ParaQ
   Module:    pqClipPanel.h

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

#ifndef _pqClipPanel_h
#define _pqClipPanel_h

#include "pqWidgetObjectPanel.h"

/// Custom panel for the Clip filter that manages a 3D widget for interactive clipping
class pqClipPanel :
  public pqWidgetObjectPanel
{
  Q_OBJECT
  
public:
  pqClipPanel(QWidget* p);
  ~pqClipPanel();
  
protected:
  virtual void setProxyInternal(pqSMProxy p);

private slots:
  /// Called if any of the Qt widget values is modified
  void onQtWidgetChanged();
  /// Called if the user accepts pending modifications
  void onAccepted();
  /// Called if the user rejects pending modifications
  void onRejected();

private:
  /// Called when the 3D widget values are modified
  void on3DWidgetChanged();

  /// Pulls the current values from the implicit plane, pushing them into the Qt and 3D widgets  
  void pullImplicitPlane();
  /// Pushes values into the Qt widgets
  void updateQtWidgets(const double* origin, const double* normal);
  /// Pushes values into the 3D widget
  void update3DWidget(const double* origin, const double* normal);
  /// Pushes values into the implicit plane
  void pushImplicitPlane(const double* origin, const double* normal);

  /// Used to avoid recursion when updating the Qt widgets  
  bool IgnoreQtWidgets;
  /// Used to avoid recursion when updating the 3D widget
  bool Ignore3DWidget;
};

#endif
