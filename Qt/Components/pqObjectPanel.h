/*=========================================================================

   Program: ParaView
   Module:    pqObjectPanel.h

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

#ifndef _pqObjectPanel_h
#define _pqObjectPanel_h

#include "pqComponentsExport.h"
#include "pqProxyPanel.h"

class pqPropertyManager;

/// Base class for Widget which provides an editor for editing properties
/// of a proxy
class PQCOMPONENTS_EXPORT pqObjectPanel : public pqProxyPanel
{
  Q_OBJECT
public:
  /// constructor
  pqObjectPanel(pqProxy* proxy, QWidget* p);
  /// destructor
  ~pqObjectPanel();

  pqProxy* referenceProxy() const;
  
public slots:
  /// Fires modified
  virtual void setModified();
  
  /// accept the changes made to the properties
  /// changes will be propogated down to the server manager
  /// subclasses should only change properties when accept is called to work
  /// properly with undo/redo
  virtual void accept();
  
  /// reset the changes made
  /// editor will query properties from the server manager
  virtual void reset();

protected:

  pqProxy* ReferenceProxy;

};

#endif

