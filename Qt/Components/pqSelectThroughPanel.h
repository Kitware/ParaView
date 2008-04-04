/*=========================================================================

   Program: ParaView
   Module:    pqSelectThroughPanel.h

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

#ifndef _pqSelectThroughPanel_h
#define _pqSelectThroughPanel_h

#include "pqObjectPanel.h"
class pqRubberBandHelper;
class pqView;

/// Custom panel for frustum selection.
class PQCOMPONENTS_EXPORT pqSelectThroughPanel :
  public pqObjectPanel
{
  typedef pqObjectPanel Superclass;

  Q_OBJECT

public:
  pqSelectThroughPanel(pqProxy* proxy, QWidget* p);
  ~pqSelectThroughPanel();

public slots:

  /// called when rubber band creation finished
  void startSelect();
  void endSelect();

  /// accept changes made by this panel overridden to push frustum values over
  /// necessary because frustum is not yet tied to a widget
  virtual void accept();

  /// Used to keep track of active render module
  void setActiveView(pqView*);

private:

  int Mode;
  double *Verts;
  class pqImplementation;
  pqImplementation* const Implementation;
  pqRubberBandHelper *RubberBandHelper;
};

#endif

