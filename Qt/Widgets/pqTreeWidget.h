/*=========================================================================

   Program: ParaView
   Module:    pqTreeWidget.h

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

#ifndef _pqTreeWidget_h
#define _pqTreeWidget_h

#include "QtWidgetsExport.h"
#include <QTreeWidget>

/**
  A convenience QTreeWidget with extra features:
  1.  Automatic size hints based on contents
  2.  A check box added in a header if items have check boxes
*/
class QTWIDGETS_EXPORT pqTreeWidget : public QTreeWidget
{
  typedef QTreeWidget Superclass;
  Q_OBJECT
public:
  
  pqTreeWidget(QWidget* p);
  ~pqTreeWidget();

  bool event(QEvent* e);

  /// give a hint on the size
  QSize sizeHint() const;
  QSize minimumSizeHint() const;
  
public slots:
  void allOn();
  void allOff();

protected slots:
  void doToggle(int col);
  void updateCheckState();
  void invalidateLayout();

protected:
  QPixmap** CheckPixmaps;
  QPixmap pixmap(Qt::CheckState state, bool active);
};

#endif // !_pqTreeWidget_h

