// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqMultiBlockPropertiesStateWidget.h"

#include "pqCoreUtilities.h"
#include "pqHighlightableToolButton.h"
#include "pqPropertiesPanel.h"
#include "pqUndoStack.h"

#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkSMColorMapEditorHelper.h"
#include "vtkSMDataAssemblyDomain.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkWeakPointer.h"

#include <QAction>
#include <QBrush>
#include <QColor>
#include <QCoreApplication>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QObject>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QPointF>
#include <QPointer>
#include <QString>

#include <string>
#include <utility>
#include <vector>

//-----------------------------------------------------------------------------
class pqMultiBlockPropertiesStateWidget::pqInternals : public QObject
{
public:
  pqInternals() { this->ColorMapEditorHelper->SetSelectedPropertiesTypeToBlocks(); }
  ~pqInternals() override = default;

  vtkWeakPointer<vtkSMProxy> Representation;
  std::vector<std::string> BlockPropertyNames;
  int IconSize;
  QString Selector;
  BlockPropertyState CurrentState;

  QPointer<QHBoxLayout> HLayout;
  QPointer<QLabel> StateLabel;
  QPointer<pqHighlightableToolButton> ResetButton;
  QPixmap Icons[BlockPropertyState::NumberOfStates];
  QString ToolTips[BlockPropertyState::NumberOfStates];

  vtkNew<vtkSMColorMapEditorHelper> ColorMapEditorHelper;

  /**
   * The set/blockInherited/representationInherited colors were selected from the IBM Design library
   * palette. See https://davidmathlogic.com/colorblind.
   */
  static QPixmap statePixMap(vtkSMColorMapEditorHelper::BlockPropertyState state, int iconSize,
    QColor setColor = QColor("#D91D86") /*magenta*/,
    QColor blockInheritedColor = QColor("#F6BB00") /*yellow*/,
    QColor representationInheritedColor = QColor("#3665B7"), /*blue*/
    QColor penColor = QColor(Qt::black), QColor backgroundColor = QColor(Qt::white))
  {
    QPixmap pixmap(iconSize, iconSize);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    QPen pen = painter.pen();
    QBrush brush = painter.brush();

    // Check each state individually and draw corresponding shapes
    const double circumCircleRadius = iconSize / 2.1;
    const double circumCircleRadius3_4 = circumCircleRadius * 3.0 / 4.0;
    const double circumCircleRadius1_2 = circumCircleRadius / 2.0;
    const QPointF center(iconSize / 2.0, iconSize / 2.0);
    if (state == vtkSMColorMapEditorHelper::BlockPropertyState::Disabled)
    {
      pen.setWidth(iconSize / 20);
      pen.setColor(penColor);
      pen.setStyle(Qt::SolidLine);
      painter.setPen(pen);
      painter.setBrush(Qt::NoBrush);
      // Draw a circle
      painter.drawEllipse(center, circumCircleRadius, circumCircleRadius);
      return pixmap;
    }
    else // (state >= vtkSMColorMapEditorHelper::BlockPropertyState::RepresentationInherited)
    {
      pen.setWidth(iconSize / 20);
      pen.setColor(penColor);
      pen.setStyle(Qt::SolidLine);
      painter.setPen(pen);
      painter.setBrush(backgroundColor);
      // Draw a circle
      painter.drawEllipse(center, circumCircleRadius, circumCircleRadius);
    }
    if (state & vtkSMColorMapEditorHelper::BlockPropertyState::RepresentationInherited)
    {
      painter.setPen(Qt::NoPen);
      // Draw an annulus
      painter.setBrush(representationInheritedColor);
      painter.drawEllipse(center, circumCircleRadius, circumCircleRadius);
      painter.setBrush(backgroundColor);
      painter.drawEllipse(center, circumCircleRadius3_4, circumCircleRadius3_4);
    }
    if (state & vtkSMColorMapEditorHelper::BlockPropertyState::BlockInherited)
    {
      painter.setPen(Qt::NoPen);
      // Draw an annulus
      painter.setBrush(blockInheritedColor);
      painter.drawEllipse(center, circumCircleRadius3_4, circumCircleRadius3_4);
      painter.setBrush(backgroundColor);
      painter.drawEllipse(center, circumCircleRadius1_2, circumCircleRadius1_2);
    }
    if (state & vtkSMColorMapEditorHelper::BlockPropertyState::Set)
    {
      painter.setPen(Qt::NoPen);
      // Draw a circle
      painter.setBrush(setColor);
      painter.drawEllipse(center, circumCircleRadius1_2, circumCircleRadius1_2);
    }
    return pixmap;
  }
};

