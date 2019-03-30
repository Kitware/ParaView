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
#include "pqProxyWidget.h"

#include "pqApplicationCore.h"
#include "pqCommandPropertyWidget.h"
#include "pqDisplayPanel.h"
#include "pqDisplayPanelInterface.h"
#include "pqDisplayPanelPropertyWidget.h"
#include "pqDoubleVectorPropertyWidget.h"
#include "pqIntVectorPropertyWidget.h"
#include "pqInterfaceTracker.h"
#include "pqPropertiesPanel.h"
#include "pqPropertyWidget.h"
#include "pqPropertyWidgetDecorator.h"
#include "pqPropertyWidgetInterface.h"
#include "pqProxyPropertyWidget.h"
#include "pqRepresentation.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqStringVectorPropertyWidget.h"
#include "pqTimer.h"
#include "vtkCollection.h"
#include "vtkNew.h"
#include "vtkPVLogger.h"
#include "vtkPVXMLElement.h"
#include "vtkSMCompoundSourceProxy.h"
#include "vtkSMDocumentation.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMNamedPropertyIterator.h"
#include "vtkSMOrderedPropertyIterator.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSettings.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMTrace.h"
#include "vtkSmartPointer.h"
#include "vtkStringList.h"
#include "vtkWeakPointer.h"

#include <QHideEvent>
#include <QLabel>
#include <QPointer>
#include <QShowEvent>
#include <QVBoxLayout>

#include <cassert>
#include <cmath>
#include <list>
#include <map>
#include <sstream>
#include <utility>
#include <vector>

//-----------------------------------------------------------------------------------
namespace
{
QFrame* newHLine(QWidget* parent)
{
  QFrame* line = new QFrame(parent);
  line->setFrameShadow(QFrame::Sunken);
  line->setFrameShape(QFrame::HLine);
  line->setLineWidth(1);
  line->setMidLineWidth(0);
  return line;
}

QWidget* newGroupSeparator(QWidget* parent)
{
  QWidget* widget = new QWidget(parent);
  QVBoxLayout* vbox = new QVBoxLayout(widget);
  vbox->setContentsMargins(0, 0, 0, pqPropertiesPanel::suggestedVerticalSpacing());
  vbox->setSpacing(0);
  vbox->addWidget(newHLine(widget));
  return widget;
}

std::vector<vtkWeakPointer<vtkPVXMLElement> > get_decorators(vtkPVXMLElement* hints)
{
  std::vector<vtkWeakPointer<vtkPVXMLElement> > decoratorTypes;
  vtkNew<vtkCollection> collection;
  if (hints)
  {
    hints->FindNestedElementByName("PropertyWidgetDecorator", collection.GetPointer());
  }
  for (int cc = 0; cc < collection->GetNumberOfItems(); cc++)
  {
    vtkPVXMLElement* elem = vtkPVXMLElement::SafeDownCast(collection->GetItemAsObject(cc));
    if (elem && elem->GetAttribute("type"))
    {
      decoratorTypes.push_back(elem);
    }
  }
  return decoratorTypes;
}

void add_decorators(pqPropertyWidget* widget, vtkPVXMLElement* hints)
{
  if (widget && hints)
  {
    auto xmls = get_decorators(hints);
    for (auto xml : xmls)
    {
      assert(xml && xml->GetAttribute("type"));
      pqPropertyWidgetDecorator::create(xml, widget);
    }
  }
}

std::string get_group_label(vtkSMPropertyGroup* smgroup)
{
  assert(smgroup != nullptr);
  auto label = smgroup->GetXMLLabel();
  if (label && label[0] != '\0')
  {
    return std::string(label);
  }
  else
  {
    // generate a unique string.
    std::ostringstream str;
    str << "__smgroup:" << smgroup;
    return str.str();
  }
}

void DetermineLegacyHiddenProperties(QSet<QString>& properties, vtkSMProxy* proxy)
{
  vtkPVXMLElement* hints = proxy->GetHints();
  if (!hints)
  {
    return;
  }

  for (unsigned int cc = 0; cc < hints->GetNumberOfNestedElements(); cc++)
  {
    vtkPVXMLElement* child = hints->GetNestedElement(cc);
    if (child->GetName() && strcmp(child->GetName(), "Property") == 0 &&
      child->GetAttribute("name") && strcmp(child->GetAttributeOrEmpty("show"), "0") == 0)
    {
      properties.insert(child->GetAttribute("name"));
    }
  }
}

// class corresponding to a single widget for a property/properties.
class pqProxyWidgetItem : public QObject
{
  typedef QObject Superclass;
  QPointer<QWidget> GroupHeader;
  QPointer<QWidget> GroupFooter;
  QPointer<QWidget> LabelWidget;
  QPointer<pqPropertyWidget> PropertyWidget;
  QStringList DefaultVisibilityForRepresentations;
  bool Group;
  QString GroupTag;

  pqProxyWidgetItem(QObject* parentObj)
    : Superclass(parentObj)
    , Group(false)
    , GroupTag()
    , Advanced(false)
    , InformationOnly(false)
  {
  }

public:
  // Regular expression with tags used to match search text.
  QStringList SearchTags;
  bool Advanced;
  bool InformationOnly;

  ~pqProxyWidgetItem() override
  {
    delete this->GroupHeader;
    delete this->GroupFooter;
    delete this->LabelWidget;
    delete this->PropertyWidget;
  }

