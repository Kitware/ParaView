// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqOMETransferFunctionsPropertyWidget.h"
#include "ui_pqOMETransferFunctionsPropertyWidget.h"
#include "ui_pqOMETransferFunctionsPropertyWidgetPage.h"

#include "pqCoreUtilities.h"
#include "pqPresetDialog.h"
#include "pqProxyWidget.h"
#include "pqRescaleScalarRangeReaction.h"
#include "pqRescaleScalarRangeToCustomDialog.h"
#include "vtkCommand.h"
#include "vtkDiscretizableColorTransferFunction.h"
#include "vtkPiecewiseFunction.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMTransferFunctionProxy.h"
#include "vtkSMUncheckedPropertyHelper.h"

#include <QPointer>
#include <QScopedValueRollback>
#include <QVBoxLayout>
#include <QVector>
#include <cassert>
#include <set>
#include <string>

class pqOMETransferFunctionsPropertyWidget::pqInternals
{
public:
  Ui::OMETransferFunctionsPropertyWidget Ui;
  QVector<QPointer<QWidget>> Pages;
  vtkWeakPointer<vtkSMPropertyGroup> Group;
};

//-----------------------------------------------------------------------------
pqOMETransferFunctionsPropertyWidget::pqOMETransferFunctionsPropertyWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
  , Internals(new pqOMETransferFunctionsPropertyWidget::pqInternals())
{
  this->setChangeAvailableAsChangeFinished(true);

  auto& internals = (*this->Internals);
  internals.Group = smgroup;
  internals.Ui.setupUi(this);

  for (int idx = 0; idx < 10; ++idx)
  {
    const QString function = QString("Channel_%1").arg(idx + 1);
    vtkSMPropertyHelper helper(smgroup->GetProperty(function.toUtf8().data()));
    if (auto lut = helper.GetAsProxy())
    {
      QWidget* page = new QWidget(this);
      Ui::OMETransferFunctionsPropertyWidgetPage pageUi;
      pageUi.setupUi(page);

      QObject::connect(pageUi.InvertTransferFunctions, &QToolButton::clicked,
        [lut]() { vtkSMTransferFunctionProxy::InvertTransferFunction(lut); });

      QObject::connect(pageUi.ChoosePreset, &QToolButton::clicked,
        [this, lut]()
        {
          (void)this;
          vtkSMProxy* sof = vtkSMPropertyHelper(lut, "ScalarOpacityFunction", true).GetAsProxy();
          bool indexedLookup = vtkSMPropertyHelper(lut, "IndexedLookup", true).GetAsInt() != 0;
          pqPresetDialog dialog(pqCoreUtilities::mainWidget(),
            indexedLookup ? pqPresetDialog::SHOW_INDEXED_COLORS_ONLY
                          : pqPresetDialog::SHOW_NON_INDEXED_COLORS_ONLY);
          dialog.setCustomizableLoadColors(!indexedLookup);
          dialog.setCustomizableLoadOpacities(sof != nullptr);
          dialog.setCustomizableUsePresetRange(false);
          dialog.setCustomizableLoadAnnotations(false);
          QObject::connect(&dialog, &pqPresetDialog::applyPreset,
            [lut, sof, &dialog](const Json::Value&)
            {
              if (dialog.loadColors())
              {
                vtkSMTransferFunctionProxy::ApplyPreset(lut, dialog.currentPreset());
              }
              if (dialog.loadOpacities() && sof != nullptr)
              {
                vtkSMTransferFunctionProxy::ApplyPreset(sof, dialog.currentPreset());
              }
            });
          dialog.exec();
        });

      auto stc = vtkDiscretizableColorTransferFunction::SafeDownCast(lut->GetClientSideObject());
      pageUi.ColorEditor->initialize(stc, true, nullptr, false);
      pageUi.ColorEditor->setProperty("SM_PROPERTY", QString("RGBPoints;%1").arg(idx));

      auto pwf = stc ? stc->GetScalarOpacityFunction() : nullptr;
      pageUi.OpacityEditor->initialize(stc, false, pwf, true);
      pageUi.OpacityEditor->setProperty("SM_PROPERTY", QString("Points;%1").arg(idx));

      this->connect(pageUi.ColorEditor, SIGNAL(controlPointsModified()), SLOT(stcChanged()));
      this->connect(pageUi.ColorEditor, SIGNAL(chartRangeModified()), SLOT(stcChanged()));
      this->connect(pageUi.OpacityEditor, SIGNAL(controlPointsModified()), SLOT(pwfChanged()));
      this->connect(pageUi.OpacityEditor, SIGNAL(chartRangeModified()), SLOT(pwfChanged()));

      page->setObjectName(function);
      page->hide();
      internals.Pages.push_back(page);

      this->addPropertyLink(this, QString("RGBPoints;%1").arg(idx).toUtf8().data(),
        SIGNAL(xrgbPointsChanged()), lut, lut->GetProperty("RGBPoints"));

      auto sof = vtkSMPropertyHelper(lut, "ScalarOpacityFunction").GetAsProxy();
      this->addPropertyLink(this, QString("Points;%1").arg(idx).toUtf8().data(),
        SIGNAL(xvmsPointsChanged()), sof, sof->GetProperty("Points"));

      // hookup weight.
      if (auto weigthProp = smgroup->GetProperty(QString("%1Weight").arg(function).toUtf8().data()))
      {
        this->addPropertyLink(
          pageUi.Weight, "value", SIGNAL(valueChanged(double)), smproxy, weigthProp);
      }

      auto lutWidget = pageUi.ColorEditor;
      auto sofWidget = pageUi.OpacityEditor;
      auto callback = [lut, sof, lutWidget, sofWidget, this](double rmin, double rmax)
      {
        double range[2] = { rmin, rmax };
        vtkSMTransferFunctionProxy::RescaleTransferFunction(lut, range);
        vtkSMTransferFunctionProxy::RescaleTransferFunction(sof, range);
        this->stcChanged(lutWidget);
        this->pwfChanged(sofWidget);
      };

      QObject::connect(
        pageUi.ColorEditor, &pqTransferFunctionWidget::rangeHandlesRangeChanged, callback);
      QObject::connect(
        pageUi.OpacityEditor, &pqTransferFunctionWidget::rangeHandlesRangeChanged, callback);

      auto callback2 = [lut, lutWidget, sofWidget, this]()
      {
        pqRescaleScalarRangeToCustomDialog* dialog =
          pqRescaleScalarRangeReaction::rescaleScalarRangeToCustom(lut);
        if (dialog != nullptr)
        {
          QObject::connect(dialog, &pqRescaleScalarRangeToCustomDialog::apply,
            [=]()
            {
              this->stcChanged(lutWidget);
              this->pwfChanged(sofWidget);
            });
        }
      };

      QObject::connect(
        pageUi.ColorEditor, &pqTransferFunctionWidget::rangeHandlesDoubleClicked, callback2);
      QObject::connect(
        pageUi.OpacityEditor, &pqTransferFunctionWidget::rangeHandlesDoubleClicked, callback2);
    }
  }

  auto cvprop = smproxy->GetProperty("ChannelVisibilities");
  assert(cvprop != nullptr);
  pqCoreUtilities::connect(
    cvprop, vtkCommand::UncheckedPropertyModifiedEvent, this, SLOT(channelVisibilitiesChanged()));
  this->channelVisibilitiesChanged();
}

