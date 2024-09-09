// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqPropertyWidget.h"
#include <QCoreApplication>

#include "pqPropertiesPanel.h"
#include "pqPropertyWidgetDecorator.h"
#include "pqProxy.h"
#include "pqTimer.h"
#include "pqUndoStack.h"
#include "pqView.h"
#include "vtkCollection.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDocumentation.h"
#include "vtkSMDomain.h"
#include "vtkSMProperty.h"

#include <QRegularExpression>

//-----------------------------------------------------------------------------
pqPropertyWidget::pqPropertyWidget(vtkSMProxy* smProxy, QWidget* parentObject)
  : QFrame(parentObject)
  , Proxy(smProxy)
  , Property(nullptr)
  , ChangeAvailableAsChangeFinished(true)
  , Selected(false)
  , Timer(new pqTimer())
{
  this->setFrameShape(QFrame::NoFrame);
  this->ShowLabel = true;
  this->Links.setAutoUpdateVTKObjects(false);
  this->Links.setUseUncheckedProperties(true);

  this->connect(&this->Links, SIGNAL(qtWidgetChanged()), this, SIGNAL(changeAvailable()));

  // This has to be a QueuedConnection otherwise changeFinished() gets fired
  // before changeAvailable() is handled by pqProxyWidget and see BUG #13029.
  this->Timer->setSingleShot(true);
  this->Timer->setInterval(0);
  this->Timer->connect(this, SIGNAL(changeAvailable()), SLOT(start()));
  this->connect(this->Timer.data(), SIGNAL(timeout()), SLOT(onChangeAvailable()));
}

//-----------------------------------------------------------------------------
pqPropertyWidget::~pqPropertyWidget()
{
  Q_FOREACH (pqPropertyWidgetDecorator* decorator, this->Decorators)
  {
    delete decorator;
  }

  this->Decorators.clear();
}

//-----------------------------------------------------------------------------
void pqPropertyWidget::onChangeAvailable()
{
  if (this->ChangeAvailableAsChangeFinished)
  {
    Q_EMIT this->changeFinished();
  }
}

//-----------------------------------------------------------------------------
pqView* pqPropertyWidget::view() const
{
  return this->View;
}

