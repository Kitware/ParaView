/*=========================================================================

   Program: ParaView
   Module:    pqActiveViewOptionsManager.h

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

/// \file pqActiveViewOptionsManager.h
/// \date 7/27/2007

#ifndef _pqActiveViewOptionsManager_h
#define _pqActiveViewOptionsManager_h


#include "pqComponentsExport.h"
#include <QObject>

class pqActiveViewOptions;
class pqActiveViewOptionsManagerInternal;
class pqView;
class QString;
class QWidget;


class PQCOMPONENTS_EXPORT pqActiveViewOptionsManager : public QObject
{
  Q_OBJECT

public:
  pqActiveViewOptionsManager(QObject *parent=0);
  virtual ~pqActiveViewOptionsManager();

  void setMainWindow(QWidget *parent);
  void setRenderViewOptions(pqActiveViewOptions *renderOptions);

  bool registerOptions(const QString &viewType, pqActiveViewOptions *options);
  void unregisterOptions(pqActiveViewOptions *options);
  bool isRegistered(pqActiveViewOptions *options) const;

  pqActiveViewOptions *getOptions(const QString &viewType) const;

public slots:
  void setActiveView(pqView *view);
  void showOptions();

private slots:
  void removeCurrent(pqActiveViewOptions *options);

private:
  pqActiveViewOptions *getCurrent() const;

private:
  pqActiveViewOptionsManagerInternal *Internal;
};

#endif