//-----------------------------------------------------------------------------
pqOMETransferFunctionsPropertyWidget::~pqOMETransferFunctionsPropertyWidget()
{
  auto& internals = (*this->Internals);
  for (const auto& page : internals.Pages)
  {
    delete page;
  }
}

//-----------------------------------------------------------------------------
void pqOMETransferFunctionsPropertyWidget::channelVisibilitiesChanged()
{
  auto& internals = (*this->Internals);
  auto smproxy = this->proxy();
  assert(smproxy != nullptr);
  vtkSMUncheckedPropertyHelper helper(smproxy, "ChannelVisibilities");

  std::set<std::string> values;
  for (unsigned int cc = 0; cc < helper.GetNumberOfElements(); ++cc)
  {
    values.insert(helper.GetAsString(cc));
  }

  internals.Ui.tabWidget->clear();
  for (const auto& page : internals.Pages)
  {
    if (values.find(page->objectName().toUtf8().toStdString()) != values.end())
    {
      internals.Ui.tabWidget->addTab(page, page->objectName());
      page->show();
    }
    else
    {
      page->hide();
    }
  }
}

//-----------------------------------------------------------------------------
void pqOMETransferFunctionsPropertyWidget::stcChanged(pqTransferFunctionWidget* tfWidget)
{
  tfWidget = tfWidget ? tfWidget : qobject_cast<pqTransferFunctionWidget*>(this->sender());
  assert(tfWidget);
  auto stc = vtkDiscretizableColorTransferFunction::SafeDownCast(tfWidget->scalarsToColors());

  QList<QVariant> values;
  for (int cc = 0; stc != nullptr && cc < stc->GetSize(); cc++)
  {
    double xrgbms[6];
    stc->GetNodeValue(cc, xrgbms);
    values.push_back(xrgbms[0]);
    values.push_back(xrgbms[1]);
    values.push_back(xrgbms[2]);
    values.push_back(xrgbms[3]);
  }

  QScopedValueRollback<bool> rollback(this->UpdatingProperty, true);
  auto pname = tfWidget->property("SM_PROPERTY").toString();
  this->setProperty(pname.toUtf8().data(), values);
  Q_EMIT this->xrgbPointsChanged();
}