  static pqProxyWidgetItem* newItem(
    pqPropertyWidget* widget, const QString& label, QObject* parentObj)
  {
    pqProxyWidgetItem* item = new pqProxyWidgetItem(parentObj);
    item->PropertyWidget = widget;
    if (!label.isEmpty() && widget->showLabel())
    {
      QLabel* labelWdg = new QLabel(QString("<p>%1</p>").arg(label), widget->parentWidget());
      labelWdg->setWordWrap(true);
      labelWdg->setAlignment(Qt::AlignLeft | Qt::AlignTop);
      item->LabelWidget = labelWdg;
    }
    return item;
  }

  /// Creates a new item for a property group. Use this overload when creating
  /// an item for a group where there's a single widget for all the properties
  /// in that group.
  static pqProxyWidgetItem* newGroupItem(
    pqPropertyWidget* widget, const QString& label, QObject* parentObj)
  {
    pqProxyWidgetItem* item = newItem(widget, QString(), parentObj);
    item->Group = true;
    if (!label.isEmpty() && widget->showLabel())
    {
      item->GroupHeader = pqProxyWidget::newGroupLabelWidget(label, widget->parentWidget());
      item->GroupFooter = newGroupSeparator(widget->parentWidget());
    }
    return item;
  }

  /// Creates a new item for a property group with several widgets (for
  /// individual properties in the group).
  static pqProxyWidgetItem* newMultiItemGroupItem(const QString& group_label,
    pqPropertyWidget* widget, const QString& widget_label, QObject* parentObj)
  {
    pqProxyWidgetItem* item = newItem(widget, widget_label, parentObj);
    item->Group = true;
    // ensure GroupTag is not null for multi-property groups.
    item->GroupTag = group_label.isNull() ? QString("") : group_label;
    if (!group_label.isEmpty())
    {
      item->GroupHeader = pqProxyWidget::newGroupLabelWidget(group_label, widget->parentWidget());
      item->GroupFooter = newGroupSeparator(widget->parentWidget());
    }
    return item;
  }

  pqPropertyWidget* propertyWidget() const { return this->PropertyWidget; }

  void appendToDefaultVisibilityForRepresentations(const QString& val)
  {
    if (!val.isEmpty() && !this->DefaultVisibilityForRepresentations.contains(val))
    {
      this->DefaultVisibilityForRepresentations.append(val);
    }
  }

  void apply() const
  {
    if (this->PropertyWidget)
    {
      this->PropertyWidget->apply();
    }
  }
  void reset() const
  {
    if (this->PropertyWidget)
    {
      this->PropertyWidget->reset();
    }
  }

  void select() const
  {
    if (this->PropertyWidget)
    {
      this->PropertyWidget->select();
    }
  }

  void deselect() const
  {
    if (this->PropertyWidget)
    {
      this->PropertyWidget->deselect();
    }
  }

  bool canShowWidget(bool show_advanced, const QString& filterText, vtkSMProxy* proxy) const
  {
    if (show_advanced == false && this->isAdvanced(proxy) == true)
    {
      // skip advanced properties.
      return false;
    }
    else if (filterText.isEmpty() == false &&
      this->SearchTags.filter(filterText, Qt::CaseInsensitive).size() == 0)
    {
      // skip properties not matching search criteria.
      return false;
    }

    foreach (const pqPropertyWidgetDecorator* decorator, this->PropertyWidget->decorators())
    {
      if (decorator && !decorator->canShowWidget(show_advanced))
      {
        return false;
      }
    }

    return true;
  }

  bool enableWidget() const
  {
    if (this->InformationOnly)
    {
      return false;
    }
    foreach (const pqPropertyWidgetDecorator* decorator, this->PropertyWidget->decorators())
    {
      if (decorator && !decorator->enableWidget())
      {
        return false;
      }
    }

    return true;
  }

  bool isAdvanced(vtkSMProxy* proxy) const
  {
    if (this->DefaultVisibilityForRepresentations.size() > 0 && proxy &&
      proxy->GetProperty("Representation") &&
      this->DefaultVisibilityForRepresentations.contains(
        vtkSMPropertyHelper(proxy, "Representation").GetAsString(), Qt::CaseInsensitive))
    {
      return false;
    }
    return this->Advanced;
  }

  void show(
    const pqProxyWidgetItem* prevVisibleItem, bool enabled = true, bool show_advanced = false) const
  {
    // If `this` is not a group, but previous item was, we need to ensure the
    // previous items group footer is visible.
    if (!this->Group && prevVisibleItem && prevVisibleItem->Group && prevVisibleItem->GroupFooter)
    {
      prevVisibleItem->GroupFooter->show();
    }

    // If `this` is a group, and belongs to different group than
    // prevVisibleItem then we need to show `this`'s header.
    if (this->GroupHeader)
    {
      this->GroupHeader->setVisible(this->GroupTag.isNull() || prevVisibleItem == nullptr ||
        this->GroupTag != prevVisibleItem->GroupTag);
    }

    if (this->LabelWidget)
    {
      this->LabelWidget->show();
    }

    this->PropertyWidget->updateWidget(show_advanced);
    this->PropertyWidget->setEnabled(enabled);
    this->PropertyWidget->show();

    if (this->GroupFooter)
    {
      // I know what you're thinking: "Typo!!!", it's not. It's deliberate. A
      // footer should only be shown by a next item. It's only needed if a
      // following item wants it.
      this->GroupFooter->hide();
    }
  }

