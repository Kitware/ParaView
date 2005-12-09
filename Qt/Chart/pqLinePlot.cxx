/*!
 * \file pqLinePlot.cxx
 *
 * \brief
 *   The pqLinePlot class is the drawing interface to a function.
 *
 * \author Mark Richardson
 * \date   August 2, 2005
 */

#include "pqLinePlot.h"


pqLinePlot::pqLinePlot(QObject *parent)
  : QObject(parent), Color(Qt::black)
{
  this->thickness = 1;
  this->modified = true;
}

void pqLinePlot::setModified(bool modified)
{
  this->modified = modified;
  if(this->modified)
    {
    emit this->plotModified(this);
    }
}

void pqLinePlot::setColor(const QColor &color)
{
  this->Color = color;
  emit this->plotModified(this);
}

void pqLinePlot::setWidth(int width)
{
  this->thickness = width;
  emit this->plotModified(this);
}


