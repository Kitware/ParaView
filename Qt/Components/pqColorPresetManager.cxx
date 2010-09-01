/*=========================================================================

   Program: ParaView
   Module:    pqColorPresetManager.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/

/// \file pqColorPresetManager.cxx
/// \date 3/12/2007

#include "pqColorPresetManager.h"
#include "ui_pqColorPresetDialog.h"

#include "pqApplicationCore.h"
#include "pqChartValue.h"
#include "pqColorMapModel.h"
#include "pqColorPresetModel.h"
#include "pqFileDialog.h"
#include "pqSettings.h"

#include <QHeaderView>
#include <QItemDelegate>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QList>
#include <QMenu>
#include <QPainter>
#include <QPersistentModelIndex>
#include <QPoint>
#include <QRect>
#include <QString>
#include <QStringList>

#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include <vtksys/ios/sstream>


class pqColorPresetManagerForm : public Ui::pqColorPresetDialog {};


class pqColorPresetDelegate : public QItemDelegate
{
public:
  pqColorPresetDelegate(QObject *parent=0);
  virtual ~pqColorPresetDelegate() {}

  virtual QSize sizeHint(const QStyleOptionViewItem &option,
      const QModelIndex &index) const;

protected:
  virtual void drawDecoration(QPainter *painter,
      const QStyleOptionViewItem &option, const QRect &rect,
      const QPixmap &pixmap) const;
};


//----------------------------------------------------------------------------
pqColorPresetDelegate::pqColorPresetDelegate(QObject *parentObject)
  : QItemDelegate(parentObject)
{
}

QSize pqColorPresetDelegate::sizeHint(const QStyleOptionViewItem &option,
    const QModelIndex &index) const
{
  // Get the size hint from the base class and pad the height.
  QSize size = QItemDelegate::sizeHint(option, index);
  size.setHeight(size.height() + 4);
  return size;
}

void pqColorPresetDelegate::drawDecoration(QPainter *painter,
    const QStyleOptionViewItem &option, const QRect &area,
    const QPixmap &pixmap) const
{
  if(pixmap.isNull() || !area.isValid())
    {
    return;
    }

  QPoint p = QStyle::alignedRect(option.direction, option.decorationAlignment,
      pixmap.size(), area).topLeft();
  painter->drawPixmap(p, pixmap);
}


//----------------------------------------------------------------------------
pqColorPresetManager::pqColorPresetManager(QWidget *widgetParent)
  : QDialog(widgetParent)
{
  this->Form = new pqColorPresetManagerForm();
  this->Model = new pqColorPresetModel(this);
  this->Model->setObjectName("ColorPresetModel");
  this->InitSections = true;

  this->Form->setupUi(this);
  this->Form->Gradients->setIconSize(QSize(100, 20));
  this->Form->Gradients->setItemDelegate(new pqColorPresetDelegate(
      this->Form->Gradients));
  this->Form->Gradients->setModel(this->Model);
  this->Form->Gradients->setContextMenuPolicy(Qt::CustomContextMenu);

  this->connect(this->Model,
      SIGNAL(rowsInserted(const QModelIndex &, int, int)),
      this, SLOT(selectNewItem(const QModelIndex &, int, int)));
  this->connect(this->Form->Gradients->selectionModel(),
      SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
      this, SLOT(updateButtons()));

  this->connect(this->Form->ImportButton, SIGNAL(clicked()),
      this, SLOT(importColorMap()));
  this->connect(this->Form->ExportButton, SIGNAL(clicked()),
      this, SLOT(exportColorMap()));
  this->connect(this->Form->NormalizeButton, SIGNAL(clicked()),
      this, SLOT(normalizeSelected()));
  this->connect(this->Form->RemoveButton, SIGNAL(clicked()),
      this, SLOT(removeSelected()));

  this->connect(this->Form->Gradients,
      SIGNAL(customContextMenuRequested(const QPoint &)),
      this, SLOT(showContextMenu(const QPoint &)));
  this->connect(this->Form->Gradients, SIGNAL(activated(const QModelIndex &)),
      this, SLOT(handleItemActivated()));

  this->connect(this->Form->OkButton, SIGNAL(clicked()), this, SLOT(accept()));
  this->connect(this->Form->CancelButton, SIGNAL(clicked()),
      this, SLOT(reject()));

  // Initialize the button enabled states.
  this->updateButtons();
}

pqColorPresetManager::~pqColorPresetManager()
{
  delete this->Form;
}

QItemSelectionModel *pqColorPresetManager::getSelectionModel() const
{
  return this->Form->Gradients->selectionModel();
}

bool pqColorPresetManager::isUsingCloseButton() const
{
  return this->Form->CancelButton->isHidden();
}

void pqColorPresetManager::setUsingCloseButton(bool showClose)
{
  if(showClose != this->Form->CancelButton->isHidden())
    {
    if(!showClose)
      {
      this->Form->OkButton->setText("&OK");
      }

    this->Form->CancelButton->setVisible(!showClose);
    if(showClose)
      {
      this->Form->OkButton->setText("&Close");
      }

    this->Form->OkButton->setEnabled(this->isUsingCloseButton() ||
        this->Form->Gradients->selectionModel()->selectedIndexes().size() > 0);
    }
}

void pqColorPresetManager::saveSettings()
{
  if(!this->Model->isModified())
    {
    return;
    }

  pqSettings *settings = pqApplicationCore::instance()->settings();
  settings->beginGroup("ColorMapPresets");
  settings->remove("");
  for(int i = 0; i < this->Model->rowCount(); i++)
    {
    // Get the color map xml for the index.
    QModelIndex index = this->Model->index(i, 0);
    if(!(this->Model->flags(index) & Qt::ItemIsEditable))
      {
      continue; // Skip the builtin color maps.
      }

    vtkPVXMLElement *root = vtkPVXMLElement::New();
    root->SetName("ColorMap");
    this->exportColorMap(index, root);

    // Save the xml in the settings.
    vtksys_ios::ostringstream xml_stream;
    root->PrintXML(xml_stream, vtkIndent());
    root->Delete();
    settings->setValue(QString::number(i), QVariant(xml_stream.str().c_str()));
    }

  settings->endGroup();
}

void pqColorPresetManager::restoreSettings()
{
  pqSettings *settings = pqApplicationCore::instance()->settings();
  settings->beginGroup("ColorMapPresets");
  QStringList keys = settings->childKeys();
  for(QStringList::Iterator key = keys.begin(); key != keys.end(); ++key)
    {
    // Get the color map xml from the settings.
    QString text = settings->value(*key).toString();
    if(text.isEmpty())
      {
      continue;
      }

    // Parse the xml.
    vtkPVXMLParser *xmlParser = vtkPVXMLParser::New();
    xmlParser->InitializeParser();
    xmlParser->ParseChunk(text.toAscii().data(), static_cast<unsigned int>(
        text.size()));
    xmlParser->CleanupParser();

    // Add the color map and clean up the parser.
    this->importColorMap(xmlParser->GetRootElement());
    xmlParser->Delete();
    }

  settings->endGroup();
  this->Model->setModified(false);
}

bool pqColorPresetManager::eventFilter(QObject *object, QEvent *e)
{
  if(e->type() == QEvent::KeyPress && object == this->Form->Gradients)
    {
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
    if(keyEvent->key() == Qt::Key_Delete ||
        keyEvent->key() == Qt::Key_Backspace)
      {
      if(this->Form->RemoveButton->isEnabled())
        {
        this->removeSelected();
        }
      }
    }

  return QDialog::eventFilter(object, e);
}

void pqColorPresetManager::importColorMap(const QStringList &files)
{
  // Read in the color maps from the specified files.
  QString colorMap("ColorMap");
  QStringList::ConstIterator fileName = files.begin();
  for( ; fileName != files.end(); ++fileName)
    {
    vtkPVXMLParser *xmlParser = vtkPVXMLParser::New();
    xmlParser->SetFileName(fileName->toAscii().data());
    xmlParser->Parse();
    vtkPVXMLElement *root = xmlParser->GetRootElement();
    if(colorMap == root->GetName())
      {
      this->importColorMap(root);
      }
    else
      {
      vtkPVXMLElement *element = 0;
      for(unsigned int i = 0; i < root->GetNumberOfNestedElements(); i++)
        {
        element = root->GetNestedElement(i);
        if(colorMap == element->GetName())
          {
          this->importColorMap(element);
          }
        }
      }

    xmlParser->Delete();
    }
}

void pqColorPresetManager::exportColorMap(const QStringList &files)
{
  if(!this->Form->ExportButton->isEnabled())
    {
    return;
    }

  // Construct the color map xml for the selected indexes.
  QModelIndexList indexes =
      this->Form->Gradients->selectionModel()->selectedIndexes();
  vtkPVXMLElement *root = vtkPVXMLElement::New();
  vtkPVXMLElement *element = root;
  if(indexes.size() > 1)
    {
    root->SetName("ColorMaps");
    }
  else
    {
    root->SetName("ColorMap");
    }

  QModelIndexList::Iterator index = indexes.begin();
  for( ; index != indexes.end(); ++index)
    {
    if(indexes.size() > 1)
      {
      element = vtkPVXMLElement::New();
      element->SetName("ColorMap");
      root->AddNestedElement(element);
      element->Delete();
      }

    this->exportColorMap(*index, element);
    }

  // Save the xml to each of the specified files.
  QStringList::ConstIterator fileName = files.begin();
  for( ; fileName != files.end(); ++fileName)
    {
    ofstream os(fileName->toAscii().data(), ios::out);
    root->PrintXML(os, vtkIndent());
    }

  // Clean up the xml structures.
  root->Delete();
}

void pqColorPresetManager::showEvent(QShowEvent *e)
{
  QDialog::showEvent(e);
  if(this->InitSections)
    {
    this->InitSections = false;
    QHeaderView *header = this->Form->Gradients->header();
    header->resizeSection(0, this->Form->Gradients->viewport()->width() -
        header->sectionSizeHint(1));
    header->resizeSection(1, header->sectionSizeHint(1));
    }
}

void pqColorPresetManager::importColorMap()
{
  // Let the user select a file.
  pqFileDialog *fileDialog = new pqFileDialog(0, this,
      tr("Import Color Map"), QString(),
      "Color Map Files (*.xml);;All Files (*)");
  fileDialog->setAttribute(Qt::WA_DeleteOnClose);
  fileDialog->setObjectName("FileImportDialog");
  fileDialog->setFileMode(pqFileDialog::ExistingFiles);

  // Listen for the user's selection.
  this->connect(fileDialog, SIGNAL(filesSelected(const QStringList &)),
      this, SLOT(importColorMap(const QStringList &)));

  fileDialog->exec();
}

void pqColorPresetManager::exportColorMap()
{
  // Let the user select a file to save as.
  pqFileDialog *fileDialog = new pqFileDialog(0, this,
      tr("Export Color Map"), QString(),
      "Color Map Files (*.xml);;All Files (*)");
  fileDialog->setAttribute(Qt::WA_DeleteOnClose);
  fileDialog->setObjectName("FileExportDialog");
  fileDialog->setFileMode(pqFileDialog::AnyFile);

  // Listen for the user's selection.
  this->connect(fileDialog, SIGNAL(filesSelected(const QStringList &)),
      this, SLOT(exportColorMap(const QStringList &)));

  fileDialog->exec();
}

void pqColorPresetManager::normalizeSelected()
{
  QModelIndexList indexes =
      this->Form->Gradients->selectionModel()->selectedIndexes();
  QModelIndexList::Iterator iter = indexes.begin();
  for( ; iter != indexes.end(); ++iter)
    {
    this->Model->normalizeColorMap(iter->row());
    }

  // Normalizing should only happen once.
  this->Form->NormalizeButton->setEnabled(false);
}

void pqColorPresetManager::removeSelected()
{
  // Use a list of persistent model indexes so the row will be updated
  // as items are deleted.
  QModelIndexList indexes =
      this->Form->Gradients->selectionModel()->selectedIndexes();
  QList<QPersistentModelIndex> toDelete;
  QModelIndexList::Iterator iter = indexes.begin();
  for( ; iter != indexes.end(); ++iter)
    {
    toDelete.append(*iter);
    }

  QList<QPersistentModelIndex>::Iterator jter = toDelete.begin();
  for( ; jter != toDelete.end(); ++jter)
    {
    this->Model->removeColorMap(jter->row());
    }
}

void pqColorPresetManager::updateButtons()
{
  QModelIndexList indexes =
      this->Form->Gradients->selectionModel()->selectedIndexes();
  this->Form->ExportButton->setEnabled(indexes.size() > 0);
  this->Form->OkButton->setEnabled(
      this->isUsingCloseButton() || indexes.size() > 0);

  // Check the list for builtin color maps.
  bool canDelete = indexes.size() > 0;
  bool canNormalize = false;
  QModelIndexList::Iterator iter = indexes.begin();
  for( ; iter != indexes.end(); ++iter)
    {
    if(!(this->Model->flags(*iter) & Qt::ItemIsEditable))
      {
      canDelete = false;
      }

    const pqColorMapModel *colorMap = this->Model->getColorMap(iter->row());
    if(!colorMap->isRangeNormalized())
      {
      canNormalize = true;
      }
    }

  this->Form->NormalizeButton->setEnabled(canNormalize);
  this->Form->RemoveButton->setEnabled(canDelete);
}

void pqColorPresetManager::showContextMenu(const QPoint &point)
{
  QMenu menu(this);
  QAction *action = menu.addAction(this->Form->ImportButton->text(),
      this, SLOT(importColorMap()));
  action->setEnabled(this->Form->ImportButton->isEnabled());
  action = menu.addAction(this->Form->ExportButton->text(),
      this, SLOT(exportColorMap()));
  action->setEnabled(this->Form->ExportButton->isEnabled());
  menu.addSeparator();
  action = menu.addAction(this->Form->RemoveButton->text(),
      this, SLOT(removeSelected()));
  action->setEnabled(this->Form->RemoveButton->isEnabled());
  menu.exec(this->Form->Gradients->viewport()->mapToGlobal(point));
}

void pqColorPresetManager::handleItemActivated()
{
  if(!this->isUsingCloseButton())
    {
    this->accept();
    }
}

void pqColorPresetManager::selectNewItem(const QModelIndex &, int first,
    int last)
{
  QItemSelectionModel *selection = this->Form->Gradients->selectionModel();
  if(this->Form->Gradients->selectionMode() ==
      QAbstractItemView::SingleSelection)
    {
    selection->setCurrentIndex(this->Model->index(last, 0),
        QItemSelectionModel::ClearAndSelect);
    }
  else
    {
    QModelIndex lastIndex = this->Model->index(last, 0);
    QItemSelection range(this->Model->index(first, 0), lastIndex);
    selection->select(range, QItemSelectionModel::ClearAndSelect);
    selection->setCurrentIndex(lastIndex, QItemSelectionModel::NoUpdate);
    }
}

void pqColorPresetManager::importColorMap(vtkPVXMLElement *element)
{
  pqColorMapModel colorMap;
  QString name = element->GetAttribute("name");
  QString space = element->GetAttribute("space");
  if(space == "RGB")
    {
    colorMap.setColorSpace(pqColorMapModel::RgbSpace);
    }
  else if(space == "Lab")
    {
    colorMap.setColorSpace(pqColorMapModel::LabSpace);
    }
  else if(space == "Wrapped")
    {
    colorMap.setColorSpace(pqColorMapModel::WrappedHsvSpace);
    }
  else if(space == "Diverging")
    {
    colorMap.setColorSpace(pqColorMapModel::DivergingSpace);
    }
  else
    {
    colorMap.setColorSpace(pqColorMapModel::HsvSpace);
    }

  // Loop through the point elements.
  for(unsigned int i = 0; i < element->GetNumberOfNestedElements(); i++)
    {
    vtkPVXMLElement *point = element->GetNestedElement(i);
    if(QString("Point") == point->GetName())
      {
      double px = 0.0;
      double a = 1.0;
      double r = 0.0, g = 0.0, b = 0.0;
      double h = 0.0, s = 0.0, v = 0.0;

      if(point->GetScalarAttribute("x", &px) == 0)
        {
        continue;
        }

      point->GetScalarAttribute("o", &a);

      QColor color;
      if(point->GetAttribute("r"))
        {
        if(point->GetScalarAttribute("r", &r) == 0)
          {
          continue;
          }

        if(point->GetScalarAttribute("g", &g) == 0)
          {
          continue;
          }

        if(point->GetScalarAttribute("b", &b) == 0)
          {
          continue;
          }

        color = QColor::fromRgbF(r, g, b);
        }
      else
        {
        if(point->GetScalarAttribute("h", &h) == 0)
          {
          continue;
          }

        if(point->GetScalarAttribute("s", &s) == 0)
          {
          continue;
          }

        if(point->GetScalarAttribute("v", &v) == 0)
          {
          continue;
          }

        color = QColor::fromHsvF(h, s, v);
        }

      // Add the new color point to the color map.
      colorMap.addPoint(pqChartValue(px), color, pqChartValue(a));
      }
    else if (QString("NaN") == point->GetName())
      {
      double r = 0.25, g = 0.0, b = 0.0;

      if (point->GetScalarAttribute("r", &r) == 0)
        {
        continue;
        }
      if (point->GetScalarAttribute("g", &g) == 0)
        {
        continue;
        }
      if (point->GetScalarAttribute("b", &b) == 0)
        {
        continue;
        }

      QColor color = QColor::fromRgbF(r, g, b);
      colorMap.setNanColor(color);
      }
    else
      {
      // Unrecognized tag.  Ignore.
      }
    }

  if(colorMap.getNumberOfPoints() > 1)
    {
    this->Model->addColorMap(colorMap, name);
    }
}

void pqColorPresetManager::exportColorMap(const QModelIndex &index,
    vtkPVXMLElement *element)
{
  // Get the color map name from the preset model data.
  QString name = this->Model->data(index, Qt::DisplayRole).toString();
  if(!name.isEmpty())
    {
    element->SetAttribute("name", name.toAscii().data());
    }

  // Get the color space and points from the color map model.
  const char *spaceNames[] = {"RGB", "HSV", "Wrapped", "Lab", "Diverging"};
  const pqColorMapModel *colorMap = this->Model->getColorMap(index.row());
  if(colorMap)
    {
    element->SetAttribute("space", spaceNames[colorMap->getColorSpaceAsInt()]);
    for(int i = 0; i < colorMap->getNumberOfPoints(); i++)
      {
      QColor color;
      pqChartValue value, opacity;
      colorMap->getPointColor(i, color);
      colorMap->getPointValue(i, value);
      colorMap->getPointOpacity(i, opacity);
      vtkPVXMLElement *point = vtkPVXMLElement::New();
      point->SetName("Point");
      point->SetAttribute("x",
          QString::number(value.getDoubleValue()).toAscii().data());
      point->SetAttribute("o",
          QString::number(opacity.getDoubleValue()).toAscii().data());
      point->SetAttribute("r",
          QString::number(color.redF()).toAscii().data());
      point->SetAttribute("g",
          QString::number(color.greenF()).toAscii().data());
      point->SetAttribute("b",
          QString::number(color.blueF()).toAscii().data());
      element->AddNestedElement(point);
      point->Delete();
      }

    QColor color;
    colorMap->getNanColor(color);
    vtkPVXMLElement *nanElement = vtkPVXMLElement::New();
    nanElement->SetName("NaN");
    nanElement->SetAttribute("r",
                             QString::number(color.redF()).toAscii().data());
    nanElement->SetAttribute("g",
                             QString::number(color.greenF()).toAscii().data());
    nanElement->SetAttribute("b",
                             QString::number(color.blueF()).toAscii().data());
    element->AddNestedElement(nanElement);
    nanElement->Delete();
    }
}


