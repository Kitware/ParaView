/*=========================================================================

   Program: ParaView
   Module:    pqAnnotationLayersModel.cxx

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
#include "pqAnnotationLayersModel.h"

#include "vtkAbstractArray.h"
#include "vtkAnnotation.h"
#include "vtkAnnotationLayers.h"
#include "vtkAnnotationLink.h"
#include "vtkInformation.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"

#include <QPixmap>

//-----------------------------------------------------------------------------
pqAnnotationLayersModel::pqAnnotationLayersModel(QObject* parentObject)
: Superclass(parentObject)
{
  this->Annotations = NULL;

  // Set up the column headers.
  this->insertHeaderSections(Qt::Horizontal, 0, 1);
  this->setCheckable(0, Qt::Horizontal, true);
  this->setCheckState(0, Qt::Horizontal, Qt::Unchecked);

  // Change the index check state when the header checkbox is clicked.
  this->connect(this, SIGNAL(headerDataChanged(Qt::Orientation, int, int)),
      this, SLOT(setIndexCheckState(Qt::Orientation, int, int)));
}

//-----------------------------------------------------------------------------
pqAnnotationLayersModel::~pqAnnotationLayersModel()
{
  this->setAnnotationLink(0);
}

//----------------------------------------------------------------------------
void pqAnnotationLayersModel::setAnnotationLink(vtkAnnotationLink* t) 
{
  if (this->Annotations != NULL)
    {
    this->Annotations->Delete();
    }
  this->Annotations = t;
  if (this->Annotations != NULL)
    {
    this->Annotations->Register(0);

    // Whenever the representation updates, we want to update the list of arrays
    // shown in the series browser.
    //QObject::connect(dataModel, SIGNAL(modelReset()), this, SLOT(reload()));

    emit this->reset();
    }
}

//-----------------------------------------------------------------------------
vtkAnnotationLink* pqAnnotationLayersModel::getAnnotationLink() const
{
  return this->Annotations;
}

//-----------------------------------------------------------------------------
int pqAnnotationLayersModel::rowCount(const QModelIndex &parentIndex) const
{
  if(!this->Annotations || !this->Annotations->GetOutput())
    {
    return 0;
    }

  if (!parentIndex.isValid() && this->Annotations)
    {
    return vtkAnnotationLayers::SafeDownCast(this->Annotations->GetOutput())->GetNumberOfAnnotations();
    }
  return 0;
}

//-----------------------------------------------------------------------------
int pqAnnotationLayersModel::columnCount(const QModelIndex &) const
{
  return 3;
}

//-----------------------------------------------------------------------------
bool pqAnnotationLayersModel::hasChildren(const QModelIndex &parentIndex) const
{
  return (this->rowCount(parentIndex) > 0);
}

//-----------------------------------------------------------------------------
QModelIndex pqAnnotationLayersModel::index(int row, int column,
  const QModelIndex &parentIndex) const
{
  if(!parentIndex.isValid() && column >= 0 && column < 3 &&
    row >= 0 && row < this->rowCount(parentIndex))
    {
    return this->createIndex(row, column);
    }

  return QModelIndex();
}

//-----------------------------------------------------------------------------
QModelIndex pqAnnotationLayersModel::parent(const QModelIndex &) const
{
  return QModelIndex();
}

//-----------------------------------------------------------------------------
QVariant pqAnnotationLayersModel::data(const QModelIndex &idx, int role) const
{
  if (idx.isValid() && idx.model() == this)
    {
    if(role == Qt::DisplayRole || role == Qt::EditRole ||
        role == Qt::ToolTipRole)
      {
      if (idx.column() == 0)
        {
        QString arrayName = this->getAnnotationName(idx.row());
        return QVariant(arrayName);
        }
      else if (idx.column() == 2)
        {
        int numItems = this->getAnnotationSize(idx.row());
        return QVariant(numItems);
        }
      }
    else if (role == Qt::CheckStateRole)
      {
      if (idx.column() == 0)
        {
        return QVariant(this->getAnnotationEnabled(idx.row())?
          Qt::Checked : Qt::Unchecked);
        }
      }
    else if (role == Qt::DecorationRole)
      {
      if (idx.column() == 1)
        {
        QPixmap pixmap(16, 16);
        pixmap.fill(this->getAnnotationColor(idx.row()));
        return QVariant(pixmap);
        }
      }
    }

  return QVariant();
}

//-----------------------------------------------------------------------------
Qt::ItemFlags pqAnnotationLayersModel::flags(const QModelIndex &idx) const
{
  Qt::ItemFlags result = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  if(idx.isValid() && idx.model() == this)
    {
    if(idx.column() == 0)
      {
      result |= Qt::ItemIsUserCheckable;
      }
    else if(idx.column() == 1)
      {
      result |= Qt::ItemIsEditable;
      }
    }

  return result;
}

//-----------------------------------------------------------------------------
bool pqAnnotationLayersModel::setData(const QModelIndex &idx,
    const QVariant &value, int role)
{
  bool result = false;
  if (idx.isValid() && idx.model() == this)
    {
    if (idx.column() == 1 && (role == Qt::DisplayRole || role == Qt::EditRole))
      {
      // FIXME
      //QString name = value.toString();
      //if (!name.isEmpty())
      //  {
      //  result = true;
      //  if (name != item->LegendName)
      //    {
      //    item->LegendName = name;
      //    // TODO Set series label name
      //    //this->Display->setSeriesLabel(idx.row(), item->LegendName);
      //    this->PQDisplay->renderViewEventually();
      //    emit this->dataChanged(idx, idx);
      //    }
      //  }
      }
    else if(idx.column() == 0 && role == Qt::CheckStateRole)
      {
      result = true;
      int checkstate = value.toInt();
      this->setAnnotationEnabled(idx.row(), checkstate == Qt::Checked);
      }
    }

  return result;
}

//-----------------------------------------------------------------------------
QVariant pqAnnotationLayersModel::headerData(int section,
    Qt::Orientation orient, int role) const
{
  if(orient == Qt::Horizontal && role == Qt::DisplayRole)
    {
    if(section == 0)
      {
      return QVariant(QString("Name"));
      }
    else if(section == 1)
      {
      return QVariant(QString("Color"));
      }
    else if(section == 2)
      {
      return QVariant(QString("# Items"));
      }
    }
  else
    {
    return this->Superclass::headerData(section, orient, role);
    }

  return QVariant();
}

//-----------------------------------------------------------------------------
void pqAnnotationLayersModel::reload()
{
  this->reset();
  this->updateCheckState(0, Qt::Horizontal);
}

//-----------------------------------------------------------------------------
const char* pqAnnotationLayersModel::getAnnotationName(int row) const
{
  if(!this->Annotations || !this->Annotations->GetOutput())
    {
    return 0;
    }

  vtkAnnotation* a = vtkAnnotationLayers::SafeDownCast(this->Annotations->GetOutput())->GetAnnotation(row);
  return a->GetInformation()->Get(vtkAnnotation::LABEL());
}

//-----------------------------------------------------------------------------
int pqAnnotationLayersModel::getAnnotationSize(int row) const
{
  if(!this->Annotations || !this->Annotations->GetOutput())
    {
    return 0;
    }

  vtkAnnotation* a = vtkAnnotationLayers::SafeDownCast(this->Annotations->GetOutput())->GetAnnotation(row);
  int count = 0;
  for(int i=0; i<a->GetSelection()->GetNumberOfNodes(); ++i)
    {
    count += a->GetSelection()->GetNode(i)->GetSelectionList()->GetNumberOfTuples();
    }
  return count;
}

//-----------------------------------------------------------------------------
void pqAnnotationLayersModel::setAnnotationEnabled(int row, bool enabled)
{
  if(!this->Annotations || !this->Annotations->GetOutput())
    {
    return;
    }

  if (row >= 0 && row < this->rowCount(QModelIndex()))
    {
    vtkAnnotation* a = vtkAnnotationLayers::SafeDownCast(this->Annotations->GetOutput())->GetAnnotation(row);
    bool e = a->GetInformation()->Get(vtkAnnotation::ENABLE());
    if(e != enabled)
      {
      a->GetInformation()->Set(vtkAnnotation::ENABLE(), enabled);
      //HACK
      vtkSmartPointer<vtkSelection> s = vtkSmartPointer<vtkSelection>::New();
      s->DeepCopy(a->GetSelection());
      a->SetSelection(0);
      a->SetSelection(s);

      QModelIndex idx = this->createIndex(row, 0);
      emit this->dataChanged(idx, idx);
      this->updateCheckState(0, Qt::Horizontal);
      }
    }
}

//-----------------------------------------------------------------------------
bool pqAnnotationLayersModel::getAnnotationEnabled(int row) const
{
  if(!this->Annotations || !this->Annotations->GetOutput())
    {
    return false;
    }

  vtkAnnotation* a = vtkAnnotationLayers::SafeDownCast(this->Annotations->GetOutput())->GetAnnotation(row);
  return a->GetInformation()->Get(vtkAnnotation::ENABLE());
}

//-----------------------------------------------------------------------------
void pqAnnotationLayersModel::setAnnotationColor(int row, const QColor &color)
{
  if(!this->Annotations || !this->Annotations->GetOutput())
    {
    return;
    }

  if (row >= 0 && row < this->rowCount(QModelIndex()))
    {
    double double_color[3];
    color.getRgbF(double_color, double_color+1, double_color+2);
    vtkAnnotation* a = vtkAnnotationLayers::SafeDownCast(this->Annotations->GetOutput())->GetAnnotation(row);
    a->GetInformation()->Set(vtkAnnotation::COLOR(),double_color,3);

    QModelIndex idx = this->createIndex(row, 1);
    emit this->dataChanged(idx, idx);
    }
}

//-----------------------------------------------------------------------------
QColor pqAnnotationLayersModel::getAnnotationColor(int row) const
{
  if(!this->Annotations || !this->Annotations->GetOutput())
    {
    return QColor();
    }

  vtkAnnotation* a = vtkAnnotationLayers::SafeDownCast(this->Annotations->GetOutput())->GetAnnotation(row);
  if(!a->GetInformation()->Has(vtkAnnotation::COLOR()))
    {
    return QColor(211,211,211);
    }
  double *color = a->GetInformation()->Get(vtkAnnotation::COLOR());
  int annColor[3];
  annColor[0] = static_cast<int>(255*color[0]);
  annColor[1] = static_cast<int>(255*color[1]);
  annColor[2] = static_cast<int>(255*color[2]);
  return QColor(annColor[0], annColor[1], annColor[2]);
}