  void hide() const
  {
    if (this->GroupHeader)
    {
      this->GroupHeader->hide();
    }
    if (this->LabelWidget)
    {
      this->LabelWidget->hide();
    }
    this->PropertyWidget->hide();
    if (this->GroupFooter)
    {
      this->GroupFooter->hide();
    }
  }

  /// Adds widgets to the layout. This is a little greedy. It adds everything
  /// that could be potentially shown to the layout. We control visibilities
  /// of things like headers and footers dynamically in show()/hide().
  void appendToLayout(QGridLayout* glayout, bool singleColumn)
  {
    if (this->GroupHeader)
    {
      glayout->addWidget(this->GroupHeader, glayout->rowCount(), 0, 1, -1);
    }
    if (this->LabelWidget)
    {
      const int row = glayout->rowCount();
      if (singleColumn)
      {
        glayout->addWidget(this->LabelWidget, row, 0, 1, -1);
        glayout->addWidget(this->PropertyWidget, (row + 1), 0, 1, -1);
      }
      else
      {
        glayout->addWidget(this->LabelWidget, row, 0, Qt::AlignTop | Qt::AlignLeft);
        glayout->addWidget(this->PropertyWidget, row, 1);
      }
    }
    else
    {
      glayout->addWidget(this->PropertyWidget, glayout->rowCount(), 0, 1, -1);
    }
    if (this->GroupFooter)
    {
      glayout->addWidget(this->GroupFooter, glayout->rowCount(), 0, 1, -1);
    }
  }

private:
  Q_DISABLE_COPY(pqProxyWidgetItem)
};

//--------------------------------------------------------------------------------------------------
// Returns true if the property should be skipped from the panel.
bool skip_property(vtkSMProperty* smproperty, const std::string& key,
  const QStringList& chosenProperties, const QSet<QString>& legacyHiddenProperties)
{
  const QString skey = QString(key.c_str());
  const QString simplifiedKey = QString(key.c_str()).remove(' ');

  if (!chosenProperties.isEmpty() &&
    !(chosenProperties.contains(simplifiedKey) || chosenProperties.contains(skey)))
  {
    vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "skip since not in chosen properties set.");
    return true;
  }

  if (smproperty->GetIsInternal())
  {
    // skip internal properties
    vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "skip since internal.");
    return true;
  }

  if (smproperty->GetPanelVisibility() && strcmp(smproperty->GetPanelVisibility(), "never") == 0 &&
    chosenProperties.size() == 0)
  {
    vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(),
      "skip since marked as never show (unless it was explicitly 'chosen').");
    return true;
  }

  if (chosenProperties.size() == 0 &&
    (legacyHiddenProperties.contains(skey) || legacyHiddenProperties.contains(simplifiedKey)))
  {
    // skipping properties marked with "show=0" in the hints section.
    vtkVLogF(
      PARAVIEW_LOG_APPLICATION_VERBOSITY(), "skip since legacy hint with `show=0` specified.");
    return true;
  }

  return false;
}

//--------------------------------------------------------------------------------------------------
// Returns true if the group should be skipped from the panel.
bool skip_group(vtkSMPropertyGroup* smgroup, const QStringList& chosenProperties)
{
  if (!chosenProperties.empty() && !chosenProperties.contains(smgroup->GetXMLLabel()))
  {
    vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "skip since group not chosen.");
    return true;
  }

  if (smgroup->GetPanelVisibility() && strcmp(smgroup->GetPanelVisibility(), "never") == 0 &&
    chosenProperties.size() == 0)
  {
    // skip property groups marked as never show
    vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "skip since group visibility is set to `never`");
    return true;
  }
  return false;
}
} // end of namespace {}

//-----------------------------------------------------------------------------------
QWidget* pqProxyWidget::newGroupLabelWidget(const QString& labelText, QWidget* parent)
{
  QWidget* widget = new QWidget(parent);

  QVBoxLayout* hbox = new QVBoxLayout(widget);
  hbox->setContentsMargins(0, pqPropertiesPanel::suggestedVerticalSpacing(), 0, 0);
  hbox->setSpacing(0);

  QLabel* label = new QLabel(QString("<html><b>%1</b></html>").arg(labelText), widget);
  label->setWordWrap(true);
  label->setAlignment(Qt::AlignBottom | Qt::AlignLeft);
  hbox->addWidget(label);

  hbox->addWidget(newHLine(parent));
  return widget;
}

//*****************************************************************************
class pqProxyWidget::pqInternals
{
public:
  vtkSmartPointer<vtkSMProxy> Proxy;
  QList<QPointer<pqProxyWidgetItem> > Items;
  bool CachedShowAdvanced;
  QString CachedFilterText;
  vtkStringList* Properties;
  QPointer<QLabel> ProxyDocumentationLabel; // used when showProxyDocumentationInPanel is true.
  pqTimer RequestUpdatePanel;

