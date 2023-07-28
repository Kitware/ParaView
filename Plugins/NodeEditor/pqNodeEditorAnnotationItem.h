// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqNodeEditorAnnotationItem_h
#define pqNodeEditorAnnotationItem_h

#include <QGraphicsItem>
#include <QRectF>

#include <vector>

class pqPipelineSource;
class pqOutputPort;
class QGraphicsSceneMouseEvent;
class QSettings;

/**
 * Class to draw some block of informations in a QGraphicsScene. Interactions are:
 * - double click on them to edit the description
 * - double click on the title to edit it
 * - Ctrl+click on the annotation to select / deselect it. Selecting it makes the description
 * visible.
 *
 * Creation is handled by pqNodeEditorWidget.
 */
class pqNodeEditorAnnotationItem : public QGraphicsItem
{
public:
  pqNodeEditorAnnotationItem(QRectF size, QGraphicsItem* parent = nullptr);

  ///@{
  /**
   * Mandatory override of QGraphicsItem.
   */
  QRectF boundingRect() const override;
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
  ///@}

  ///@{
  /**
   * Import / Export layout from a Qt settings instance.
   */
  void importLayout(const QSettings& settings, int id);
  void exportLayout(QSettings& settings, int id);

  static std::vector<pqNodeEditorAnnotationItem*> importAll(const QSettings& settings);
  static void exportAll(QSettings& settings, std::vector<pqNodeEditorAnnotationItem*> annot);
  ///@}

protected:
  void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

  /**
   * Update the title pos. Must be called when changes have been made to the boundingBox.
   */
  void updateTitlePos();

private:
  QRectF boundingBox;
  QGraphicsTextItem* title;
  QString text = "Description";

  // Used for interactions
  QPointF dragOffset{ 0.f, 0.f };
  int selectedPoint = -1;
};

#endif // pqNodeEditorAnnotationItem_h
