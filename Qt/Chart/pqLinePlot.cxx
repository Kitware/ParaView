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

pqLinePlot::pqLinePlot(QObject *p)
  : QObject(p), Pen(Qt::black)
{
  this->Modified = true;
}

void pqLinePlot::setModified(bool modified)
{
  this->Modified = modified;
  if(this->Modified)
    {
    emit this->plotModified(this);
    }
}

void pqLinePlot::setPen(const QPen& pen)
{
  this->Pen = pen;
  emit this->plotModified(this);
}

