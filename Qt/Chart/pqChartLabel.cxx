#include "pqChartLabel.h"
#include <QPainter>

pqChartLabel::pqChartLabel(QObject *p) :
  QObject(p),
  Color(Qt::black)
{
}

void pqChartLabel::setBounds(const QRect& bounds)
{
  this->Bounds = bounds;
  emit repaintNeeded();
}

void pqChartLabel::setText(const QString& text)
{
  this->Text = text;
  emit layoutNeeded();
}

void pqChartLabel::setColor(const QColor& color)
{
  this->Color = color;
  emit repaintNeeded();
}

void pqChartLabel::setFont(const QFont& font)
{
  this->Font = font;
  emit layoutNeeded();
}

const QRect pqChartLabel::getSizeRequest()
{
  QFontMetrics metrics(this->Font);
  return metrics.boundingRect(this->Text);
}

void pqChartLabel::draw(QPainter& painter, const QRect &area)
{
  if(this->Text.isEmpty())
    return;
  
  painter.save();
    painter.setRenderHint(QPainter::TextAntialiasing, false);
    painter.setFont(this->Font);
    painter.setPen(this->Color);
    painter.drawText(this->Bounds, Qt::AlignCenter, this->Text);
  painter.restore();
}

