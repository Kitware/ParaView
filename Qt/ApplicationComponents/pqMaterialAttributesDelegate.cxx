/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
#include "pqMaterialAttributesDelegate.h"

#include "pqActiveObjects.h"
#include "pqColorChooserButton.h"
#include "pqDoubleSliderWidget.h"
#include "pqDoubleSpinBox.h"
#include "pqFileDialog.h"
#include "pqMaterialEditor.h"
#include "pqServer.h"
#include "pqTextureSelectorPropertyWidget.h"
#include "pqVectorWidget.h"

#include "vtkOSPRayMaterialLibrary.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSessionProxyManager.h"

#include <QCheckBox>
#include <QComboBox>
#include <QMetaProperty>
#include <QPainter>
#include <QPushButton>
#include <QVector2D>
#include <QVector3D>

//-----------------------------------------------------------------------------
pqMaterialAttributesDelegate::pqMaterialAttributesDelegate(QObject* parent)
  : QStyledItemDelegate(parent)
{
}

//-----------------------------------------------------------------------------
void pqMaterialAttributesDelegate::paint(
  QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
  QStyleOptionViewItem modOption = option;

  if (index.column() == 1)
  {
    QString attrName = index.sibling(index.row(), 0).data(Qt::EditRole).toString();
    QString matType =
      index.sibling(index.row(), 0)
        .data(static_cast<int>(pqMaterialEditor::ExtendedItemDataRole::PropertyValue))
        .toString();

    const auto& dic = vtkOSPRayMaterialLibrary::GetParametersDictionary();

    auto paramType = dic.at(matType.toStdString()).at(attrName.toStdString());

    QVariant variant =
      index.data(static_cast<int>(pqMaterialEditor::ExtendedItemDataRole::PropertyValue));

    switch (paramType)
    {
      case vtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB:
      {
        QColor c = variant.value<QColor>();

        // swatch
        int mg = static_cast<int>(0.3 * option.rect.height());
        QRect swatchRect = option.rect.marginsRemoved(QMargins(mg, mg, mg, mg));
        swatchRect.setWidth(swatchRect.height());

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(Qt::black);
        painter->setBrush(QBrush(c));
        painter->drawEllipse(swatchRect);
        painter->restore();

        modOption.rect.setLeft(swatchRect.right() + 4);
        modOption.text = QString("(%1, %2, %3)").arg(c.red()).arg(c.green()).arg(c.blue());
      }
      break;
      case vtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT: // normalized
        modOption.text = QString("%1 %").arg(100.0 * variant.value<double>());
        break;
      case vtkOSPRayMaterialLibrary::ParameterType::FLOAT:
        modOption.text = QString::number(variant.value<double>());
        break;
      case vtkOSPRayMaterialLibrary::ParameterType::BOOLEAN:
        modOption.text = variant.value<bool>() ? "true" : "false";
        break;
      case vtkOSPRayMaterialLibrary::ParameterType::VEC2:
      {
        QVector2D vec = variant.value<QVector2D>();
        modOption.text = QString("[%1, %2]").arg(vec.x()).arg(vec.y());
      }
      break;
      case vtkOSPRayMaterialLibrary::ParameterType::VEC3:
      {
        QVector3D vec = variant.value<QVector3D>();
        modOption.text = QString("[%1, %2, %3]").arg(vec.x()).arg(vec.y()).arg(vec.z());
      }
      break;
      case vtkOSPRayMaterialLibrary::ParameterType::FLOAT_DATA:
      {
        QVariantList list = variant.value<QVariantList>();
        modOption.text = "{ ";
        for (QVariant& v : list)
        {
          modOption.text += QString::number(v.value<double>());
          modOption.text += " ";
        }
        modOption.text += "}";
      }
      break;
      case vtkOSPRayMaterialLibrary::ParameterType::TEXTURE:
      {
        QString str = variant.toString();
        if (str.isEmpty())
        {
          str = "<None>";
        }

        modOption.text = str;
      }
      break;
      default:
        break;
    }
  }

  QStyledItemDelegate::paint(painter, modOption, index);
}

