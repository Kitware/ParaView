/*=========================================================================

   Program: ParaView
   Module:    pqExtractLocationsPanel.h

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
#ifndef __pqExtractLocationsPanel_h
#define __pqExtractLocationsPanel_h


#include "pqObjectPanel.h"

/// pqExtractLocationsPanel is a custom panel used to type in locations to
// analyze in ParaView.
class PQCOMPONENTS_EXPORT pqExtractLocationsPanel : public pqObjectPanel
{
  Q_OBJECT
  typedef pqObjectPanel Superclass;
public:
  pqExtractLocationsPanel(pqProxy* proxy, QWidget* parent=0);
  ~pqExtractLocationsPanel();

  /// Called when the panel becomes active. 
  virtual void select();

protected slots:
  /// Deletes selected elements.
  void deleteSelected();

  /// Deletes all elements.
  void deleteAll();

  // Adds a new value.
  void newValue();

protected:
  /// Creates links between the Qt widgets and the server manager properties.
  void linkServerManagerProperties();

private:
  pqExtractLocationsPanel(const pqExtractLocationsPanel&); // Not implemented.
  void operator=(const pqExtractLocationsPanel&); // Not implemented.


  class pqInternal;
  pqInternal *Internal;
};

#endif

