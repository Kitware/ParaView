/*=========================================================================

   Program: ParaView
   Module:  pqPointPickingHelper.h

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
#ifndef pqPointPickingHelper_h
#define pqPointPickingHelper_h

#include "pqApplicationComponentsModule.h"
#include <QKeySequence>
#include <QObject>
#include <QPointer>

class pqRenderView;
class pqView;
class QShortcut;

/**
* pqPointPickingHelper is a helper class that is designed for use by
* subclasses of pqInteractivePropertyWidget (or others) that want to support
* using a shortcut key to pick a point on the surface mesh.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqPointPickingHelper : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqPointPickingHelper(const QKeySequence& keySequence, bool pick_on_mesh, QObject* parent = 0);
  ~pqPointPickingHelper() override;

  /**
  * Returns whether the helper will pick a point in the mesh or simply a point
  * on the surface. In other words, if pickOnMesh returns true, then the
  * picked point will always be a point specified in the points that form the
  * mesh.
  */
  bool pickOnMesh() const { return this->PickOnMesh; }

public Q_SLOTS:
  /**
  * Set the view on which the pick is active. We only support pqRenderView and
  * subclasses currently.
  */
  void setView(pqView* view);

  /**
  * Enable/disable the pick point shortcut.
  */
  void setShortcutEnabled(bool);

Q_SIGNALS:
  void pick(double x, double y, double z);

private Q_SLOTS:
  void pickPoint();

private:
  Q_DISABLE_COPY(pqPointPickingHelper)
  QKeySequence KeySequence;
  QPointer<pqRenderView> View;
  bool PickOnMesh;
  bool ShortcutEnabled;
  QPointer<QShortcut> Shortcut;
};

#endif
