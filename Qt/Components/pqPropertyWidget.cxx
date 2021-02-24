/*=========================================================================

   Program: ParaView
   Module: pqPropertyWidget.cxx

   Copyright (c) 2005-2012 Sandia Corporation, Kitware Inc.
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
#include "pqPropertyWidget.h"

#include "pqPropertiesPanel.h"
#include "pqPropertyWidgetDecorator.h"
#include "pqProxy.h"
#include "pqTimer.h"
#include "pqUndoStack.h"
#include "pqView.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDocumentation.h"
#include "vtkSMDomain.h"
#include "vtkSMProperty.h"

//-----------------------------------------------------------------------------
pqPropertyWidget::pqPropertyWidget(vtkSMProxy* smProxy, QWidget* parentObject)
  : QWidget(parentObject)
  , Proxy(smProxy)
  , Property(nullptr)
  , ChangeAvailableAsChangeFinished(true)
  , Selected(false)
  , Timer(new pqTimer())
{
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
  foreach (pqPropertyWidgetDecorator* decorator, this->Decorators)
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
    QString doc = pqProxy::rstToHtml(smproperty->GetDocumentation()->GetDescription()).c_str();
    doc = doc.trimmed();
    doc = doc.replace(QRegExp("\\s+"), " ");
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
  return this->Property->SetPanelVisibility(vis);
}

//-----------------------------------------------------------------------------
void pqPropertyWidget::apply()
{
  BEGIN_UNDO_SET("Property Changed");
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

bool pqPropertyWidget::isSingleRowItem() const
{
  return false;
}