  pqInternals(vtkSMProxy* smproxy, QStringList properties)
    : Proxy(smproxy)
    , CachedShowAdvanced(false)
  {
    vtkNew<vtkSMPropertyIterator> propertyIter;
    this->Properties = vtkStringList::New();
    propertyIter->SetProxy(smproxy);

    for (propertyIter->Begin(); !propertyIter->IsAtEnd(); propertyIter->Next())
    {
      QString propertyKeyName = propertyIter->GetKey();
      propertyKeyName.replace(" ", "");
      if (properties.contains(propertyKeyName))
      {
        this->Properties->AddString(propertyKeyName.toStdString().c_str());
      }
    }
    if (this->Properties->GetLength() == 0)
    {
      this->Properties->Delete();
      this->Properties = NULL;
    }
  }

  ~pqInternals()
  {
    foreach (pqProxyWidgetItem* item, this->Items)
    {
      delete item;
    }
    if (this->Properties)
    {
      this->Properties->Delete();
    }
  }

  /// Use this method to add pqProxyWidgetItem to the Items instance. Ensures
  /// that signals/slots are setup correctly.
  void appendToItems(pqProxyWidgetItem* item, pqProxyWidget* self)
  {
    this->Items.append(item);

    // Add widget to the layout.
    QGridLayout* gridLayout = qobject_cast<QGridLayout*>(self->layout());
    assert(gridLayout);
    item->appendToLayout(gridLayout, self->useDocumentationForLabels());

    foreach (pqPropertyWidgetDecorator* decorator, item->propertyWidget()->decorators())
    {
      this->RequestUpdatePanel.connect(decorator, SIGNAL(visibilityChanged()), SLOT(start()));
      this->RequestUpdatePanel.connect(decorator, SIGNAL(enableStateChanged()), SLOT(start()));
    }
  }
};

//*****************************************************************************
pqProxyWidget::pqProxyWidget(vtkSMProxy* smproxy, QWidget* parentObject, Qt::WindowFlags wflags)
  : Superclass(parentObject, wflags)
{
  this->constructor(smproxy, QStringList(), parentObject, wflags);
}

//-----------------------------------------------------------------------------
pqProxyWidget::pqProxyWidget(
  vtkSMProxy* smproxy, const QStringList& properties, QWidget* parentObject, Qt::WindowFlags wflags)
  : Superclass(parentObject, wflags)
{
  this->constructor(smproxy, properties, parentObject, wflags);
  this->updatePanel();
}

//-----------------------------------------------------------------------------
void pqProxyWidget::constructor(
  vtkSMProxy* smproxy, const QStringList& properties, QWidget* parentObject, Qt::WindowFlags wflags)
{
  assert(smproxy);
  (void)parentObject;
  (void)wflags;

  this->ApplyChangesImmediately = false;
  // if the proxy wants a more descriptive layout for the panel, use it.
  this->UseDocumentationForLabels = pqProxyWidget::useDocumentationForLabels(smproxy);
  this->Internals = new pqProxyWidget::pqInternals(smproxy, properties);
  this->Internals->ProxyDocumentationLabel = new QLabel(this);
  this->Internals->ProxyDocumentationLabel->hide();
  this->Internals->ProxyDocumentationLabel->setWordWrap(true);
  this->Internals->RequestUpdatePanel.setInterval(0);
  this->Internals->RequestUpdatePanel.setSingleShot(true);
  this->connect(&this->Internals->RequestUpdatePanel, SIGNAL(timeout()), SLOT(updatePanel()));

  QGridLayout* gridLayout = new QGridLayout(this);
  gridLayout->setMargin(pqPropertiesPanel::suggestedMargin());
  gridLayout->setHorizontalSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());
  gridLayout->setVerticalSpacing(pqPropertiesPanel::suggestedVerticalSpacing());

  // Update stretch factors.
  if (this->UseDocumentationForLabels)
  {
    // nothing to do, QGridLayout will just have 1 column.
  }
  else
  {
    gridLayout->setColumnStretch(0, 0);
    gridLayout->setColumnStretch(1, 1);
  }

  this->createWidgets(properties);

  this->setApplyChangesImmediately(false);
  this->hideEvent(NULL);

  // In collaboration setup any pqProxyWidget should be disable
  // when the user lose its MASTER role. And enable back when
  // user became MASTER again.
  // This is achieved by adding a PV_MUST_BE_MASTER property
  // to the current container.
  this->setProperty("PV_MUST_BE_MASTER", QVariant(true));
  this->setEnabled(pqApplicationCore::instance()->getActiveServer()->isMaster());
}

//-----------------------------------------------------------------------------
pqProxyWidget::~pqProxyWidget()
{
  delete this->Internals;
  this->Internals = NULL;
}

//-----------------------------------------------------------------------------
bool pqProxyWidget::useDocumentationForLabels(vtkSMProxy* smproxy)
{
  return (smproxy && smproxy->GetHints() &&
    smproxy->GetHints()->FindNestedElementByName("UseDocumentationForLabels"));
}

namespace
{
const char* vtkGetDocumentation(vtkSMDocumentation* doc, pqProxyWidget::DocumentationType dtype)
{
  if (doc)
  {
    switch (dtype)
    {
      case pqProxyWidget::USE_SHORT_HELP:
        return doc->GetShortHelp();
      case pqProxyWidget::USE_LONG_HELP:
        return doc->GetLongHelp();
      case pqProxyWidget::USE_DESCRIPTION:
        return doc->GetDescription();
      default:
        break;
    }
  }
  return NULL;
}
}

