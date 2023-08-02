// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqMaterialAttributesDelegate.h"

#include "pqActiveObjects.h"
#include "pqColorChooserButton.h"
#include "pqDialog.h"
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

#include "vtksys/SystemTools.hxx"

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QMetaProperty>
#include <QPainter>
#include <QPointer>
#include <QPushButton>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

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
          str = "Edit";
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
      case vtkOSPRayMaterialLibrary::ParameterType::VEC4:
      {
        pqVectorWidgetImpl<QVector4D, 4>* widget =
          new pqVectorWidgetImpl<QVector4D, 4>(variant.value<QVector4D>(), parent);
        return widget;
      }
      case vtkOSPRayMaterialLibrary::ParameterType::FLOAT_DATA:
        return nullptr;
      case vtkOSPRayMaterialLibrary::ParameterType::TEXTURE:
      {
        auto list = variant.toList();
        return this->createPropertiesEditor(list, parent);
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
      {
        QVariantList list = this->getPropertiesFromEditor(editor);
        newValue = list;
      }
      break;
      default:
        break;
    }

    model->setData(
      index, newValue, static_cast<int>(pqMaterialEditor::ExtendedItemDataRole::PropertyValue));
  }
}

//-----------------------------------------------------------------------------
QVariantList pqMaterialAttributesDelegate::getPropertiesFromEditor(QWidget* editor) const
{
  QVariantList variantList;
  QGridLayout* layout = static_cast<QGridLayout*>(editor->layout());
  if (!layout)
  {
    QPushButton* btn = static_cast<QPushButton*>(editor);
    pqDialog* child = btn->findChild<pqDialog*>("map properties dialog");
    if (child)
    {
      layout = static_cast<QGridLayout*>(child->layout());
    }
  }

  if (!layout)
  {
    qWarning() << "Can't data information from the map properties editor.";
    return variantList;
  }

  // Layout, created in createPropertiesEditor(), always follows this structure:
  // [QLabel A, QWidget* A, QLabel B, QWidget* B, ...]
  for (int i = 0; i < layout->rowCount(); i++)
  {
    auto* itemLabel = layout->itemAtPosition(i, 0);
    QLabel* widgetLabel = qobject_cast<QLabel*>(itemLabel->widget());
    if (!widgetLabel)
    {
      qWarning() << "Map properties editor haven't be correctly generated.";
      return variantList;
    }
    variantList.append(widgetLabel->text());

    auto* itemValue = layout->itemAtPosition(i, 1);
    auto* widgetValue = itemValue->widget();

    std::string label = widgetLabel->text().toStdString();
    if (label.find(".rotation") != std::string::npos)
    {
      auto* spinWidget = qobject_cast<pqDoubleSpinBox*>(widgetValue);
      variantList.append(spinWidget->value());
    }
    else if (label.find(".scale") != std::string::npos ||
      label.find(".translation") != std::string::npos)
    {
      auto* vector2Widget = dynamic_cast<pqVectorWidgetImpl<QVector2D, 2>*>(widgetValue);
      variantList.append(vector2Widget->value());
    }
    else if (label.find(".") == std::string::npos)
    {
      auto* btn = qobject_cast<QPushButton*>(widgetValue);
      auto* hiddenItemValue = layout->itemAtPosition(i, 2);
      auto* hiddenWidgetValue = dynamic_cast<QLabel*>(hiddenItemValue->widget());
      QString path = hiddenWidgetValue->text();
      variantList.append(path);
      break;
    }
  }

  return variantList;
}

