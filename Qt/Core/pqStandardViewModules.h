/*=========================================================================

   Program: ParaView
   Module:    pqStandardViewModules.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/

#ifndef _pqStandardViewModules_h
#define _pqStandardViewModules_h

#include "pqCoreExport.h"
#include "pqViewModuleInterface.h"
#include <QObject>

/// interface class for plugins that create view modules
class PQCORE_EXPORT pqStandardViewModules : public QObject, 
                                            public pqViewModuleInterface
{
  Q_OBJECT
  Q_INTERFACES(pqViewModuleInterface)
public:
  
  pqStandardViewModules(QObject* o);
  ~pqStandardViewModules();

  virtual QStringList viewTypes() const;
  QStringList displayTypes() const;
  QString viewTypeName(const QString&) const;

  bool canCreateView(const QString& viewtype) const;
  
  vtkSMProxy* createViewProxy(const QString& viewtype,
                              pqServer *server);

  pqView* createView(const QString& viewtype,
    const QString& group,
    const QString& name,
    vtkSMViewProxy* viewmodule,
    pqServer* server,
    QObject* parent);
  
  pqDataRepresentation* createDisplay(const QString& display_type, 
    const QString& group,
    const QString& name,
    vtkSMProxy* proxy,
    pqServer* server,
    QObject* parent);

};

#endif