//-----------------------------------------------------------------------------
QString pqProxyWidget::documentationText(vtkSMProperty* smProperty, DocumentationType dtype)
{
  const char* xmlDocumentation =
    smProperty ? vtkGetDocumentation(smProperty->GetDocumentation(), dtype) : NULL;
  if (!xmlDocumentation || xmlDocumentation[0] == 0)
  {
    const char* xmlLabel = smProperty->GetXMLLabel();
    return xmlLabel;
  }
  else
  {
    return pqProxy::rstToHtml(xmlDocumentation).c_str();
  }
}

//-----------------------------------------------------------------------------
QString pqProxyWidget::documentationText(vtkSMProxy* smProxy, DocumentationType dtype)
{
  const char* xmlDocumentation =
    smProxy ? vtkGetDocumentation(smProxy->GetDocumentation(), dtype) : NULL;
  return (!xmlDocumentation || xmlDocumentation[0] == 0)
    ? QString()
    : pqProxy::rstToHtml(xmlDocumentation).c_str();
}

//-----------------------------------------------------------------------------
pqProxyWidget::DocumentationType pqProxyWidget::showProxyDocumentationInPanel(vtkSMProxy* smproxy)
{
  vtkPVXMLElement* xml = (smproxy && smproxy->GetHints())
    ? smproxy->GetHints()->FindNestedElementByName("ShowProxyDocumentationInPanel")
    : NULL;
  if (xml)
  {
    const QString type = QString(xml->GetAttributeOrDefault("type", "description")).toLower();
    if (type == "long_help")
    {
      return USE_LONG_HELP;
    }
    else if (type == "short_help")
    {
      return USE_SHORT_HELP;
    }
    return USE_DESCRIPTION;
  }
  return NONE;
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqProxyWidget::proxy() const
{
  return this->Internals->Proxy;
}

//-----------------------------------------------------------------------------
void pqProxyWidget::showEvent(QShowEvent* sevent)
{
  this->Superclass::showEvent(sevent);
  if (sevent == NULL || !sevent->spontaneous())
  {
    foreach (const pqProxyWidgetItem* item, this->Internals->Items)
    {
      item->select();
    }
  }
}

//-----------------------------------------------------------------------------
void pqProxyWidget::hideEvent(QHideEvent* hevent)
{
  if (hevent == NULL || !hevent->spontaneous())
  {
    foreach (const pqProxyWidgetItem* item, this->Internals->Items)
    {
      item->deselect();
    }
  }
  this->Superclass::hideEvent(hevent);
}

//-----------------------------------------------------------------------------
void pqProxyWidget::apply() const
{
  SM_SCOPED_TRACE(PropertiesModified).arg("proxy", this->proxy());
  foreach (const pqProxyWidgetItem* item, this->Internals->Items)
  {
    item->apply();
  }
}

//-----------------------------------------------------------------------------
void pqProxyWidget::reset() const
{
  foreach (const pqProxyWidgetItem* item, this->Internals->Items)
  {
    item->reset();
  }
}

//-----------------------------------------------------------------------------
void pqProxyWidget::setView(pqView* view)
{
  foreach (const pqProxyWidgetItem* item, this->Internals->Items)
  {
    item->propertyWidget()->setView(view);
  }
}

//-----------------------------------------------------------------------------
void pqProxyWidget::setApplyChangesImmediately(bool immediate_apply)
{
  this->ApplyChangesImmediately = immediate_apply;
}

//---------------------------------------------------------------------------
void pqProxyWidget::createWidgets(const QStringList& properties)
{
  vtkSMProxy* smproxy = this->proxy();

  vtkVLogScopeF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "creating widgets for `%s`",
    smproxy->GetLogNameOrDefault());

  QGridLayout* gridLayout = qobject_cast<QGridLayout*>(this->layout());
  assert(gridLayout);

  DocumentationType dtype = this->showProxyDocumentationInPanel(smproxy);
  if (dtype != NONE)
  {
    QString doc = this->documentationText(smproxy, dtype);
    this->Internals->ProxyDocumentationLabel->setText("<p>" + doc + "</p>");
    this->Internals->ProxyDocumentationLabel->setVisible(!doc.isEmpty());
    gridLayout->addWidget(this->Internals->ProxyDocumentationLabel, gridLayout->rowCount(), 0,
      /*row_stretch*/ 1, /*column_stretch*/ -1);
  }
  else
  {
    this->Internals->ProxyDocumentationLabel->hide();
  }

  // Create widgets for properties.
  this->createPropertyWidgets(properties);

  // handle hints to create 3D widgets, if any.
  if (properties.isEmpty())
  {
    this->create3DWidgets();
  }
  foreach (const pqProxyWidgetItem* item, this->Internals->Items)
  {
    QObject::connect(
      item->propertyWidget(), SIGNAL(changeAvailable()), this, SIGNAL(changeAvailable()));
    QObject::connect(
      item->propertyWidget(), SIGNAL(changeFinished()), this, SLOT(onChangeFinished()));
    QObject::connect(
      item->propertyWidget(), SIGNAL(restartRequired()), this, SIGNAL(restartRequired()));
  }
}

