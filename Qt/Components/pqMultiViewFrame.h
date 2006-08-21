/*=========================================================================

   Program: ParaView
   Module:    pqMultiViewFrame.h

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

#ifndef _pqMultiViewFrame_h
#define _pqMultiViewFrame_h

#include <QWidget>
#include "pqComponentsExport.h"
#include "ui_pqMultiViewFrameMenu.h"

/// a holder for a widget in a multiview
class PQCOMPONENTS_EXPORT pqMultiViewFrame : public QWidget, public Ui::MultiViewFrameMenu
{
  Q_OBJECT
  Q_PROPERTY(bool menuAutoHide READ menuAutoHide WRITE setMenuAutoHide)
  Q_PROPERTY(bool active WRITE setActive READ active)
  Q_PROPERTY(QColor borderColor WRITE setBorderColor READ borderColor)
public:
  pqMultiViewFrame(QWidget* parent = NULL);
  ~pqMultiViewFrame();

  /// sets the window title in the title bar and the widget.
  void setTitle(const QString& title);

  /// whether the menu is auto hidden
  bool menuAutoHide() const;
  /// whether the menu is auto hidden
  void setMenuAutoHide(bool);

  /// set the main widget for this holder
  void setMainWidget(QWidget*);
  /// get the main widget for this holder
  QWidget* mainWidget();
  
  /// get whether active, if active, a border is drawn
  bool active() const;
  /// get the color of the border
  QColor borderColor() const;

  void hideMenu(bool hide);

public slots:

  /// close this frame, emits closePressed() so receiver does the actual remove
  void close();
  /// maximize this frame, emits closePressed() so receiver does the actual maximize
  void maximize();
  /// split this frame vertically, emits splitVerticalPressed so receiver does the actual split
  void splitVertical();
  /// split this frame horizontally, emits splitVerticalPressed so receiver does the actual split
  void splitHorizontal();
  /// sets the border color
  void setBorderColor(QColor);
  /// sets whether this frame is active.  if active, a border is drawn
  void setActive(bool);

signals:
  /// signal active state changed
  void activeChanged(bool);
  /// signal close pressed
  void closePressed();
  /// signal maximize pressed
  void maximizePressed();
  /// signal split vertical pressed
  void splitVerticalPressed();
  /// signal split horizontal pressed
  void splitHorizontalPressed();

protected:
  void paintEvent(QPaintEvent* e);

private:
  QWidget* MainWidget;
  bool AutoHide;
  bool Active;
  bool MenuHidden;
  QColor Color;
  QWidget* Menu;
};

#endif //_pqMultiViewFrame_h

