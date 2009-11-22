/*=========================================================================

   Program: ParaView
   Module:    pqScalarBarVisibilityAdaptor.h

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
#ifndef __pqScalarBarVisibilityAdaptor_h
#define __pqScalarBarVisibilityAdaptor_h

#include <QObject>
#include "pqCoreExport.h"

class pqDataRepresentation;
class pqView;
class QAction;

/// pqScalarBarVisibilityAdaptor is an adptor that can be hooked on to
/// any action to make it control the scalar bar 
/// visibility of the scalar bar for the selected source 
/// in the selected render window.
/// TO_DEPRECATE: This class will be deprecated soon and replaced by
/// pqScalarBarVisibilityReaction.
class PQCORE_EXPORT pqScalarBarVisibilityAdaptor : public QObject
{
  Q_OBJECT
public:
  pqScalarBarVisibilityAdaptor(QAction* parent=0);
  virtual ~pqScalarBarVisibilityAdaptor();

signals:
  /// Fired when to indicate if the visibility of the scalar bar can
  /// be changed in the current setup.
  void canChangeVisibility(bool);

  /// Fired to update the scalarbar visibility state.
  void scalarBarVisible(bool);

public slots:
  /// Set the active display which this adaptor is going to
  /// show/hide the scalar bar for.
  void setActiveRepresentation(pqDataRepresentation *display);

protected slots:
  void updateState();
  void setScalarBarVisibility(bool visible);

protected:
  /// internal method called by updateState().
  void updateStateInternal();

private:
  pqScalarBarVisibilityAdaptor(const pqScalarBarVisibilityAdaptor&); // Not implemented.
  void operator=(const pqScalarBarVisibilityAdaptor&); // Not implemented.

  class pqInternal;
  pqInternal* Internal;
};


#endif