//-----------------------------------------------------------------------------
void pqProxyWidget::createPropertyWidgets(const QStringList& properties)
{
  vtkSMProxy* smproxy = this->proxy();

  // step 1: iterate over all groups to populate `property_2_group_map`.
  //         this will make it easier to determine if a property belong to a
  //         group.
  std::map<vtkSMProperty*, vtkSMPropertyGroup*> property_2_group_map;
  for (size_t index = 0, num_groups = smproxy->GetNumberOfPropertyGroups(); index < num_groups;
       ++index)
  {
    auto smgroup = smproxy->GetPropertyGroup(index);
    for (unsigned int cc = 0, num_properties = smgroup->GetNumberOfProperties();
         cc < num_properties; ++cc)
    {
      if (auto smproperty = smgroup->GetProperty(cc))
      {
        property_2_group_map[smproperty] = smgroup;
      }
    }
  }

  // step 2: iterate over all properties and build an ordered list of properties
  //         that corresponds to the order in which the widgets will be rendered.
  //         this is generally same as the order in the XML with one exception,
  //         properties in groups with same label are placed next to each other.
  std::list<std::pair<vtkSMProperty*, std::string> > ordered_properties;
  std::map<std::string, decltype(ordered_properties)::iterator> group_end_iterators;

  vtkNew<vtkSMOrderedPropertyIterator> opiter;
  opiter->SetProxy(smproxy);
  for (opiter->Begin(); !opiter->IsAtEnd(); opiter->Next())
  {
    auto smproperty = opiter->GetProperty();
    vtkSMPropertyGroup* smgroup = nullptr;
    try
    {
      smgroup = property_2_group_map.at(smproperty);
    }
    catch (std::out_of_range&)
    {
      ordered_properties.push_back(std::make_pair(smproperty, std::string(opiter->GetKey())));
      continue;
    }

    assert(smgroup != nullptr);
    const std::string xmllabel = ::get_group_label(smgroup);
    auto geiter = group_end_iterators.find(xmllabel);
    auto insert_pos =
      (geiter != group_end_iterators.end()) ? std::next(geiter->second) : ordered_properties.end();

    group_end_iterators[xmllabel] = ordered_properties.insert(
      insert_pos, std::make_pair(smproperty, std::string(opiter->GetKey())));
  }

  // Handle legacy-hidden properties: previously, developers hid properties from
  // panels by adding XML in the hints section. We need to ensure that that
  // works too. So, we'll scan the hints and create a list of properties to
  // hide.
  QSet<QString> legacyHiddenProperties;
  DetermineLegacyHiddenProperties(legacyHiddenProperties, smproxy);

  enum class EnumState
  {
    None = 0,  //< undefined
    Custom,    //< group is using a custom widget
    Collection //< group is simply grouping multiple property widgets together
  };
  std::map<vtkSMPropertyGroup*, EnumState> group_widget_status;

  // step 3: now iterate over the `ordered_properties` list and create widgets
  // as needed.
  pqInterfaceTracker* interfaceTracker = pqApplicationCore::instance()->interfaceTracker();
  const QList<pqPropertyWidgetInterface*> interfaces =
    interfaceTracker->interfaces<pqPropertyWidgetInterface*>();

  for (auto& apair : ordered_properties)
  {
    auto smproperty = apair.first;
    const std::string& smkey = apair.second;
    vtkVLogScopeF(
      PARAVIEW_LOG_APPLICATION_VERBOSITY(), "create property widget for  `%s`", smkey.c_str());
    if (smproperty == nullptr ||
      ::skip_property(smproperty, smkey, properties, legacyHiddenProperties))
    {
      continue;
    }
    vtkSMPropertyGroup* smgroup = nullptr;
    try
    {
      smgroup = property_2_group_map.at(smproperty);
    }
    catch (std::out_of_range&)
    {
    }

    vtkVLogIfF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), smgroup != nullptr,
      "part of property-group with label `%s`",
      (smgroup->GetXMLLabel() ? smgroup->GetXMLLabel() : "(unspecified)"));
    if (smgroup != nullptr && ::skip_group(smgroup, properties))
    {
      if (properties.size() > 0)
      {
        // We're encountering a weird case. The user explicitly listed the
        // properties to create widgets for, however, only one (or some) properties from
        // the group were requested to be shown, not the entire group.
        // There's no right way to handle this. We handle it the "legacy" way
        // i.e. just create the widget for this property as if it was not part
        // of the group at all.
        smgroup = nullptr;
        vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(),
          "treat as a non-group property since explicitly selected in isolation.");
      }
      else
      {
        continue;
      }
    }

    if (smgroup != nullptr)
    {
      auto gwsiter = group_widget_status.find(smgroup);
      if (gwsiter != group_widget_status.end() && gwsiter->second == EnumState::Custom)
      {
        // already created a custom widget for this group, skip
        // the property.
        vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(),
          "skip since already handled in custom group widget");
        continue;
      }

      if (gwsiter == group_widget_status.end())
      {
        // first time we're encountering a property from this group.
        // let's see if we're creating a custom group widget or just a
        // multi-property group.
        auto& ref_state = group_widget_status[smgroup];
        ref_state = EnumState::None;
        for (auto iface : interfaces)
        {
          if (auto gwidget = iface->createWidgetForPropertyGroup(smproxy, smgroup, this))
          {
            vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "created group widget `%s`",
              gwidget->metaObject()->className());

            // handle group decorators for custom widget, if any.
            ::add_decorators(gwidget, smgroup->GetHints());

            gwidget->setParent(this);
            gwidget->setObjectName(QString(smgroup->GetPanelWidget()).remove(' '));

            auto item =
              pqProxyWidgetItem::newGroupItem(gwidget, QString(smgroup->GetXMLLabel()), this);
            item->Advanced = (smgroup->GetPanelVisibility() &&
              strcmp(smgroup->GetPanelVisibility(), "advanced") == 0);
            item->SearchTags << smgroup->GetPanelWidget();
            if (smgroup->GetXMLLabel())
            {
              item->SearchTags << smgroup->GetXMLLabel();
            }
            // FIXME: Maybe SearchTags should have the labels for all the properties
            // in this group.

            this->Internals->appendToItems(item, this);
            group_widget_status[smgroup] = EnumState::Custom;
            break;
          }
        } // for ()

        if (ref_state == EnumState::Custom)
        {
          // we just add a custom widget for this property's group,
          // continue on to the next property.
          continue;
        }

        // no custom widget created for the group, must be simply a
        // multi-property group. just update the state and fall-through
        // to create a widget for the property.
        ref_state = EnumState::Collection;
      }
    }

    assert(smgroup == nullptr || group_widget_status[smgroup] == EnumState::Collection);

    const bool isCompoundProxy = vtkSMCompoundSourceProxy::SafeDownCast(smproxy) != nullptr;
    const char* xmllabel =
      (smproperty->GetXMLLabel() && !isCompoundProxy) ? smproperty->GetXMLLabel() : smkey.c_str();

    const QString xmlDocumentation = pqProxyWidget::documentationText(smproperty);

    // create property widget
    pqPropertyWidget* pwidget = this->createWidgetForProperty(smproperty, smproxy, this);
    if (!pwidget)
    {
      vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "skip since failed to determine widget type.");
      continue;
    }

    pwidget->setObjectName(QString(smkey.c_str()).remove(' '));

    const QString itemLabel = this->UseDocumentationForLabels
      ? QString("<p><b>%1</b>: %2</p>").arg(xmllabel).arg(xmlDocumentation)
      : QString(xmllabel);

    auto item = (smgroup == nullptr) ? pqProxyWidgetItem::newItem(pwidget, QString(itemLabel), this)
                                     : pqProxyWidgetItem::newMultiItemGroupItem(
                                         smgroup->GetXMLLabel(), pwidget, QString(itemLabel), this);

    // save record of the property widget and containing widget
    item->SearchTags << xmllabel << xmlDocumentation << smkey.c_str();
    item->InformationOnly = smproperty->GetInformationOnly();
    item->Advanced =
      smproperty->GetPanelVisibility() && strcmp(smproperty->GetPanelVisibility(), "advanced") == 0;
    if (smproperty->GetPanelVisibilityDefaultForRepresentation())
    {
      item->appendToDefaultVisibilityForRepresentations(
        smproperty->GetPanelVisibilityDefaultForRepresentation());
    }

    // handle group decorator, if any.
    if (smgroup)
    {
      // Create decorators, if any.
      ::add_decorators(pwidget, smgroup->GetHints());

      if (smgroup->GetXMLLabel())
      {
        // see #18498
        item->SearchTags << smgroup->GetXMLLabel();
      }
    }

    this->Internals->appendToItems(item, this);
  }
}

