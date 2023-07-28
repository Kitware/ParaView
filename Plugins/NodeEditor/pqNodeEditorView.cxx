// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (C) 2021 Jonas Lukasczyk
// SPDX-License-Identifier: BSD-3-Clause

#include "pqNodeEditorView.h"
#include "pqNodeEditorUtils.h"

#include <pqDeleteReaction.h>
#include <pqKeySequences.h>

// qt includes
#include <QAction>
#include <QKeyEvent>
#include <QWheelEvent>

// ----------------------------------------------------------------------------
pqNodeEditorView::pqNodeEditorView(QGraphicsScene* scene, QWidget* parent)
  : QGraphicsView(scene, parent)
  , deleteAction(new QAction(this))
{
  this->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
  this->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
  this->setDragMode(QGraphicsView::ScrollHandDrag);
  constexpr QRectF MAX_SCENE_SIZE{ -1e4, -1e4, 3e4, 3e4 };
  this->setSceneRect(MAX_SCENE_SIZE);

  // Handle shortcuts through pqKeySequences in order to prevent any sort of conflicts
  new pqDeleteReaction(this->deleteAction);
  pqKeySequences::instance().addModalShortcut(
    QKeySequence{ Qt::Key_Delete }, this->deleteAction, parent);

  auto* annotateAction = new QAction(this);
  QObject::connect(
    annotateAction, &QAction::triggered, [this](bool) { Q_EMIT this->annotate(false); });
  pqKeySequences::instance().addModalShortcut(QKeySequence{ "N" }, annotateAction, parent);

  auto* deleteAnnotation = new QAction(this);
  QObject::connect(
    deleteAnnotation, &QAction::triggered, [this](bool) { Q_EMIT this->annotate(true); });
  pqKeySequences::instance().addModalShortcut(QKeySequence{ "Ctrl+N" }, deleteAnnotation, parent);
}

// ----------------------------------------------------------------------------
void pqNodeEditorView::wheelEvent(QWheelEvent* event)
{
  constexpr double ZOOM_INCREMENT_RATIO = 0.1;

  const ViewportAnchor anchor = this->transformationAnchor();
  this->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
  const int angle = event->angleDelta().y();
  const double factor = 1.0 + ((angle > 0) ? ZOOM_INCREMENT_RATIO : -ZOOM_INCREMENT_RATIO);

  this->scale(factor, factor);
  this->setTransformationAnchor(anchor);
}

// ----------------------------------------------------------------------------
void pqNodeEditorView::triggerDeleteAction() const
{
  this->deleteAction->trigger();
}
