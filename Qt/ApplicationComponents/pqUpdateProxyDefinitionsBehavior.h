/*=========================================================================

   Program: ParaView
   Module:    pqUpdateProxyDefinitionsBehavior.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
#ifndef __pqUpdateProxyDefinitionsBehavior_h
#define __pqUpdateProxyDefinitionsBehavior_h

#include <QObject>
#include <QSet>

#include "pqApplicationComponentsExport.h"

class pqProxyGroupMenuManager;

/// @ingroup Behaviors
/// pqUpdateProxyDefinitionsBehavior is associated with a
/// pqProxyGroupMenuManager. When created, it populates the
/// pqProxyGroupMenuManager with new proxy definitions that get added to a
/// specified group(or groups) automatically.
/// This also ensures that user-defined custom-filters are always shown in the
/// menu.
/// This also now ensures that user-defined custom-filter are removed when unloaded
/// from the application.
/// Note: We should think about renaming to pqUpdateProxyDefinitionsBehavior
/// I am leaning towards not automatically added new filters/sources since large
/// plugins can bring in a plethora of filters, many of which may be internal.
class PQAPPLICATIONCOMPONENTS_EXPORT pqUpdateProxyDefinitionsBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;
public:
  enum eMode
    {
    SOURCES, // proxies that don't have inputs
    FILTERS, // proxies that have inputs
    ANY
    };

  /// menuManager cannnot be NULL.
  pqUpdateProxyDefinitionsBehavior(eMode mode, const QString& xmlgroup,
    pqProxyGroupMenuManager* menuManager);

protected slots:
  /// This slot is called after plugins are loaded or after custom-filter
  /// definitions are added.
  void update();

  /// This slot is called after custom-filter definitions are removed.
  void remove(QString name);

protected:
  pqProxyGroupMenuManager* MenuManager;
  QSet<QString> AlreadySeenSet;
  QString XMLGroup;
  eMode Mode;

private:
  Q_DISABLE_COPY(pqUpdateProxyDefinitionsBehavior)

};

#endif