//-----------------------------------------------------------------------------
pqMultiBlockPropertiesStateWidget::pqMultiBlockPropertiesStateWidget(vtkSMProxy* repr,
  const QList<QString>& propertyNames, int iconSize, QString selector, QWidget* parent,
  Qt::WindowFlags f)
  : Superclass(parent, f)
  , Internals(new pqMultiBlockPropertiesStateWidget::pqInternals())
{
  auto& internals = *this->Internals;
  internals.Representation = repr;
  internals.IconSize = iconSize;
  internals.CurrentState = BlockPropertyState::NumberOfStates; // invalid state
  internals.Selector = selector;

  // populate the properties
  for (const auto& propertyName : propertyNames)
  {
    const std::string propName = propertyName.toStdString();
    auto prop = vtkSMStringVectorProperty::SafeDownCast(repr->GetProperty(propName.c_str()));
    if (vtkSMColorMapEditorHelper::GetPropertyType(prop) == vtkSMColorMapEditorHelper::Blocks)
    {
      internals.BlockPropertyNames.push_back(propName);
    }
  }

  // create the icons
  // for (int state = 0; state < BlockPropertyState::NumberOfStates; ++state)
  // {
  //   QPixmap pixmap = internals.statePixMap(static_cast<BlockPropertyState>(state), 512);
  //   QString fileName = QFileDialog::getSaveFileName(this, "Save Image", "", "PNG Files (*.png)");
  //   pixmap.save(fileName, "PNG");
  // }
  // load the icons
  internals.Icons[BlockPropertyState::Disabled] =
    QIcon(":/pqWidgets/Icons/pqStateDisabled.svg").pixmap(internals.IconSize, internals.IconSize);
  internals.Icons[BlockPropertyState::RepresentationInherited] =
    QIcon(":/pqWidgets/Icons/pqStateRepresentationInherited.svg")
      .pixmap(internals.IconSize, internals.IconSize);
  internals.Icons[BlockPropertyState::BlockInherited] =
    QIcon(":/pqWidgets/Icons/pqStateBlockInherited.svg")
      .pixmap(internals.IconSize, internals.IconSize);
  internals.Icons[BlockPropertyState::MixedInherited] =
    QIcon(":/pqWidgets/Icons/pqStateMixedInherited.svg")
      .pixmap(internals.IconSize, internals.IconSize);
  internals.Icons[BlockPropertyState::Set] =
    QIcon(":/pqWidgets/Icons/pqStateSet.svg").pixmap(internals.IconSize, internals.IconSize);
  internals.Icons[BlockPropertyState::SetAndRepresentationInherited] =
    QIcon(":/pqWidgets/Icons/pqStateSetAndRepresentationInherited.svg")
      .pixmap(internals.IconSize, internals.IconSize);
  internals.Icons[BlockPropertyState::SetAndBlockInherited] =
    QIcon(":/pqWidgets/Icons/pqStateSetAndBlockInherited.svg")
      .pixmap(internals.IconSize, internals.IconSize);
  internals.Icons[BlockPropertyState::SetAndMixedInherited] =
    QIcon(":/pqWidgets/Icons/pqStateSetAndMixedInherited.svg")
      .pixmap(internals.IconSize, internals.IconSize);
  // create the tooltips
  const QString toolTipFirstPart =
    internals.BlockPropertyNames.size() > 1 ? tr("Properties are ") : tr("Property is ");
  internals.ToolTips[BlockPropertyState::Disabled] =
    toolTipFirstPart + tr("disabled because no blocks are selected");
  internals.ToolTips[BlockPropertyState::RepresentationInherited] =
    toolTipFirstPart + tr("inherited from the representation");
  internals.ToolTips[BlockPropertyState::BlockInherited] =
    toolTipFirstPart + tr("inherited from block(s)");
  internals.ToolTips[BlockPropertyState::MixedInherited] =
    toolTipFirstPart + tr("inherited from block(s) and the representation");
  internals.ToolTips[BlockPropertyState::Set] = toolTipFirstPart + tr("set in block(s)");
  internals.ToolTips[BlockPropertyState::SetAndRepresentationInherited] =
    toolTipFirstPart + tr("set in block(s) and inherited from the representation");
  internals.ToolTips[BlockPropertyState::SetAndBlockInherited] =
    toolTipFirstPart + tr("set in block(s) and inherited from block(s)");
  internals.ToolTips[BlockPropertyState::SetAndMixedInherited] =
    toolTipFirstPart + tr("set in block(s) and inherited from block(s) and the representation");

  // create the layout
  internals.HLayout = new QHBoxLayout(this);
  internals.HLayout->setSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());
  internals.HLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
  internals.HLayout->setContentsMargins(pqPropertiesPanel::suggestedMargin(),
    pqPropertiesPanel::suggestedMargin(), pqPropertiesPanel::suggestedMargin(),
    pqPropertiesPanel::suggestedMargin());
  internals.HLayout->setStretch(0, 1);

  // create the reset button
  internals.ResetButton = new pqHighlightableToolButton(this);
  internals.ResetButton->setObjectName("Reset");
  internals.ResetButton->setIcon(QIcon(":/pqWidgets/Icons/pqReset.svg"));
  internals.ResetButton->setIconSize(QSize(iconSize - 4, iconSize - 4));
  internals.ResetButton->setToolTip(tr("Reset to value(s) inherited from block/representation"));

  // create the state label
  internals.StateLabel = new QLabel("BlockPropertyState", this);
  internals.StateLabel->setToolTip(internals.ToolTips[BlockPropertyState::Disabled]);
  internals.StateLabel->setPixmap(internals.Icons[BlockPropertyState::Disabled]);

  // add the widgets to the layout
  internals.HLayout->addWidget(internals.StateLabel);
  internals.HLayout->addWidget(internals.ResetButton);

  // set the initial state
  this->updateState();

  // connect the signals
  for (const auto& propertyName : internals.BlockPropertyNames)
  {
    pqCoreUtilities::connect(repr->GetProperty(propertyName.c_str()), vtkCommand::ModifiedEvent,
      this, SLOT(onPropertiesChanged()));
  }
  if (this->Internals->Selector.isEmpty())
  {
    if (vtkSMProperty* selectedBlockSelectors = repr->GetProperty("SelectedBlockSelectors"))
    {
      pqCoreUtilities::connect(selectedBlockSelectors, vtkCommand::ModifiedEvent, this,
        SLOT(onSelectedBlockSelectorsChanged()));
    }
  }
  QObject::connect(internals.ResetButton, &pqHighlightableToolButton::clicked, this,
    &pqMultiBlockPropertiesStateWidget::onResetButtonClicked);
  if (auto selectors = vtkSMStringVectorProperty::SafeDownCast(repr->GetProperty("Selectors")))
  {
    if (auto domain = selectors->FindDomain<vtkSMDataAssemblyDomain>())
    {
      pqCoreUtilities::connect(
        domain, vtkCommand::DomainModifiedEvent, this, SLOT(onResetButtonClicked()));
    }
  }
}

