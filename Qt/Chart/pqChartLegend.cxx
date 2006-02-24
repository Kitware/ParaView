/*=========================================================================

   Program:   ParaQ
   Module:    pqChartLegend.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

#include "pqChartLabel.h"
#include "pqChartLegend.h"
#include "pqMarkerPen.h"

#include <vtkstd/vector>

#include <QPainter>

static const int padding = 5;
static const int line_width = 15;

class pqChartLegend::pqImplementation
{
public:
  ~pqImplementation()
  {
    this->clear();
  }

  void clear()
  {
    for(unsigned int i = 0; i != this->Pens.size(); ++i)
      delete this->Pens[i];
  
    for(unsigned int i = 0; i != this->Labels.size(); ++i)
      delete this->Labels[i];
      
    this->Pens.clear();
    this->Labels.clear();
  }

  QRect Bounds;
  
  vtkstd::vector<pqMarkerPen*> Pens;
  vtkstd::vector<pqChartLabel*> Labels;
};

pqChartLegend::pqChartLegend(QObject* p) :
  QObject(p),
  Implementation(new pqImplementation())
{
}

pqChartLegend::~pqChartLegend()
{
  delete Implementation;
}

void pqChartLegend::clear()
{
  this->Implementation->clear();
  emit layoutNeeded();
}

void pqChartLegend::addEntry(pqMarkerPen* pen, pqChartLabel* label)
{
  if(!label)
    return;
      
  this->Implementation->Pens.push_back(pen);
  this->Implementation->Labels.push_back(label);
  emit layoutNeeded();
}

const QRect pqChartLegend::getSizeRequest()
{
  QRect result(0, 0, 0, 0);

  for(unsigned int i = 0; i != this->Implementation->Labels.size(); ++i)
    {
    QRect request = this->Implementation->Labels[i]->getSizeRequest();
    result.setBottom(result.bottom() + request.height());
    result.setWidth(vtkstd::max(result.width(), request.width()));
    }
  
  result.setWidth(result.width() + line_width + (2 * padding));
  result.setHeight(result.height() + (2 * padding));
  
  return result;
}

void pqChartLegend::setBounds(const QRect& bounds)
{
  this->Implementation->Bounds = bounds;
  
  if(this->Implementation->Labels.size())
    {
    const int entry_height = (bounds.height() - padding - padding) / this->Implementation->Labels.size();
    for(unsigned int i = 0; i != this->Implementation->Labels.size(); ++i)
      {
      this->Implementation->Labels[i]->setBounds(
        QRect(
          bounds.left() + line_width + padding,
          bounds.top() + padding + (i * entry_height),
          bounds.width() - line_width - padding - padding,
          entry_height));
      }
    }
  
  emit repaintNeeded();
}

void pqChartLegend::draw(QPainter& painter, const QRect& area)
{
  if(this->Implementation->Labels.empty())
    return;
    
  painter.save();
  painter.drawRect(this->Implementation->Bounds);
  
  for(unsigned int i = 0; i != this->Implementation->Labels.size(); ++i)
    {
    painter.setRenderHint(QPainter::Antialiasing, true);
    
    const QRect label_bounds = this->Implementation->Labels[i]->getBounds();

    this->Implementation->Pens[i]->drawLine(
      painter,
      this->Implementation->Bounds.left() + padding,
      label_bounds.bottom(),
      this->Implementation->Bounds.left() + line_width,
      label_bounds.top());
    }
  
  for(unsigned int i = 0; i != this->Implementation->Labels.size(); ++i)
    {
    this->Implementation->Labels[i]->draw(painter, area);
    }
  
  painter.restore();
}
