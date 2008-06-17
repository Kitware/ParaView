/*=========================================================================

   Program: ParaView
   Module:    pqDisplayPanelDecoratorInterface.h

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
#ifndef __pqDisplayPanelDecoratorInterface_h 
#define __pqDisplayPanelDecoratorInterface_h

#include <QtPlugin>
#include "pqComponentsExport.h"
class pqDisplayPanel;

/// pqDisplayPanelDecoratorInterface is used to add decorators to display panels
/// created by ParaView. This makes it possible to added additional widgets to
/// existing display panels. This is typically useful for plugins that add new
/// painters to the rendering pipeline and want to expose some of the painter
/// parameters through the GUI.
/// Unlike other plugin interfaces, where ParaView stops after the first
/// interface implementation that handles the case, for this interface, every
/// registered implementation gets an opportunity to decorate the panel.
class pqDisplayPanelDecoratorInterface 
{
public:
  virtual ~pqDisplayPanelDecoratorInterface() {}

  /// Returns true if this implementation can decorate the given panel type.
  virtual bool canDecorate(pqDisplayPanel* panel) const = 0;

  /// Called to allow the implementation to decorate the panel. This is called
  /// only if canDecorate(panel) returns true.
  virtual void decorate(pqDisplayPanel* panel) const =0;
protected:
  pqDisplayPanelDecoratorInterface() {}

private:
  pqDisplayPanelDecoratorInterface(const pqDisplayPanelDecoratorInterface&); // Not implemented.
  void operator=(const pqDisplayPanelDecoratorInterface&); // Not implemented.
};

Q_DECLARE_INTERFACE(pqDisplayPanelDecoratorInterface, "com.kitware/paraview/displaypaneldecorator")

#endif