//-----------------------------------------------------------------------------
pqMultiBlockPropertiesStateWidget::~pqMultiBlockPropertiesStateWidget() = default;

//-----------------------------------------------------------------------------
void pqMultiBlockPropertiesStateWidget::updateState()
{
  auto& internals = *this->Internals;
  const std::vector<std::string> blockSelectors = this->getSelectors();
  const std::pair<std::string, BlockPropertyState> selectorAndState =
    internals.ColorMapEditorHelper->HasBlocksProperties(
      internals.Representation, blockSelectors, internals.BlockPropertyNames);
  const BlockPropertyState state = selectorAndState.second;
  if (internals.CurrentState != selectorAndState.second)
  {
    internals.CurrentState = state;

    internals.StateLabel->setPixmap(internals.Icons[state]);
    internals.StateLabel->setToolTip(internals.ToolTips[state]);
    internals.StateLabel->setEnabled(state != BlockPropertyState::Disabled);

    internals.ResetButton->setEnabled(state >= BlockPropertyState::Set);

    Q_EMIT this->stateChanged(state);
  }
}

//-----------------------------------------------------------------------------
pqMultiBlockPropertiesStateWidget::BlockPropertyState pqMultiBlockPropertiesStateWidget::getState()
  const
{
  return this->Internals->CurrentState;
}

