// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "pqNodeEditorAnnotationItem.h"

#include "pqNodeEditorUtils.h"

#include <QBrush>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsTextItem>
#include <QInputDialog>
#include <QKeyEvent>
#include <QPainter>
#include <QPen>
#include <QSettings>

namespace details
{
/**
 * Map an int index to a corner of a QRectF through setters and getters.
 */
class QRectAccessor
{
public:
  QRectAccessor(QRectF& wrapped)
    : rect(wrapped)
  {
  }

  QPointF point(int i)
  {
    switch (i)
    {
      case 0:
        return rect.topLeft();
      case 1:
        return rect.topRight();
      case 2:
        return rect.bottomLeft();
      case 3:
        return rect.bottomRight();
      default:
        return QPointF{ 0.0, 0.0 };
    }
  }

  void setPoint(int i, const QPointF& p)
  {
    switch (i)
    {
      case 0:
        rect.setTopLeft(p);
        break;
      case 1:
        rect.setTopRight(p);
        break;
      case 2:
        rect.setBottomLeft(p);
        break;
      case 3:
        rect.setBottomRight(p);
        break;
      default:
        return;
    }
  }

  QRectF& rect;
};
}

// ----------------------------------------------------------------------------
pqNodeEditorAnnotationItem::pqNodeEditorAnnotationItem(QRectF size, QGraphicsItem* parent)
  : QGraphicsItem(parent)
  , boundingBox{ size }
  , title(new QGraphicsTextItem(QObject::tr("Annotation"), this))
{
  this->setFlag(GraphicsItemFlag::ItemIsSelectable);
  this->setCursor(Qt::ArrowCursor);

  this->title->setFlag(GraphicsItemFlag::ItemIgnoresTransformations, false);
  this->title->setTextInteractionFlags(Qt::TextEditorInteraction);
  this->title->setCursor(Qt::ArrowCursor);
  title->setDefaultTextColor(pqNodeEditorUtils::CONSTS::COLOR_CONSTRAST);
  QFont font;
  font.setPointSize(24);
  this->title->setFont(font);
  this->updateTitlePos();
}

// ----------------------------------------------------------------------------
void pqNodeEditorAnnotationItem::updateTitlePos()
{
  constexpr float MARGIN = 40;
  this->title->setPos(this->boundingBox.left() + MARGIN, this->boundingBox.top() - MARGIN);
}

// ----------------------------------------------------------------------------
QRectF pqNodeEditorAnnotationItem::boundingRect() const
{
  return this->boundingBox;
}

// ----------------------------------------------------------------------------
void pqNodeEditorAnnotationItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
  QRectF rect = this->boundingBox.adjusted(8, 8, -8, -8);
  QColor background;
  if (this->isSelected())
  {
    background = pqNodeEditorUtils::CONSTS::COLOR_BASE;
    background.setAlphaF(0.85);
    // put this above nodes, without conflicting with an eventual future type
    // as graph elements are separated by step of 10 in pqNodeEditorUtils.h
    this->setZValue(pqNodeEditorUtils::CONSTS::NODE_LAYER + 9);
  }
  else
  {
    background = pqNodeEditorUtils::CONSTS::COLOR_GRID;
    background.setAlphaF(0.2);
    this->setZValue(pqNodeEditorUtils::CONSTS::ANNOTATION_LAYER);
  }

  QPen pen(pqNodeEditorUtils::CONSTS::COLOR_CONSTRAST, pqNodeEditorUtils::CONSTS::EDGE_WIDTH - 1);
  painter->setBrush(background);
  painter->setPen(pen);
  painter->drawRect(rect);

  constexpr int RADIUS = 7;
  painter->setBrush(pqNodeEditorUtils::CONSTS::COLOR_BASE);
  painter->drawEllipse(rect.bottomLeft(), RADIUS, RADIUS);
  painter->drawEllipse(rect.bottomRight(), RADIUS, RADIUS);
  painter->drawEllipse(rect.topLeft(), RADIUS, RADIUS);
  painter->drawEllipse(rect.topRight(), RADIUS, RADIUS);

  if (this->isSelected())
  {
    QFont font;
    font.setPointSize(20);
    painter->setFont(font);
    painter->drawText(rect.adjusted(8, 8, -8, -8), this->text);
  }
}

// ----------------------------------------------------------------------------
void pqNodeEditorAnnotationItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
  this->setCursor(QCursor(Qt::ClosedHandCursor));
  this->dragOffset = event->pos();
  this->selectedPoint = -1;

  QRectF adjusted = this->boundingBox.adjusted(8, 8, -8, -8);
  details::QRectAccessor bbox(adjusted);
  for (int i = 0; i < 4; ++i)
  {
    constexpr int RADIUS = 15;
    if (std::abs((bbox.point(i) - this->dragOffset).manhattanLength()) < RADIUS)
    {
      this->selectedPoint = i;
      break;
    }
  }
}

