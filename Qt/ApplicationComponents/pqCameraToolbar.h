/*=========================================================================

   Program: ParaView
   Module:    pqCameraToolbar.h

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
#ifndef pqCameraToolbar_h
#define pqCameraToolbar_h

#include "pqApplicationComponentsModule.h"
#include <QToolBar>

class pqPipelineSource;
class pqDataRepresentation;

/**
 * pqCameraToolbar is the toolbar that has icons for resetting camera
 * orientations as well as zoom-to-data and zoom-to-box.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqCameraToolbar : public QToolBar
{
  Q_OBJECT
  typedef QToolBar Superclass;

public:
  pqCameraToolbar(const QString& title, QWidget* parentObject = nullptr)
    : Superclass(title, parentObject)
  {
    this->constructor();
  }
  pqCameraToolbar(QWidget* parentObject = nullptr)
    : Superclass(parentObject)
  {
    this->constructor();
  }

private Q_SLOTS:

  void onSourceChanged(pqPipelineSource*);
  void updateEnabledState();
  void onRepresentationChanged(pqDataRepresentation*);

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqCameraToolbar)
  void constructor();
  QAction* ZoomToDataAction;
  QAction* ZoomClosestToDataAction;

  // Currently bound connection to an active source, used to
  // disable ZoomToData actions if the current source
  // is not visible
  QMetaObject::Connection SourceVisibilityChangedConnection;
  // Currently bound connection to an active representation, used to
  // disable ZoomToData actions if the current representation
  // is not visible
  QMetaObject::Connection RepresentationDataUpdatedConnection;

  /**
   * A source is visible if its representation is visible
   * and at least one of its block is visible in case of a
   * composite dataset.
   */
  bool isRepresentationVisible(pqDataRepresentation* repr);
};

#endif
