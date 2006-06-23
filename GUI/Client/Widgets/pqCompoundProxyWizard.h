/*=========================================================================

   Program: ParaView
   Module:    pqCompoundProxyWizard.h

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


#ifndef _pqCompoundProxyWizard_h
#define _pqCompoundProxyWizard_h

#include <QtGui/QDialog>
#include "ui_pqCompoundProxyWizard.h"
#include "pqWidgetsExport.h"

class pqServer;

/// wizard to manage loading/saving of compound proxies
class PQWIDGETS_EXPORT pqCompoundProxyWizard : public QDialog, 
                                               public Ui::pqCompoundProxyWizard
{
  Q_OBJECT
public:
  /// constructor
  pqCompoundProxyWizard(pqServer* s, QWidget* p = 0, Qt::WFlags f = 0);
  /// destructor
  ~pqCompoundProxyWizard();

public slots:
  /// slot for when the load button is pushed
  void onLoad();
  /// slot to handle loading a list of files
  void onLoad(const QStringList& files);
  /// slot to handle unloading a compound proxy
  void onRemove();
  /// slot to handle adding a file/proxy to the list view
  void addToList(const QString& filename, const QString& proxy);

signals:
  /// signal emitted when a new compound proxy is loaded
  void newCompoundProxy(const QString& filename, const QString& proxy);

private:
  pqServer* Server;

};

#endif // _pqCompoundProxyWizard_h