// ----------------------------------------------------------------------------
void pqNodeEditorAnnotationItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
  this->QGraphicsItem::mouseMoveEvent(event);

  if (this->selectedPoint >= 0)
  {
    this->prepareGeometryChange();
    details::QRectAccessor(this->boundingBox)
      .setPoint(this->selectedPoint, this->mapToScene(event->pos() - this->pos()));
    this->updateTitlePos();
  }
  else
  {
    this->setPos(this->mapToScene(event->pos() - this->dragOffset));
  }
}

// ----------------------------------------------------------------------------
void pqNodeEditorAnnotationItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
  this->QGraphicsItem::mouseReleaseEvent(event);

  this->selectedPoint = -1;
  this->setCursor(QCursor(Qt::ArrowCursor));
}

// ----------------------------------------------------------------------------
void pqNodeEditorAnnotationItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
  event->ignore();

  QInputDialog* inp = new QInputDialog(nullptr, Qt::Popup | Qt::FramelessWindowHint);
  inp->setWindowTitle(QObject::tr("Annotation") + ":");
  inp->setLabelText(QObject::tr("Annotation") + ":");
  inp->setTextValue(this->text);
  inp->setOption(QInputDialog::UsePlainTextEditForTextInput);
  inp->adjustSize();
  inp->move(event->screenPos());
  if (inp->exec() == QDialog::Accepted)
  {
    this->prepareGeometryChange();
    this->text = inp->textValue();
  }
}

// ----------------------------------------------------------------------------
void pqNodeEditorAnnotationItem::importLayout(const QSettings& settings, int id)
{
  const QString prefix = "annotations." + QString::number(id);

  this->prepareGeometryChange();
  if (auto titl = pqNodeEditorUtils::safeGetValue<QString>(settings, prefix + ".title"))
  {
    this->title->setPlainText(titl.Value);
  }
  if (auto desc = pqNodeEditorUtils::safeGetValue<QString>(settings, prefix + ".text"))
  {
    this->text = desc.Value;
  }
  if (auto bbox = pqNodeEditorUtils::safeGetValue<QRectF>(settings, prefix + ".boundingBox"))
  {
    this->boundingBox = bbox.Value;
  }
  if (auto transfo = pqNodeEditorUtils::safeGetValue<QTransform>(settings, prefix + ".transform"))
  {
    this->setTransform(transfo.Value);
  }
  if (auto pos = pqNodeEditorUtils::safeGetValue<QPointF>(settings, prefix + ".pos"))
  {
    this->setPos(pos.Value);
  }
  if (auto selected = pqNodeEditorUtils::safeGetValue<bool>(settings, prefix + ".selected"))
  {
    this->setSelected(selected);
  }
  this->updateTitlePos();
}

// ----------------------------------------------------------------------------
void pqNodeEditorAnnotationItem::exportLayout(QSettings& settings, int id)
{
  const QString prefix = "annotations." + QString::number(id);

  auto exportProperty = [&](const char* prop, QVariant value) {
    settings.setValue(prefix + prop, value);
  };

  exportProperty(".pos", this->pos());

  exportProperty(".transform", this->transform());

  exportProperty(".title", this->title->toPlainText());

  exportProperty(".text", this->text);

  exportProperty(".boundingBox", this->boundingBox);

  exportProperty(".selected", this->isSelected());
}

// ----------------------------------------------------------------------------
std::vector<pqNodeEditorAnnotationItem*> pqNodeEditorAnnotationItem::importAll(
  const QSettings& settings)
{
  const auto count = pqNodeEditorUtils::safeGetValue<int>(settings, "annotations.count");
  if (!count.Valid || count.Value == 0)
  {
    return {};
  }

  std::vector<pqNodeEditorAnnotationItem*> result(count.Value);
  for (int i = 0; i < count.Value; ++i)
  {
    result[i] = new pqNodeEditorAnnotationItem(QRectF());
    result[i]->importLayout(settings, i);
  }
  return result;
}

// ----------------------------------------------------------------------------
void pqNodeEditorAnnotationItem::exportAll(
  QSettings& settings, std::vector<pqNodeEditorAnnotationItem*> annot)
{
  settings.setValue("annotations.count", static_cast<int>(annot.size()));

  int idx = 0;
  for (auto* annotation : annot)
  {
    annotation->exportLayout(settings, idx++);
  }
}
