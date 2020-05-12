/*=========================================================================

   Program: ParaView
   Module:  pqSeriesEditorPropertyWidget.cxx

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
#include "pqSeriesEditorPropertyWidget.h"
#include "ui_pqSeriesEditorPropertyWidget.h"

#include "pqActiveObjects.h"
#include "pqAnnotationsModel.h"
#include "pqPropertiesPanel.h"
#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkNew.h"
#include "vtkPVXMLElement.h"
#include "vtkSMChartSeriesSelectionDomain.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMTransferFunctionProxy.h"
#include "vtkSetGet.h"
#include "vtkSmartPointer.h"
#include "vtkStringList.h"

#include <cassert>
#include <limits>

class pqSeriesAnnotationsModel : public pqAnnotationsModel
{
  typedef pqAnnotationsModel Superclass;

public:
  pqSeriesAnnotationsModel(QObject* parent = nullptr)
    : Superclass(parent)
  {
  }

  QVariant headerData(int section, Qt::Orientation orientation, int role) const override
  {
    if (orientation == Qt::Horizontal && (role == Qt::DisplayRole || role == Qt::ToolTipRole))
    {
      switch (section)
      {
        case VALUE:
          return role == Qt::DisplayRole ? tr("Variable") : tr("Series name");
        case VISIBILITY:
          return role == Qt::DisplayRole ? "" : tr("Toggle series visibility");
        case COLOR:
          return role == Qt::DisplayRole ? "" : tr("Set color to use for the series");
        case LABEL:
          return role == Qt::DisplayRole ? tr("Legend Name")
                                         : tr("Set the text to use for the series in the legend");
      }
    }

    return this->Superclass::headerData(section, orientation, role);
  }

  Qt::ItemFlags flags(const QModelIndex& idx) const override
  {
    auto value = this->Superclass::flags(idx);
    if (idx.column() == VALUE)
    {
      return value & ~Qt::ItemIsEditable;
    }

    return value;
  }

  void domainModified()
  {
    Q_EMIT this->dataChanged(
      this->index(0, 0), this->index(this->rowCount() - 1, NUMBER_OF_COLUMNS - 1));
  }
};

class pqSeriesEditorPropertyWidget::pqInternals
{
public:
  Ui::SeriesEditorPropertyWidget Ui;
  vtkNew<vtkEventQtSlotConnect> VTKConnector;
  QMap<QString, double> Thickness;
  QMap<QString, int> Style;
  QMap<QString, int> MarkerStyle;
  QMap<QString, double> MarkerSize;
  QMap<QString, int> PlotCorner;
  bool RefreshingWidgets;
  vtkSmartPointer<vtkSMTransferFunctionProxy> LookupTableProxy;
  pqSeriesAnnotationsModel* Model;
  QVector<int> PresetToSeriesMapping;

  pqInternals(pqSeriesEditorPropertyWidget* self)
    : RefreshingWidgets(false)
  {
    auto pxm = pqActiveObjects::instance().proxyManager();
    auto lutProxy =
      vtkSMTransferFunctionProxy::SafeDownCast(pxm->NewProxy("lookup_tables", "PVLookupTable"));
    this->LookupTableProxy.TakeReference(lutProxy);
    vtkSMPropertyHelper(lutProxy, "IndexedLookup").Set(1);

    this->Ui.setupUi(self);
    this->Ui.wdgLayout->setMargin(pqPropertiesPanel::suggestedMargin());
    this->Ui.wdgLayout->setHorizontalSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());
    this->Ui.wdgLayout->setVerticalSpacing(pqPropertiesPanel::suggestedVerticalSpacing());

    // give infinite maximum to line thickness and marker size
    this->Ui.MarkerSize->setMaximum(std::numeric_limits<double>::infinity());
    this->Ui.Thickness->setMaximum(std::numeric_limits<double>::infinity());

    this->Ui.SeriesTable->setLookupTableProxy(this->LookupTableProxy);
    this->Model = new pqSeriesAnnotationsModel(self);
    this->Ui.SeriesTable->setAnnotationsModel(this->Model);
    this->Ui.SeriesTable->setSupportsReorder(false);
    this->Ui.SeriesTable->allowsUserDefinedValues(false);
    this->Ui.SeriesTable->allowsRegexpMatching(true);
    this->Ui.SeriesTable->supportsOpacityMapping(false);
    this->Ui.SeriesTable->setColumnVisibility(pqAnnotationsModel::VISIBILITY, true);
  }
};
//=============================================================================

//-----------------------------------------------------------------------------
pqSeriesEditorPropertyWidget::pqSeriesEditorPropertyWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
  , Internals(nullptr)
{
  vtkSMProperty* visibilityProperty = smgroup->GetProperty("SeriesVisibility");
  if (!visibilityProperty)
  {
    qCritical("SeriesVisibility property is required by pqSeriesEditorPropertyWidget."
              " This widget is not going to work.");
    return;
  }

  this->Internals = new pqInternals(this);

  auto visDomain = visibilityProperty->FindDomain<vtkSMChartSeriesSelectionDomain>();
  this->Internals->Model->setVisibilityDomain(visDomain);

  Ui::SeriesEditorPropertyWidget& ui = this->Internals->Ui;

  this->Internals->VTKConnector->Connect(smgroup->GetProperty("SeriesVisibility"),
    vtkCommand::DomainModifiedEvent, this, SLOT(domainModified(vtkObject*)));

  if (smgroup->GetProperty("SeriesLabel"))
  {
    this->addPropertyLink(this, "presetLabel", SIGNAL(presetLabelChanged()),
      this->Internals->LookupTableProxy->GetProperty("Annotations"));

    this->addPropertyLink(
      this, "seriesLabel", SIGNAL(seriesLabelChanged()), smgroup->GetProperty("SeriesLabel"));

    // Trigger an update of the internal lut
    Q_EMIT this->presetLabelChanged();
    this->apply();
  }
  else
  {
    ui.SeriesTable->setColumnVisibility(pqAnnotationsModel::LABEL, false);
  }

  this->addPropertyLink(this, "seriesVisibility", SIGNAL(seriesVisibilityChanged()),
    smgroup->GetProperty("SeriesVisibility"));

  if (smgroup->GetProperty("SeriesColor"))
  {
    this->addPropertyLink(this, "presetColor", SIGNAL(presetColorChanged()),
      this->Internals->LookupTableProxy->GetProperty("IndexedColors"));

    this->addPropertyLink(
      this, "seriesColor", SIGNAL(seriesColorChanged()), smgroup->GetProperty("SeriesColor"));

    // Trigger an update of the internal lut
    Q_EMIT this->presetColorChanged();
    this->apply();
  }
  else
  {
    ui.SeriesTable->setColumnVisibility(pqAnnotationsModel::COLOR, false);
  }

  if (smgroup->GetProperty("SeriesLineThickness"))
  {
    this->addPropertyLink(this, "seriesLineThickness", SIGNAL(seriesLineThicknessChanged()),
      smgroup->GetProperty("SeriesLineThickness"));
  }
  else
  {
    ui.ThicknessLabel->hide();
    ui.Thickness->hide();
  }

  if (smgroup->GetProperty("SeriesLineStyle"))
  {
    this->addPropertyLink(this, "seriesLineStyle", SIGNAL(seriesLineStyleChanged()),
      smgroup->GetProperty("SeriesLineStyle"));
  }
  else
  {
    ui.StyleListLabel->hide();
    ui.StyleList->hide();
  }

  if (smgroup->GetProperty("SeriesMarkerStyle"))
  {
    this->addPropertyLink(this, "seriesMarkerStyle", SIGNAL(seriesMarkerStyleChanged()),
      smgroup->GetProperty("SeriesMarkerStyle"));
  }
  else
  {
    ui.MarkerStyleListLabel->hide();
    ui.MarkerStyleList->hide();
  }

  if (smgroup->GetProperty("SeriesMarkerSize"))
  {
    this->addPropertyLink(this, "seriesMarkerSize", SIGNAL(seriesMarkerSizeChanged()),
      smgroup->GetProperty("SeriesMarkerSize"));
  }
  else
  {
    ui.MarkerSizeLabel->hide();
    ui.MarkerSize->hide();
  }

  if (smgroup->GetProperty("SeriesPlotCorner"))
  {
    this->addPropertyLink(this, "seriesPlotCorner", SIGNAL(seriesPlotCornerChanged()),
      smgroup->GetProperty("SeriesPlotCorner"));
  }
  else
  {
    ui.AxisListLabel->hide();
    ui.AxisList->hide();
  }

  this->connect(ui.Thickness, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
    &pqSeriesEditorPropertyWidget::savePropertiesWidgets);
  this->connect(ui.StyleList, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
    &pqSeriesEditorPropertyWidget::savePropertiesWidgets);
  this->connect(ui.MarkerStyleList, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
    &pqSeriesEditorPropertyWidget::savePropertiesWidgets);
  this->connect(ui.MarkerSize, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
    &pqSeriesEditorPropertyWidget::savePropertiesWidgets);
  this->connect(ui.AxisList, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
    &pqSeriesEditorPropertyWidget::savePropertiesWidgets);

  this->connect(ui.SeriesTable, &pqColorAnnotationsWidget::indexedColorsChanged, this,
    &pqSeriesEditorPropertyWidget::seriesColorChanged);
  this->connect(ui.SeriesTable, &pqColorAnnotationsWidget::presetChanged, this,
    &pqSeriesEditorPropertyWidget::onPresetChanged);
  this->connect(ui.SeriesTable, &pqColorAnnotationsWidget::annotationsChanged, this,
    &pqSeriesEditorPropertyWidget::seriesLabelChanged);
  this->connect(ui.SeriesTable, &pqColorAnnotationsWidget::visibilitiesChanged, this,
    &pqSeriesEditorPropertyWidget::seriesVisibilityChanged);

  this->connect(ui.SeriesTable, &pqColorAnnotationsWidget::selectionChanged, this,
    &pqSeriesEditorPropertyWidget::refreshPropertiesWidgets);

  this->connect(ui.SeriesTable, &pqColorAnnotationsWidget::indexedColorsChanged, this,
    &pqSeriesEditorPropertyWidget::presetColorChanged);
  this->connect(ui.SeriesTable, &pqColorAnnotationsWidget::indexedColorsChanged, this,
    &pqSeriesEditorPropertyWidget::seriesColorChanged);

  auto lastNameProp = smgroup->GetProperty("LastPresetName");
  if (!lastNameProp)
  {
    ui.SeriesTable->enablePresets(false);
  }
  else
  {
    auto name = vtkSMPropertyHelper(lastNameProp).GetAsString();
    this->Internals->Ui.SeriesTable->setCurrentPresetName(name);
  }
}

//-----------------------------------------------------------------------------
pqSeriesEditorPropertyWidget::~pqSeriesEditorPropertyWidget()
{
  delete this->Internals;
  this->Internals = nullptr;
}

//-----------------------------------------------------------------------------
void pqSeriesEditorPropertyWidget::setSeriesVisibility(const QList<QVariant>& values)
{
  this->Internals->Ui.SeriesTable->setVisibilities(values);
}

//-----------------------------------------------------------------------------
QList<QVariant> pqSeriesEditorPropertyWidget::seriesVisibility() const
{
  return this->Internals->Ui.SeriesTable->visibilities();
}

//-----------------------------------------------------------------------------
void pqSeriesEditorPropertyWidget::setPresetLabel(const QList<QVariant>& vtkNotUsed(values))
{
  // handle by `onPresetChanged`.
  return;
}

//-----------------------------------------------------------------------------
QList<QVariant> pqSeriesEditorPropertyWidget::presetLabel() const
{
  return this->Internals->Ui.SeriesTable->annotations();
}

//-----------------------------------------------------------------------------
void pqSeriesEditorPropertyWidget::setSeriesLabel(const QList<QVariant>& values)
{
  this->Internals->Ui.SeriesTable->setAnnotations(values);
}

//-----------------------------------------------------------------------------
QList<QVariant> pqSeriesEditorPropertyWidget::seriesLabel() const
{
  return this->Internals->Ui.SeriesTable->annotations();
}

//-----------------------------------------------------------------------------
void pqSeriesEditorPropertyWidget::setPresetColor(const QList<QVariant>& vtkNotUsed(values))
{
  // handle by `onPresetChanged`.
  return;
}

//-----------------------------------------------------------------------------
QList<QVariant> pqSeriesEditorPropertyWidget::presetColor() const
{
  return this->Internals->Ui.SeriesTable->indexedColors();
}

//-----------------------------------------------------------------------------
void pqSeriesEditorPropertyWidget::setSeriesColor(const QList<QVariant>& values)
{
  // values has the format 'name r g b name r g b ...'
  QList<QVariant> colors;
  for (int cc = 0; cc < values.size(); cc++)
  {
    if (cc % 4 != 0)
    {
      colors.push_back(values[cc]);
    }
  }

  this->Internals->Ui.SeriesTable->setIndexedColors(colors);
}

//-----------------------------------------------------------------------------
QList<QVariant> pqSeriesEditorPropertyWidget::seriesColor() const
{
  const QList<QVariant>& colors = this->Internals->Ui.SeriesTable->indexedColors();
  const QList<QVariant>& labels = this->seriesLabel();

  QList<QVariant> reply;
  for (int cc = 0; cc < colors.size() / 3; cc++)
  {
    reply.push_back(labels[2 * cc]);
    reply.push_back(colors[3 * cc]);
    reply.push_back(colors[3 * cc + 1]);
    reply.push_back(colors[3 * cc + 2]);
  }
  return reply;
}

namespace
{
template <class T>
void setSeriesValues(QMap<QString, T>& data, const QList<QVariant>& values)
{
  data.clear();
  for (int cc = 0; (cc + 1) < values.size(); cc += 2)
  {
    data[values[cc].toString()] = values[cc + 1].value<T>();
  }
}

template <class T>
void getSeriesValues(const QMap<QString, T>& data, QList<QVariant>& reply)
{
  typename QMap<QString, T>::const_iterator iter = data.constBegin();
  for (; iter != data.constEnd(); ++iter)
  {
    reply.push_back(iter.key());
    reply.push_back(QString::number(iter.value()));
  }
}
}

//-----------------------------------------------------------------------------
void pqSeriesEditorPropertyWidget::setSeriesLineThickness(const QList<QVariant>& values)
{
  setSeriesValues<double>(this->Internals->Thickness, values);
  this->refreshPropertiesWidgets();
}

//-----------------------------------------------------------------------------
QList<QVariant> pqSeriesEditorPropertyWidget::seriesLineThickness() const
{
  QList<QVariant> reply;
  getSeriesValues<double>(this->Internals->Thickness, reply);
  return reply;
}

//-----------------------------------------------------------------------------
void pqSeriesEditorPropertyWidget::setSeriesLineStyle(const QList<QVariant>& values)
{
  setSeriesValues<int>(this->Internals->Style, values);
  this->refreshPropertiesWidgets();
}

//-----------------------------------------------------------------------------
QList<QVariant> pqSeriesEditorPropertyWidget::seriesLineStyle() const
{
  QList<QVariant> reply;
  getSeriesValues(this->Internals->Style, reply);
  return reply;
}

//-----------------------------------------------------------------------------
void pqSeriesEditorPropertyWidget::setSeriesMarkerStyle(const QList<QVariant>& values)
{
  setSeriesValues<int>(this->Internals->MarkerStyle, values);
  this->refreshPropertiesWidgets();
}

//-----------------------------------------------------------------------------
QList<QVariant> pqSeriesEditorPropertyWidget::seriesMarkerStyle() const
{
  QList<QVariant> reply;
  getSeriesValues(this->Internals->MarkerStyle, reply);
  return reply;
}

//-----------------------------------------------------------------------------
void pqSeriesEditorPropertyWidget::setSeriesMarkerSize(const QList<QVariant>& values)
{
  setSeriesValues<double>(this->Internals->MarkerSize, values);
  this->refreshPropertiesWidgets();
}

//-----------------------------------------------------------------------------
QList<QVariant> pqSeriesEditorPropertyWidget::seriesMarkerSize() const
{
  QList<QVariant> reply;
  getSeriesValues<double>(this->Internals->MarkerSize, reply);
  return reply;
}

//-----------------------------------------------------------------------------
void pqSeriesEditorPropertyWidget::setSeriesPlotCorner(const QList<QVariant>& values)
{
  setSeriesValues<int>(this->Internals->PlotCorner, values);
  this->refreshPropertiesWidgets();
}

//-----------------------------------------------------------------------------
QList<QVariant> pqSeriesEditorPropertyWidget::seriesPlotCorner() const
{
  QList<QVariant> reply;
  getSeriesValues(this->Internals->PlotCorner, reply);
  return reply;
}

//-----------------------------------------------------------------------------
void pqSeriesEditorPropertyWidget::refreshPropertiesWidgets()
{
  Ui::SeriesEditorPropertyWidget& ui = this->Internals->Ui;

  QString key = ui.SeriesTable->currentAnnotationValue();

  if (key.isEmpty())
  {
    // nothing is selected, disable all properties widgets.
    ui.AxisList->setEnabled(false);
    ui.MarkerStyleList->setEnabled(false);
    ui.MarkerSize->setEnabled(false);
    ui.StyleList->setEnabled(false);
    ui.Thickness->setEnabled(false);
    return;
  }

  this->Internals->RefreshingWidgets = true;
  ui.Thickness->setValue(this->Internals->Thickness[key]);
  ui.Thickness->setEnabled(true);

  ui.StyleList->setCurrentIndex(this->Internals->Style[key]);
  ui.StyleList->setEnabled(true);

  ui.MarkerStyleList->setCurrentIndex(this->Internals->MarkerStyle[key]);
  ui.MarkerStyleList->setEnabled(true);

  ui.MarkerSize->setValue(this->Internals->MarkerSize[key]);
  ui.MarkerSize->setEnabled(true);

  ui.AxisList->setCurrentIndex(this->Internals->PlotCorner[key]);
  ui.AxisList->setEnabled(true);
  this->Internals->RefreshingWidgets = false;
}

//-----------------------------------------------------------------------------
void pqSeriesEditorPropertyWidget::savePropertiesWidgets()
{
  if (this->Internals->RefreshingWidgets)
  {
    return;
  }

  Ui::SeriesEditorPropertyWidget& ui = this->Internals->Ui;

  QWidget* senderWidget = qobject_cast<QWidget*>(this->sender());
  assert(senderWidget);

  QStringList selectedAnnotations = ui.SeriesTable->selectedAnnotations();

  for (auto key : selectedAnnotations)
  {
    // update the parameter corresponding to the modified widget.
    if (ui.Thickness == senderWidget && this->Internals->Thickness[key] != ui.Thickness->value())
    {
      this->Internals->Thickness[key] = ui.Thickness->value();
      Q_EMIT this->seriesLineThicknessChanged();
    }
    else if (ui.StyleList == senderWidget &&
      this->Internals->Style[key] != ui.StyleList->currentIndex())
    {
      this->Internals->Style[key] = ui.StyleList->currentIndex();
      Q_EMIT this->seriesLineStyleChanged();
    }
    else if (ui.MarkerStyleList == senderWidget &&
      this->Internals->MarkerStyle[key] != ui.MarkerStyleList->currentIndex())
    {
      this->Internals->MarkerStyle[key] = ui.MarkerStyleList->currentIndex();
      Q_EMIT this->seriesMarkerStyleChanged();
    }
    else if (ui.MarkerSize == senderWidget &&
      this->Internals->MarkerSize[key] != ui.MarkerSize->value())
    {
      this->Internals->MarkerSize[key] = ui.MarkerSize->value();
      Q_EMIT this->seriesMarkerSizeChanged();
    }
    else if (ui.AxisList == senderWidget &&
      this->Internals->PlotCorner[key] != ui.AxisList->currentIndex())
    {
      this->Internals->PlotCorner[key] = ui.AxisList->currentIndex();
      Q_EMIT this->seriesPlotCornerChanged();
    }
  }
}

//-----------------------------------------------------------------------------
void pqSeriesEditorPropertyWidget::domainModified(vtkObject*)
{
  this->Internals->Model->domainModified();
}

//-----------------------------------------------------------------------------
void pqSeriesEditorPropertyWidget::onPresetChanged(const QString& name)
{
  auto annotationsProp = vtkSMStringVectorProperty::SafeDownCast(
    this->Internals->LookupTableProxy->GetProperty("Annotations"));
  auto colorsProp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->Internals->LookupTableProxy->GetProperty("IndexedColors"));

  if (!annotationsProp || !colorsProp || colorsProp->GetNumberOfElements() == 0)
  {
    vtkWarningWithObjectMacro(
      nullptr, "Cannot find Annotations or IndexedColors property for series.");
    return;
  }

  vtkNew<vtkStringList> presetAnnotations;
  annotationsProp->GetElements(presetAnnotations);

  const QList<QVariant>& currentAnnotations = this->Internals->Ui.SeriesTable->annotations();
  QList<QVariant> newAnnotations = currentAnnotations;
  QList<QVariant> newColors;

  // create a set of lists with preset values filtered through the given regexp.
  QMap<QStringList, int> filteredPresetStringLists;
  QRegularExpression regexp = this->Internals->Ui.SeriesTable->presetRegularExpression();
  for (int j = 0; j < presetAnnotations->GetNumberOfStrings(); j += 2)
  {
    auto presetStringList = QStringList(presetAnnotations->GetString(j));
    if (regexp.isValid() && !regexp.pattern().isEmpty())
    {
      auto presetMatching = regexp.match(presetAnnotations->GetString(j));
      if (presetMatching.hasMatch())
      {
        presetStringList = presetMatching.capturedTexts();
        if (presetStringList.count() > 1)
        {
          // Remove the full match when capturing groups
          presetStringList.removeFirst();
        }
      }
    }
    filteredPresetStringLists.insert(presetStringList, j);
  }

  // We want to keep current order.
  for (int i = 0; i + 1 < currentAnnotations.size(); i += 2)
  {
    int idx = -1;
    if (presetAnnotations->GetLength() > 0)
    {
      QByteArray nameArray = currentAnnotations.at(i).toString().toLocal8Bit();
      const char* seriesName = nameArray.data();

      auto seriesStringlist = QStringList(seriesName);
      if (regexp.isValid() && !regexp.pattern().isEmpty())
      {
        auto matching = regexp.match(seriesName);
        if (matching.hasMatch())
        {
          seriesStringlist = matching.capturedTexts();
          if (seriesStringlist.count() > 1)
          {
            // Remove the full match when capturing groups
            seriesStringlist.removeFirst();
          }
        }
      }

      if (filteredPresetStringLists.contains(seriesStringlist))
      {
        idx = filteredPresetStringLists[seriesStringlist] / 2;
      }

      // if found, update annotation legend
      if (this->Internals->Ui.SeriesTable->presetLoadAnnotations() && idx > -1 &&
        (idx + 1) < presetAnnotations->GetLength())
      {
        newAnnotations[i + 1] = presetAnnotations->GetString(2 * idx + 1);
      }
    }

    if (idx == -1)
    {
      idx = i / 2;
    }

    newColors.push_back(QVariant(currentAnnotations.at(i)));

    idx = idx % (colorsProp->GetNumberOfElements() / 3);
    newColors.push_back(vtkSMPropertyHelper(colorsProp).GetAsDouble(3 * idx));
    newColors.push_back(vtkSMPropertyHelper(colorsProp).GetAsDouble(3 * idx + 1));
    newColors.push_back(vtkSMPropertyHelper(colorsProp).GetAsDouble(3 * idx + 2));
  }

  this->setSeriesColor(newColors);
  if (this->Internals->Ui.SeriesTable->presetLoadAnnotations())
  {
    this->setSeriesLabel(newAnnotations);
  }

  auto presetProp = this->proxy()->GetProperty("LastPresetName");
  vtkSMPropertyHelper(presetProp).Set(name.toStdString().c_str());
}
