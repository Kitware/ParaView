/*=========================================================================

   Program: ParaView
   Module:    pqCollapsedGroup.h

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

#ifndef _pqCollapsedGroup_h
#define _pqCollapsedGroup_h

#include "QtWidgetsExport.h"
#include <QGroupBox>

class QPushButton;

class QTWIDGETS_EXPORT pqCollapsedGroup :
  public QGroupBox
{
  typedef QDialog base;
  
  Q_OBJECT
  
public:
  /// the filter is a string of semi-colon separated filters
  pqCollapsedGroup(QWidget* parent = 0);
  pqCollapsedGroup(const QString& title, QWidget* parent = 0);
  ~pqCollapsedGroup();
  virtual bool eventFilter(QObject* watched, QEvent* e);

signals:

protected:
  
protected slots:
  void buttonPressed();

private:
  pqCollapsedGroup(const pqCollapsedGroup&);
  pqCollapsedGroup& operator=(const pqCollapsedGroup&);

  void initialize();

  bool Hidden;
  int OldMargin;
  QPushButton* HideButton;
  QIcon* HideIcon;
  QIcon* ShowIcon;
};

#endif // !_pqCollapsedGroup_h

