/*=========================================================================

   Program: ParaView
   Module:    pqPendingDisplayManager.h

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
#ifndef __pqPendingDisplayManager_h
#define __pqPendingDisplayManager_h

#include "pqCoreExport.h"
#include <QObject>

class pqView;
class pqPendingDisplayUndoElement;
class pqPipelineSource;
class pqUndoStack;

/// This class helps manage deferred displays for an application.
/// An application may add sources for which displays will eventually 
/// be made.  This also works with undo/redo, so if the deletion of
/// a display is done at undo time, it'll put the display back on
/// the pending display list, and a redo/accept can re-create it.
class PQCORE_EXPORT pqPendingDisplayManager  : public QObject
{
  Q_OBJECT
public:

  pqPendingDisplayManager(QObject* p = 0);
  ~pqPendingDisplayManager();

  /// Get/Set the undo stack that should be used to add
  /// pending display undo elements.
  void setUndoStack(pqUndoStack*);
  pqUndoStack* undoStack() const;

  /// Get/Set if adding sources is ignored.
  void setAddSourceIgnored(bool ignored);
  bool isAddSourceIgnored() const {return this->IgnoreAdd;}

  /// Returns if the source is pending a display creation
  /// by this manager.
  bool isPendingDisplay(pqPipelineSource*) const;

public slots:
  /// add a source for which a display will eventually be made
  void addPendingDisplayForSource(pqPipelineSource* s);

  /// remove a source for which a display will eventually be made
  void removePendingDisplayForSource(pqPipelineSource* s);
  
  /// get the number of deferred displays
  int getNumberOfPendingDisplays();

  /// set the active view - the pending representations will be
  /// created on this view
  void setActiveView(pqView*);

  /// create deferred displays. 
  void createPendingDisplays();

signals:
  // signal emitted for whether the state of pending displays changes
  void pendingDisplays(bool);

protected:
  friend class pqPendingDisplayUndoElement;
  void internalAddPendingDisplayForSource(pqPipelineSource* s);

private:
  class MyInternal;
  MyInternal* Internal;
  bool IgnoreAdd;

  /// create deferred displays. 
  void createPendingDisplays(pqView* view);

  pqPendingDisplayManager(const pqPendingDisplayManager&); // Not implemented.
  void operator=(const pqPendingDisplayManager&); // Not implemented.
  
};

#endif