//---------------------------------------------------------------------------
// Creates 3D widgets for the panel.
void pqProxyWidget::create3DWidgets()
{
  vtkSMProxy* smProxy = this->proxy();
  vtkPVXMLElement* hints = smProxy->GetHints();
  if (hints && (hints->FindNestedElementByName("PropertyGroup") != NULL))
  {
    qCritical("Obsolete 3DWidget request encountered in the proxy hints. "
              "Please refer to the 'Major API Changes' guide in ParaView developer documentation.");
  }
}

//-----------------------------------------------------------------------------
pqPropertyWidget* pqProxyWidget::createWidgetForProperty(
  vtkSMProperty* smproperty, vtkSMProxy* smproxy, QWidget* parentObj)
{
  // check for custom widgets
  pqPropertyWidget* widget = NULL;
  pqInterfaceTracker* interfaceTracker = pqApplicationCore::instance()->interfaceTracker();
  QList<pqPropertyWidgetInterface*> interfaces =
    interfaceTracker->interfaces<pqPropertyWidgetInterface*>();
  foreach (pqPropertyWidgetInterface* interface, interfaces)
  {
    widget = interface->createWidgetForProperty(smproxy, smproperty, parentObj);
    if (widget)
    {
      break;
    }
  }

  if (widget != NULL)
  {
  }
  else if (vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(smproperty))
  {
    widget = new pqDoubleVectorPropertyWidget(dvp, smproxy, parentObj);
  }
  else if (vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(smproperty))
  {
    widget = pqIntVectorPropertyWidget::createWidget(ivp, smproxy, parentObj);
  }
  else if (vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(smproperty))
  {
    widget = pqStringVectorPropertyWidget::createWidget(svp, smproxy, parentObj);
  }
  else if (vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(smproperty))
  {
    bool selection_input =
      (pp->GetHints() && pp->GetHints()->FindNestedElementByName("SelectionInput"));

    // find the domain
    vtkSMDomain* domain = 0;
    vtkSMDomainIterator* domainIter = pp->NewDomainIterator();
    for (domainIter->Begin(); !domainIter->IsAtEnd(); domainIter->Next())
    {
      domain = domainIter->GetDomain();
    }
    domainIter->Delete();

    if (selection_input || vtkSMProxyListDomain::SafeDownCast(domain))
    {
      widget = new pqProxyPropertyWidget(pp, smproxy, parentObj);
    }
  }
  else if (smproperty && strcmp(smproperty->GetClassName(), "vtkSMProperty") == 0)
  {
    widget = new pqCommandPropertyWidget(smproperty, smproxy, parentObj);
  }

  if (widget)
  {
    vtkVLogF(
      PARAVIEW_LOG_APPLICATION_VERBOSITY(), "created `%s`", widget->metaObject()->className());
    widget->setProperty(smproperty);

    // Create decorators, if any.
    ::add_decorators(widget, smproperty->GetHints());

    // Create all default decorators
    for (int cc = 0; cc < interfaces.size(); cc++)
    {
      pqPropertyWidgetInterface* interface = interfaces[cc];
      interface->createDefaultWidgetDecorators(widget);
    }
  }

  return widget;
}

