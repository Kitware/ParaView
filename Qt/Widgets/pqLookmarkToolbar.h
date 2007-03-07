/*=========================================================================

   Program: ParaView
   Module:    pqLookmarkToolbar.h

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

========================================================================*/

/// \file pqLookmarkToolbar.h
/// \date 7/3/2006

#ifndef _pqLookmarkToolbar_h
#define _pqLookmarkToolbar_h


#include "QtWidgetsExport.h"

#include <QToolBar>

class QAction;
class QImage;


class QTWIDGETS_EXPORT pqLookmarkToolbar : public QToolBar
{
  Q_OBJECT

public:
  pqLookmarkToolbar(const QString &title, QObject* p=0);
  pqLookmarkToolbar(QObject* p=0);
  ~pqLookmarkToolbar(){}

public slots:
  void onLookmarkRemoved(const QString &name);
  void onLookmarkAdded(const QString &name, const QImage &icon);
  void onLookmarkNameChanged(const QString &oldName, const QString &newName);

  void showContextMenu(const QPoint &pos);
  void editCurrentLookmark();
  void removeCurrentLookmark();

protected slots:
  void onLoadLookmark(QAction*);

signals:
  void loadLookmark(const QString &name);
  void editLookmark(const QString &name);
  void removeLookmark(const QString &name);

protected:
  void connectActions();

private:
  QAction *ActionEdit;
  QAction *ActionRemove;
  QAction *CurrentLookmark;
};

#endif
