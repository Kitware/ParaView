/*=========================================================================

   Program: ParaView
   Module:    pqScalarBarRepresentation.h

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
#ifndef __pqScalarBarRepresentation_h
#define __pqScalarBarRepresentation_h

#include "pqRepresentation.h"

class pqPipelineRepresentation;
class pqScalarsToColors;
class vtkUndoElement;

// PQ object for a scalar bar. Keeps itself connected with the pqScalarsToColors
// object, if any.
class PQCORE_EXPORT pqScalarBarRepresentation : public pqRepresentation
{
  Q_OBJECT

  typedef pqRepresentation Superclass;
public:
  pqScalarBarRepresentation(const QString& group, const QString& name,
    vtkSMProxy* scalarbar, pqServer* server,
    QObject* parent=0);
  virtual ~pqScalarBarRepresentation();

  /// Get the lookup table this scalar bar shows, if any.
  pqScalarsToColors* getLookupTable() const;

  /// A scalar bar title is divided into two parts (any of which can be empty).
  /// Typically the first is the array name and the second is the component.
  /// This method returns the pair.
  QPair<QString, QString> getTitle() const;
  
  /// Set the title formed by combining two parts.
  void setTitle(const QString& name, const QString& component);

  /// Set the visibility. Note that this affects the visibility of the
  /// display in the view it has been added to, if any. This method does not 
  /// call a re-render on the view, caller must call that explicitly.
  virtual void setVisible(bool visible);

  /// set by pqPipelineRepresentation when it forces the visibility of the
  /// scalar bar to be off.
  void setAutoHidden(bool h)
    { this->AutoHidden = h; }
  bool getAutoHidden() const
    { return this->AutoHidden; }

  virtual void setDefaultPropertyValues();

signals:
  /// Fired just before the color is changed on the underlying proxy.
  /// This must be hooked to an undo stack to record the
  /// changes in a undo set.
  void begin(const QString&);

  /// Fired just after the color is changed on the underlying proxy.
  /// This must be hooked to an undo stack to record the
  /// changes in a undo set.
  void end();
  
  /// For undo-stack.
  void addToActiveUndoSet(vtkUndoElement* element);

protected slots:
  void onLookupTableModified();

  void startInteraction();
  void endInteraction();

protected:
  /// flag set to true, when the scalarbar has been hidden by
  /// pqPipelineRepresentation and not explicitly by the user. Used to restore
  /// scalar bar visibility when the representation becomes visible.
  bool AutoHidden;

private:
  class pqInternal;
  pqInternal* Internal;
};



#endif