//-----------------------------------------------------------------------------
void pqOMETransferFunctionsPropertyWidget::pwfChanged(pqTransferFunctionWidget* tfWidget)
{
  tfWidget = tfWidget ? tfWidget : qobject_cast<pqTransferFunctionWidget*>(this->sender());
  assert(tfWidget);
  auto pwf = tfWidget->piecewiseFunction();

  QList<QVariant> values;
  for (int cc = 0; pwf != nullptr && cc < pwf->GetSize(); cc++)
  {
    double xvms[4];
    pwf->GetNodeValue(cc, xvms);
    values.push_back(xvms[0]);
    values.push_back(xvms[1]);
    values.push_back(xvms[2]);
    values.push_back(xvms[3]);
  }
  QScopedValueRollback<bool> rollback(this->UpdatingProperty, true);
  auto pname = tfWidget->property("SM_PROPERTY").toString();
  this->setProperty(pname.toUtf8().data(), values);
  Q_EMIT this->xvmsPointsChanged();
}

//-----------------------------------------------------------------------------
bool pqOMETransferFunctionsPropertyWidget::event(QEvent* evt)
{
  if (!this->UpdatingProperty && evt->type() == QEvent::DynamicPropertyChange)
  {
    auto devt = dynamic_cast<QDynamicPropertyChangeEvent*>(evt);
    const auto pname = QString(devt->propertyName());
    const auto parts = pname.split(';');
    assert(parts.size() == 2);

    if (parts[0] == "RGBPoints")
    {
      this->setXrgbPoints(
        parts[1].toInt(), this->property(pname.toUtf8().data()).value<QList<QVariant>>());
    }
    else
    {
      assert(parts[0] == "Points");
      this->setXvmsPoints(
        parts[1].toInt(), this->property(pname.toUtf8().data()).value<QList<QVariant>>());
    }
  }

  return this->Superclass::event(evt);
}

//-----------------------------------------------------------------------------
void pqOMETransferFunctionsPropertyWidget::setXvmsPoints(int index, const QList<QVariant>& xvms)
{
  auto& internals = (*this->Internals);
  auto smgroup = internals.Group.GetPointer();
  assert(index < static_cast<int>(smgroup->GetNumberOfProperties()));

  auto lut = vtkSMPropertyHelper(smgroup->GetProperty(index)).GetAsProxy();
  auto pwf = vtkDiscretizableColorTransferFunction::SafeDownCast(lut->GetClientSideObject())
               ->GetScalarOpacityFunction();

  pwf->RemoveAllPoints();
  for (int cc = 0; (cc + 3) < xvms.size(); cc += 4)
  {
    pwf->AddPoint(xvms[cc].toDouble(), xvms[cc + 1].toDouble(), xvms[cc + 2].toDouble(),
      xvms[cc + 3].toDouble());
  }
}

//-----------------------------------------------------------------------------
void pqOMETransferFunctionsPropertyWidget::setXrgbPoints(int index, const QList<QVariant>& xrgb)
{
  auto& internals = (*this->Internals);
  auto smgroup = internals.Group.GetPointer();
  assert(index < static_cast<int>(smgroup->GetNumberOfProperties()));
  auto lut = vtkSMPropertyHelper(smgroup->GetProperty(index)).GetAsProxy();
  auto stc = vtkDiscretizableColorTransferFunction::SafeDownCast(lut->GetClientSideObject());

  stc->RemoveAllPoints();
  for (int cc = 0; (cc + 3) < xrgb.size(); cc += 4)
  {
    stc->AddRGBPoint(xrgb[cc].toDouble(), xrgb[cc + 1].toDouble(), xrgb[cc + 2].toDouble(),
      xrgb[cc + 3].toDouble());
  }
}
