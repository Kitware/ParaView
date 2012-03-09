/*=========================================================================

   Program: ParaView
   Module:    pqRecentFilesMenu.h

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

#ifndef _pqRecentFilesMenu_h
#define _pqRecentFilesMenu_h

#include "pqComponentsExport.h"

#include <QObject>

class pqServer;
class pqServerResource;
class QAction;
class QMenu;

/** Displays a collection of recently-used files (server resources)
as a menu, sorted in most-recently-used order and grouped by server */
class PQCOMPONENTS_EXPORT pqRecentFilesMenu :
  public QObject
{
  Q_OBJECT

public:
  /// Assigns the menu that will display the list of files
  pqRecentFilesMenu(QMenu& menu, QObject* p=0);
  virtual ~pqRecentFilesMenu();

  /// Open a resource on the given server
  virtual bool open(
    pqServer* server, const pqServerResource& resource) const;

private slots:
  void onResourcesChanged();
  void onOpenResource(QAction*);
  void onOpenResource();
  void onServerStarted(pqServer*);

private:
  pqRecentFilesMenu(const pqRecentFilesMenu&);
  pqRecentFilesMenu& operator=(const pqRecentFilesMenu&);

  class pqImplementation;
  pqImplementation* const Implementation;  
};

#endif // !_pqRecentFilesMenu_h