//-----------------------------------------------------------------------------
QWidget* pqMaterialAttributesDelegate::createPropertiesEditor(
  QVariantList list, QWidget* parent) const
{
  auto editor = new QPushButton(parent);
  editor->setText("Edit");

  QObject::connect(editor, &QPushButton::clicked, [editor, parent, list](bool) {
    pqDialog* dialog = new pqDialog(editor);

    dialog->setObjectName("map properties dialog");
    dialog->setWindowTitle("Map Properties");
    QGridLayout* layout = new QGridLayout(dialog);
    dialog->setGeometry(0, 0, 300, 100);
    layout->setSizeConstraint(QLayout::SetFixedSize);
    dialog->setLayout(layout);

    int currentLine = 0;
    for (std::size_t i = 0; i < list.size(); i += 2)
    {
      QVariant variantName = list[i];
      QPointer<QLabel> variableLabel = new QLabel(editor);
      variableLabel->setText(variantName.toString());
      layout->addWidget(variableLabel, currentLine, 0);

      QVariant variantValue = list[i + 1];
      switch (variantValue.type())
      {
        case QMetaType::Double:
        {
          QPointer<pqDoubleSpinBox> valEdit = new pqDoubleSpinBox(editor);
          valEdit->setSingleStep(0.1);
          valEdit->setRange(0.0, 1000.0);
          valEdit->setValue(variantValue.value<double>());
          layout->addWidget(valEdit, currentLine, 1);
        }
        break;
        case QMetaType::QVector2D:
        {
          QPointer<pqVectorWidgetImpl<QVector2D, 2>> widget =
            new pqVectorWidgetImpl<QVector2D, 2>(variantValue.value<QVector2D>(), editor);
          layout->addWidget(widget, currentLine, 1);
        }
        break;
        case QMetaType::QString:
        {
          QString btnTxt = variantValue.value<QString>();
          if (btnTxt.isEmpty())
          {
            btnTxt = "Select the texture";
          }
          else
          {
            btnTxt = vtksys::SystemTools::GetFilenameName(btnTxt.toStdString()).c_str();
          }

          auto btn = new QPushButton(editor);
          btn->setText(btnTxt);
          layout->addWidget(btn, currentLine, 1);

          auto* hiddenLabelForTexture = new QLabel(editor);
          hiddenLabelForTexture->setText(variantValue.value<QString>());
          hiddenLabelForTexture->setHidden(true);
          layout->addWidget(hiddenLabelForTexture, currentLine, 2);

          QObject::connect(btn, &QPushButton::clicked, [btn, hiddenLabelForTexture](bool) {
            const QString filters = tr("Image files") + " (*.png *.jpg *.bmp *.ppm)";
            pqServer* server = pqActiveObjects::instance().activeServer();
            pqFileDialog* dialog =
              new pqFileDialog(server, btn, tr("Open Texture:"), QString(), filters);
            dialog->setObjectName("LoadMaterialTextureDialog");
            dialog->setFileMode(pqFileDialog::ExistingFile);
            QObject::connect(dialog,
              // Qt independant version of qOverload, for not having to deal with Qt's API
              // breaks
              static_cast<void (pqFileDialog::*)(const QList<QStringList>&)>(
                &pqFileDialog::filesSelected),
              [btn, hiddenLabelForTexture](const QList<QStringList>& files) {
                if (!files.empty() && !files[0].empty())
                {
                  std::string fileFullPath = files[0][0].toStdString();
                  btn->setText(vtksys::SystemTools::GetFilenameName(fileFullPath).c_str());
                  hiddenLabelForTexture->setText(files[0][0]);
                }
              });

            dialog->open();
          });
        }
        break;
        default:
          qWarning() << "variant type: " << variantValue.typeName() << " isn't supported.";
          break;
      }

      currentLine++;
    }

    QPointer<QPushButton> okBtn = new QPushButton(editor);
    okBtn->setText("Ok");
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setVerticalSpacing(10);
    layout->setHorizontalSpacing(10);
    layout->addWidget(okBtn, currentLine, 1);
    QObject::connect(okBtn, &QPushButton::clicked, [dialog, editor, parent](bool) {
      dialog->close();
      editor->close();
    });

    dialog->open();
  });

  return editor;
}
