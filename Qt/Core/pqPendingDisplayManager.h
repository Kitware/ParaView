/*=========================================================================

   Program: ParaView
   Module:    pqPendingDisplayManager.h

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
#ifndef __pqPendingDisplayManager_h
#define __pqPendingDisplayManager_h

#include "pqCoreExport.h"
#include <QObject>

class pqPipelineSource;
class pqRenderModule;
class pqPendingDisplayUndoElement;

class PQCORE_EXPORT pqPendingDisplayManager  : public QObject
{
  Q_OBJECT
public:

  pqPendingDisplayManager(QObject* p = 0);
  ~pqPendingDisplayManager();

public slots:

  void addPendingDisplayForSource(pqPipelineSource* s);
  void removePendingDisplayForSource(pqPipelineSource* s);
  
  void createPendingDisplays(pqRenderModule* rm);

  int getNumberOfPendingDisplays();

signals:
  // signal emitted for whether the state of pending displays changes
  void pendingDisplays(bool);

protected:
  friend class pqPendingDisplayUndoElement;
  void internalAddPendingDisplayForSource(pqPipelineSource* s);

private:
  class MyInternal;
  MyInternal* Internal;

  pqPendingDisplayManager(const pqPendingDisplayManager&); // Not implemented.
  void operator=(const pqPendingDisplayManager&); // Not implemented.
  
};

#endif

