/*=========================================================================

   Program: ParaView
   Module:    pqExtractSelectionsPanel.h

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
#ifndef __pqExtractSelectionsPanel_h
#define __pqExtractSelectionsPanel_h


#include "pqObjectPanel.h"
class vtkSMSourceProxy;

/// pqExtractSelectionsPanel is a custom panel used for 
/// "ExtractCellSelection" and "ExtractPointSelection" filters.
class PQCOMPONENTS_EXPORT pqExtractSelectionsPanel : public pqObjectPanel
{
  Q_OBJECT
  typedef pqObjectPanel Superclass;
public:
  pqExtractSelectionsPanel(pqProxy* proxy, QWidget* parent=0);
  ~pqExtractSelectionsPanel();
  
public slots:
  /// accept the changes made to the properties
  /// changes will be propogated down to the server manager
  /// subclasses should only change properties when accept is called to work
  /// properly with undo/redo
  virtual void accept();

protected slots:
  // Copy active selection.
  void copyActiveSelection();

  void onActiveSelectionChanged();

  /// This method gets called to refresh all domains 
  /// and information properties. Subclass can override
  /// this to update any domain related entities.
  /// Since this is not a particularly fast operation, we update 
  /// the information and domains only when the panel is selected 
  /// or an already active panel is accepted. 
  virtual void updateInformationAndDomains();

protected:
  void linkServerManagerProperties();
  void updateLabels();
  void setSelectionSource(vtkSMSourceProxy *selSource);

private:
  pqExtractSelectionsPanel(const pqExtractSelectionsPanel&); // Not implemented.
  void operator=(const pqExtractSelectionsPanel&); // Not implemented.


  class pqInternal;
  pqInternal *Internal;
};

#endif

