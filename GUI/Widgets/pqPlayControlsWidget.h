/*=========================================================================

   Program:   ParaQ
   Module:    pqPlayControlsWidget.h

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

#ifndef _pqPlayControlsWidget_h
#define _pqPlayControlsWidget_h

#include "QtWidgetsExport.h"

#include "qwidget.h"

class QHBoxLayout;
class QToolButton;

class QTWIDGETS_EXPORT pqPlayControlsWidget :
  public QWidget
{
  Q_OBJECT

public:
  pqPlayControlsWidget(QWidget *parent);
  ~pqPlayControlsWidget();

signals:
  void play();
  void pause();
  void forward();
  void back();
  void first();
  void last();

  void showTip(const QString&);
  void removeTip();

private: 
  enum {
    FIRST,
    BACK,
    FORWARD,
    LAST,
//    PAUSE,
//    PLAY,
    NUM_BUTTONS
  };

  static const char *Name[NUM_BUTTONS];
  static const char *Image[NUM_BUTTONS];

  QToolButton *Button[NUM_BUTTONS];
  QHBoxLayout *Layout;
};

#endif


