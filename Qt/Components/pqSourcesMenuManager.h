/*=========================================================================

   Program: ParaView
   Module:    pqSourcesMenuManager.h

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
#ifndef __pqSourcesMenuManager_h 
#define __pqSourcesMenuManager_h

#include "pqProxyMenuManager.h"
class pqServer;

/// Manager for the filters menu.
/// Extends pqProxyMenuManager to add logic to enable/disable filters correctly.
class PQCOMPONENTS_EXPORT pqSourcesMenuManager : public pqProxyMenuManager
{
  Q_OBJECT
  typedef pqProxyMenuManager Superclass;
public:
  pqSourcesMenuManager(QMenu* menu);
  virtual ~pqSourcesMenuManager();

protected:
  virtual bool filter(const QString& name);

private:
  pqSourcesMenuManager(const pqSourcesMenuManager&); // Not implemented.
  void operator=(const pqSourcesMenuManager&); // Not implemented.
};

#endif