//-----------------------------------------------------------------------------
bool pqProxyWidget::filterWidgets(bool show_advanced, const QString& filterText)
{
  this->Internals->CachedShowAdvanced = show_advanced;
  this->Internals->CachedFilterText = filterText;

  if (!filterText.isEmpty())
  {
    show_advanced = true;
  }

  // disable updates to avoid flicker
  // only if needed to avoid bug
  // with setUpdatesEnabled
  // https://bugreports.qt.io/browse/QTBUG-8459
  bool prevUE = this->updatesEnabled();
  if (prevUE)
  {
    this->setUpdatesEnabled(false);
  }

  const pqProxyWidgetItem* prevItem = NULL;
  vtkSMProxy* smProxy = this->Internals->Proxy;
  foreach (const pqProxyWidgetItem* item, this->Internals->Items)
  {
    bool visible = item->canShowWidget(show_advanced, filterText, smProxy);
    if (visible)
    {
      item->show(prevItem, item->enableWidget(), show_advanced);
      prevItem = item;
    }
    else
    {
      item->hide();
    }
  }
  if (prevUE)
  {
    this->setUpdatesEnabled(prevUE);
  }
  return (prevItem != NULL);
}

//-----------------------------------------------------------------------------
void pqProxyWidget::updatePanel()
{
  this->filterWidgets(this->Internals->CachedShowAdvanced, this->Internals->CachedFilterText);
}

//-----------------------------------------------------------------------------
bool pqProxyWidget::restoreDefaults()
{
  bool anyReset = false;
  vtkSMSettings* settings = vtkSMSettings::GetInstance();
  if (this->Internals->Proxy)
  {
    vtkSmartPointer<vtkSMPropertyIterator> iter;
    iter.TakeReference(this->Internals->Proxy->NewPropertyIterator());
    for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
      vtkSMProperty* smproperty = iter->GetProperty();

      // restore defaults only for properties listed
      if (this->Internals->Properties &&
        this->Internals->Properties->GetIndex(smproperty->GetXMLName()) == -1)
      {
        continue;
      }

      // Restore only basic type properties.
      if (vtkSMVectorProperty::SafeDownCast(smproperty) && !smproperty->GetNoCustomDefault() &&
        !smproperty->GetInformationOnly())
      {
        if (!smproperty->IsValueDefault())
        {
          anyReset = true;
        }
        smproperty->ResetToDefault();

        // Restore to site setting if there is one. If there isn't, this does
        // not change the property setting. NOTE: user settings have priority
        // of VTK_DOUBLE_MAX, so we set the site settings priority to a
        // number just below VTK_DOUBLE_MAX.
        settings->GetPropertySetting(smproperty, nextafter(VTK_DOUBLE_MAX, 0));
      }
    }
  }

  // The code above bypasses the changeAvailable() and
  // changeFinished() signal from the pqProxyWidget, so we check here
  // whether we should act as if changes are available only if any of
  // the properties have been reset.
  if (anyReset)
  {
    emit changeAvailable();
    emit changeFinished();
  }
  return anyReset;
}

//-----------------------------------------------------------------------------
void pqProxyWidget::saveAsDefaults()
{
  vtkSMSettings* settings = vtkSMSettings::GetInstance();
  vtkSMNamedPropertyIterator* propertyIt = NULL;
  if (this->Internals->Properties)
  {
    propertyIt = vtkSMNamedPropertyIterator::New();
    propertyIt->SetPropertyNames(this->Internals->Properties);
  }
  settings->SetProxySettings(this->Internals->Proxy, propertyIt);
  if (propertyIt)
  {
    propertyIt->Delete();
  }
}

//-----------------------------------------------------------------------------
void pqProxyWidget::onChangeFinished()
{
  if (this->ApplyChangesImmediately)
  {
    pqPropertyWidget* pqSender = qobject_cast<pqPropertyWidget*>(this->sender());
    if (pqSender)
    {
      SM_SCOPED_TRACE(PropertiesModified).arg("proxy", this->proxy());
      pqSender->apply();
    }
  }
  emit this->changeFinished();
}