//-----------------------------------------------------------------------------
QWidget* pqMaterialAttributesDelegate::createEditor(
  QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
  if (index.column() == 0)
  {
    QComboBox* combo = new QComboBox(parent);

    QStringList paramsList;

    // add current material
    paramsList << index.data(Qt::EditRole).toString();

    pqMaterialEditor* materialEditor = qobject_cast<pqMaterialEditor*>(this->parent());
    if (materialEditor)
    {
      for (auto p : materialEditor->availableParameters())
      {
        paramsList << p.c_str();
      }
    }

    combo->insertItems(0, paramsList);

    return combo;
  }
  else if (index.column() == 1)
  {
    QModelIndex sibling = index.sibling(index.row(), 0);
    QString attrName = sibling.data(Qt::EditRole).toString();
    QString matType =
      sibling.data(static_cast<int>(pqMaterialEditor::ExtendedItemDataRole::PropertyValue))
        .toString();

    auto& dic = vtkOSPRayMaterialLibrary::GetParametersDictionary();
    auto paramType = dic.at(matType.toStdString()).at(attrName.toStdString());

    QVariant variant =
      index.data(static_cast<int>(pqMaterialEditor::ExtendedItemDataRole::PropertyValue));

    switch (paramType)
    {
      case vtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB:
      {
        pqColorChooserButton* button = new pqColorChooserButton(parent);
        QColor c = variant.value<QColor>();
        button->setChosenColor(c);
        button->setText(QString("%1, %2, %3").arg(c.red()).arg(c.green()).arg(c.blue()));

        QObject::connect(
          button, &pqColorChooserButton::chosenColorChanged, [button](const QColor& nc) {
            button->setText(QString("%1, %2, %3").arg(nc.red()).arg(nc.green()).arg(nc.blue()));
          });

        return button;
      }
      case vtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT: // normalized
      {
        pqDoubleSliderWidget* widget = new pqDoubleSliderWidget(parent);
        widget->setAutoFillBackground(true);
        widget->setValue(variant.value<double>() * 100.0);

        // reduce the input box size
        pqDoubleLineEdit* lineEdit = widget->findChild<pqDoubleLineEdit*>("DoubleLineEdit");
        lineEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        lineEdit->setMaximumSize(40, QWIDGETSIZE_MAX);

        return widget;
      }
      case vtkOSPRayMaterialLibrary::ParameterType::FLOAT:
      {
        pqDoubleSpinBox* valEdit = new pqDoubleSpinBox(parent);
        valEdit->setSingleStep(0.1);
        valEdit->setRange(0.0, 1000.0);
        valEdit->setValue(variant.value<double>());
        return valEdit;
      }
      case vtkOSPRayMaterialLibrary::ParameterType::BOOLEAN:
      {
        QCheckBox* checkbox = new QCheckBox(parent);
        checkbox->setAutoFillBackground(true);
        checkbox->setCheckState(variant.value<bool>() ? Qt::Checked : Qt::Unchecked);
        return checkbox;
      }
      case vtkOSPRayMaterialLibrary::ParameterType::VEC2:
      {
        pqVectorWidgetImpl<QVector2D, 2>* widget =
          new pqVectorWidgetImpl<QVector2D, 2>(variant.value<QVector2D>(), parent);
        return widget;
      }
      case vtkOSPRayMaterialLibrary::ParameterType::VEC3:
      {
        pqVectorWidgetImpl<QVector3D, 3>* widget =
          new pqVectorWidgetImpl<QVector3D, 3>(variant.value<QVector3D>(), parent);
        return widget;
      }
      case vtkOSPRayMaterialLibrary::ParameterType::TEXTURE:
      {
        auto editor = new QPushButton(parent);
        editor->setText(variant.value<QString>());
        QObject::connect(editor, &QPushButton::clicked, [editor](bool) {
          const QString filters = "Image files (*.png *.jpg *.bmp *.ppm)";
          pqServer* server = pqActiveObjects::instance().activeServer();
          pqFileDialog* dialog =
            new pqFileDialog(server, editor, tr("Open Texture:"), QString(), filters);
          dialog->setObjectName("LoadMaterialTextureDialog");
          dialog->setFileMode(pqFileDialog::ExistingFile);
          QObject::connect(dialog,
            // Qt independant version of qOverload, for not having to deal with Qt's API breaks
            static_cast<void (pqFileDialog::*)(const QList<QStringList>&)>(
              &pqFileDialog::filesSelected),
            [editor](const QList<QStringList>& files) {
              if (!files.empty() && !files[0].empty())
              {
                editor->setText(files[0][0]);
              }
            });

          dialog->open();
        });

        return editor;
      }
      default:
        return nullptr;
    }
  }
  return QStyledItemDelegate::createEditor(parent, option, index);
}

//-----------------------------------------------------------------------------
void pqMaterialAttributesDelegate::setModelData(
  QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
  QVariant newValue = editor->property(editor->metaObject()->userProperty().name());

  if (index.column() == 0)
  {
    model->setData(index, newValue, Qt::EditRole);
  }
  else if (index.column() == 1)
  {
    QModelIndex sibling = index.sibling(index.row(), 0);
    QString attrName = sibling.data(Qt::EditRole).toString();
    QString matType =
      sibling.data(static_cast<int>(pqMaterialEditor::ExtendedItemDataRole::PropertyValue))
        .toString();

    const auto& dic = vtkOSPRayMaterialLibrary::GetParametersDictionary();
    auto paramType = dic.at(matType.toStdString()).at(attrName.toStdString());

    switch (paramType)
    {
      case vtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT:
        newValue = newValue.toDouble() * 0.01;
        break;
      case vtkOSPRayMaterialLibrary::ParameterType::TEXTURE:
        // Get the text property of the button
        newValue = editor->property("text");
      default:
        break;
    }

    model->setData(
      index, newValue, static_cast<int>(pqMaterialEditor::ExtendedItemDataRole::PropertyValue));
  }
}