//-----------------------------------------------------------------------------
QPixmap pqMultiBlockPropertiesStateWidget::getStatePixmap(BlockPropertyState state) const
{
  return this->Internals->Icons[state];
}

//-----------------------------------------------------------------------------
QString pqMultiBlockPropertiesStateWidget::getStateToolTip(BlockPropertyState state) const
{
  return this->Internals->ToolTips[state];
}

//-----------------------------------------------------------------------------
pqHighlightableToolButton* pqMultiBlockPropertiesStateWidget::getResetButton() const
{
  return this->Internals->ResetButton;
}

//-----------------------------------------------------------------------------
std::vector<std::string> pqMultiBlockPropertiesStateWidget::getProperties() const
{
  return this->Internals->BlockPropertyNames;
}

//-----------------------------------------------------------------------------
std::vector<std::string> pqMultiBlockPropertiesStateWidget::getSelectors() const
{
  if (!this->Internals->Selector.isEmpty())
  {
    return { this->Internals->Selector.toStdString() };
  }
  else
  {
    return this->Internals->ColorMapEditorHelper->GetSelectedBlockSelectors(
      this->Internals->Representation);
  }
}

//-----------------------------------------------------------------------------
void pqMultiBlockPropertiesStateWidget::onPropertiesChanged()
{
  this->updateState();
}

//-----------------------------------------------------------------------------
void pqMultiBlockPropertiesStateWidget::onSelectedBlockSelectorsChanged()
{
  const BlockPropertyState oldState = this->getState();
  this->updateState();
  // if the state remains the same, we still need to emit a signal,
  // because the selected block selectors have changed
  if (oldState == this->getState())
  {
    Q_EMIT this->selectedBlockSelectorsChanged();
  }
}

//-----------------------------------------------------------------------------
void pqMultiBlockPropertiesStateWidget::onResetButtonClicked()
{
  Q_EMIT this->startStateReset();
  auto& internals = *this->Internals;
  auto selectedBlockSelectors = this->getSelectors();
  for (const auto& propertyName : internals.BlockPropertyNames)
  {
    const QString undoText = tr("Reset ") +
      QCoreApplication::translate("ServerManagerXML",
        internals.Representation->GetProperty(propertyName.c_str())->GetXMLLabel());
    BEGIN_UNDO_SET(undoText);
    internals.ColorMapEditorHelper->ResetBlocksProperty(
      internals.Representation, selectedBlockSelectors, propertyName);
    // onPropertiesChanged will be invoked because the property is modified
    END_UNDO_SET();
  }
  Q_EMIT this->endStateReset();
}
