/*=========================================================================

   Program: ParaView
   Module:    pqNamedWidgets.h

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

#ifndef _pqNamedWidgets_h
#define _pqNamedWidgets_h

#include "pqSMProxy.h"

class QWidget;
class pqPropertyManager;

class pqNamedWidgets :
  public QObject
{
  Q_OBJECT
  
public:
  pqNamedWidgets();
  ~pqNamedWidgets();

  /// Link Qt widgets with server manager properties by name
  void link(QWidget* parent, pqSMProxy proxy);
  /// Remove links between Qt widgets and server manager properties
  void unlink(QWidget* parent, pqSMProxy proxy);

signals:
  /// Signal emitted when changes have been made to a property
  void propertyChanged();

public slots:
  /// Accept pending changes
  void accept();
  /// Reset pending changes
  void reset();

private slots:
  void onPropertyChanged(bool);

private:
  pqPropertyManager* const PropertyManager;
};

#endif

