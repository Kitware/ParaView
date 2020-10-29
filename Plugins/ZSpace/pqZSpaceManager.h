/*=========================================================================

   Program: ParaView
   Module:    pqZSpaceManager.h

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
/**
 * @class   pqZSpaceManager
 * @brief   Autoload class that enable input independent update of the ZSpace render views.
 *
 * This class observes every views of type vtkZSpaceView. When a render is called on one
 * of these views, an other render is manually triggered to ensure constant update
 * of the zSpace render view.
 * @par Thanks:
 * Kitware SAS
 * This work was supported by EDF.
 */

#ifndef pqZSpaceManager_h
#define pqZSpaceManager_h

#include <QObject>

#include <set>

class pqView;
class pqPipelineSource;

class pqZSpaceManager : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqZSpaceManager(QObject* p = nullptr);
  ~pqZSpaceManager() override = default;

  void onShutdown() {}
  void onStartup() {}

public Q_SLOTS:
  void onViewAdded(pqView*);
  void onViewRemoved(pqView*);

protected Q_SLOTS:
  void onRenderEnded();

protected:
  std::set<pqView*> ZSpaceViews;

private:
  Q_DISABLE_COPY(pqZSpaceManager)
};

#endif