//-----------------------------------------------------------------------------
void pqPropertyWidget::setView(pqView* pqview)
{
  this->View = pqview;
  Q_EMIT this->viewChanged(pqview);
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqPropertyWidget::proxy() const
{
  return this->Proxy;
}

//-----------------------------------------------------------------------------
QString pqPropertyWidget::getTooltip(vtkSMProperty* smproperty)
{
  if (smproperty && smproperty->GetDocumentation())
  {
    QString doc = pqProxy::rstToHtml(QCoreApplication::translate(
      "ServerManagerXML", smproperty->GetDocumentation()->GetDescription()));
    doc = doc.trimmed();
    doc = doc.replace(QRegularExpression("\\s+"), " ");
    return QString("<html><head/><body><p align=\"justify\">%1</p></body></html>").arg(doc);
  }
  return QString();
}

//-----------------------------------------------------------------------------
void pqPropertyWidget::setProperty(vtkSMProperty* smproperty)
{
  this->Property = smproperty;
  this->setToolTip(pqPropertyWidget::getTooltip(smproperty));
  if ((smproperty->GetHints() &&
        smproperty->GetHints()->FindNestedElementByName("RestartRequired")))
  {
    this->connect(this, SIGNAL(changeAvailable()), SIGNAL(restartRequired()));
  }
}

//-----------------------------------------------------------------------------
vtkSMProperty* pqPropertyWidget::property() const
{
  return this->Property;
}

//-----------------------------------------------------------------------------
char* pqPropertyWidget::panelVisibility() const
{
  return this->Property->GetPanelVisibility();
}

//-----------------------------------------------------------------------------
void pqPropertyWidget::setPanelVisibility(const char* vis)
{
  this->Property->SetPanelVisibility(vis);
}

//-----------------------------------------------------------------------------
void pqPropertyWidget::apply()
{
  BEGIN_UNDO_SET(tr("Property Changed"));
  this->Links.accept();
  END_UNDO_SET();
}

//-----------------------------------------------------------------------------
void pqPropertyWidget::reset()
{
  this->Links.reset();
}

//-----------------------------------------------------------------------------
void pqPropertyWidget::setShowLabel(bool isLabelVisible)
{
  this->ShowLabel = isLabelVisible;
}

//-----------------------------------------------------------------------------
bool pqPropertyWidget::showLabel() const
{
  return this->ShowLabel;
}

//-----------------------------------------------------------------------------
void pqPropertyWidget::addPropertyLink(QObject* qobject, const char* qproperty, const char* qsignal,
  vtkSMProperty* smproperty, int smindex)
{
  this->Links.addPropertyLink(qobject, qproperty, qsignal, this->Proxy, smproperty, smindex);
}

//-----------------------------------------------------------------------------
void pqPropertyWidget::addPropertyLink(QObject* qobject, const char* qproperty, const char* qsignal,
  vtkSMProxy* smproxy, vtkSMProperty* smproperty, int smindex)
{
  this->Links.addPropertyLink(qobject, qproperty, qsignal, smproxy, smproperty, smindex);
}

//-----------------------------------------------------------------------------
void pqPropertyWidget::removePropertyLink(QObject* qobject, const char* qproperty,
  const char* qsignal, vtkSMProperty* smproperty, int smindex)
{
  this->Links.removePropertyLink(qobject, qproperty, qsignal, this->Proxy, smproperty, smindex);
}

//-----------------------------------------------------------------------------
void pqPropertyWidget::removePropertyLink(QObject* qobject, const char* qproperty,
  const char* qsignal, vtkSMProxy* smproxy, vtkSMProperty* smproperty, int smindex)
{
  this->Links.removePropertyLink(qobject, qproperty, qsignal, smproxy, smproperty, smindex);
}

//-----------------------------------------------------------------------------
void pqPropertyWidget::addDecorator(pqPropertyWidgetDecorator* decorator)
{
  if (!decorator || decorator->parent() != this)
  {
    qCritical("Either the decorator is NULL or has an invalid parent."
              "Please check the code.");
  }
  else
  {
    this->Decorators.push_back(decorator);
  }
}

//-----------------------------------------------------------------------------
void pqPropertyWidget::removeDecorator(pqPropertyWidgetDecorator* decorator)
{
  this->Decorators.removeAll(decorator);
}

//-----------------------------------------------------------------------------
int pqPropertyWidget::hintsWidgetHeightNumberOfRows(vtkPVXMLElement* hints, int defaultValue)
{
  if (vtkPVXMLElement* element = hints ? hints->FindNestedElementByName("WidgetHeight") : nullptr)
  {
    int rowCount = 0;
    if (element->GetScalarAttribute("number_of_rows", &rowCount))
    {
      return rowCount;
    }
  }
  return defaultValue;
}

//-----------------------------------------------------------------------------
std::vector<std::string> pqPropertyWidget::parseComponentLabels(
  vtkPVXMLElement* hints, unsigned int elemCount)
{
  if (hints == nullptr)
  {
    return {};
  }

  vtkNew<vtkCollection> elements;
  hints->GetElementsByName("ComponentLabel", elements.GetPointer());

  const int nbCompLabels = elements->GetNumberOfItems();
  std::vector<std::string> componentLabels;
  componentLabels.resize((elemCount != 0) ? elemCount : nbCompLabels);

  for (int i = 0; i < nbCompLabels; ++i)
  {
    vtkPVXMLElement* labelElement = vtkPVXMLElement::SafeDownCast(elements->GetItemAsObject(i));
    if (labelElement)
    {
      int component = 0;
      if (labelElement->GetScalarAttribute("component", &component))
      {
        if (static_cast<std::size_t>(component) < componentLabels.size())
        {
          componentLabels[component] = labelElement->GetAttributeOrEmpty("label");
        }
      }
    }
  }

  return componentLabels;
}

//-----------------------------------------------------------------------------
bool pqPropertyWidget::isSingleRowItem() const
{
  return false;
}

//-----------------------------------------------------------------------------
void pqPropertyWidget::setReadOnly(bool readOnly)
{
  this->setEnabled(!readOnly);
}
