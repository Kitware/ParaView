// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqProxyWidget.h"

#include "Private/pqProxyWidgetItem.h"
#include "pqApplicationCore.h"
#include "pqCommandPropertyWidget.h"
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
#include "pqShortcutDecorator.h"
#include "pqStringVectorPropertyWidget.h"
#include "pqTimer.h"
#include "vtkCollection.h"
#include "vtkNew.h"
#include "vtkPVGeneralSettings.h"
#include "vtkPVLogger.h"
#include "vtkPVXMLElement.h"
#include "vtkSMCompoundSourceProxy.h"
#include "vtkSMDocumentation.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMNamedPropertyIterator.h"
#include "vtkSMOrderedPropertyIterator.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSettings.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMTrace.h"
#include "vtkSMVectorProperty.h"
#include "vtkSmartPointer.h"
#include "vtkStringList.h"
#include "vtkWeakPointer.h"

#include <QCoreApplication>
#include <QHideEvent>
#include <QLabel>
#include <QMenu>
#include <QPointer>
#include <QShowEvent>
#include <QVBoxLayout>

#include <cassert>
#include <cmath>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <utility>
#include <vector>

//-----------------------------------------------------------------------------------
namespace
{
std::vector<vtkWeakPointer<vtkPVXMLElement>> get_decorators(vtkPVXMLElement* hints)
{
  std::vector<vtkWeakPointer<vtkPVXMLElement>> decoratorTypes;
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

//-----------------------------------------------------------------------------------
void add_decorators(pqPropertyWidget* widget, vtkPVXMLElement* hints)
{
  if (widget && hints)
  {
    auto xmls = get_decorators(hints);
    for (const auto& xml : xmls)
    {
      assert(xml && xml->GetAttribute("type"));
      pqPropertyWidgetDecorator::create(xml, widget);
    }
  }
}

//-----------------------------------------------------------------------------------
std::string get_group_label(vtkSMPropertyGroup* smgroup)
{
  assert(smgroup != nullptr);
  char* label = smgroup->GetXMLLabel();
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

//-----------------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------------
bool skip(const char* key, const char* visibility, const QStringList& chosenProperties,
  const QSet<QString>& legacyHiddenProperties, const pqProxyWidget* self)
{
  const QString skey = QString(key);
  const QString simplifiedKey = QString(key).remove(' ');

  if (chosenProperties.contains(simplifiedKey) || chosenProperties.contains(skey))
  {
    // property has been explicitly chosen.
    return false;
  }
  else if (!chosenProperties.isEmpty())
  {
    vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "skip since not in chosen properties set.");
    return true;
  }

  Q_ASSERT(chosenProperties.isEmpty());

  if (legacyHiddenProperties.contains(skey) || legacyHiddenProperties.contains(simplifiedKey))
  {
    // skipping properties marked with "show=0" in the hints section.
    vtkVLogF(
      PARAVIEW_LOG_APPLICATION_VERBOSITY(), "skip since legacy hint with `show=0` specified.");
    return true;
  }

  if (visibility == nullptr)
  {
    // unclear what to do here, let's go with skipping the property since
    // typically, we have a non-null string.
    vtkVLogF(
      PARAVIEW_LOG_APPLICATION_VERBOSITY(), "skip since no panel visibility flag specified.");
    return true;
  }

  if (strcmp(visibility, "never") == 0)
  {
    vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(),
      "skip since marked as never show (unless it was explicitly 'chosen').");
  }

  if (!self->defaultVisibilityLabels().contains(visibility) &&
    !self->advancedVisibilityLabels().contains(visibility))
  {
    vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(),
      "skip since not in default/advanced visibility label sets.");
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------------
// Returns true if the property should be skipped from the panel.
bool skip_property(vtkSMProperty* smproperty, const std::string& key,
  const QStringList& chosenProperties, const QSet<QString>& legacyHiddenProperties,
  const pqProxyWidget* self)
{
  if (smproperty->GetIsInternal())
  {
    // skip internal properties
    vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "skip since internal.");
    return true;
  }

  return skip(
    key.c_str(), smproperty->GetPanelVisibility(), chosenProperties, legacyHiddenProperties, self);
}

//-----------------------------------------------------------------------------------
// Returns true if the group should be skipped from the panel.
bool skip_group(
  vtkSMPropertyGroup* smgroup, const QStringList& chosenProperties, const pqProxyWidget* self)
{
  return skip(smgroup->GetXMLLabel(), smgroup->GetPanelVisibility(), chosenProperties, {}, self);
}

// return true if this property widget can save default values to settings,
// and have its default value restored.
bool canSaveDefault(vtkSMProperty* smproperty)
{
  return (smproperty && !vtkSMInputProperty::SafeDownCast(smproperty) &&
    !smproperty->GetNoCustomDefault() && !smproperty->GetInformationOnly());
}

// given a propertyWidget, see if it has a vtkSMProxyListDomain, and
// return the chosen proxy widget if it does.
pqProxyPropertyWidget* getChosenProxyFromDomain(pqPropertyWidget* propertyWidget)
{
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(propertyWidget->property());
  if (pp)
  {
    // find the domain
    vtkSMDomain* domain = nullptr;
    vtkSMDomainIterator* domainIter = pp->NewDomainIterator();
    for (domainIter->Begin(); !domainIter->IsAtEnd(); domainIter->Next())
    {
      domain = domainIter->GetDomain();
    }
    domainIter->Delete();

    auto* proxyPropWidget = qobject_cast<pqProxyPropertyWidget*>(propertyWidget);
    if (proxyPropWidget && vtkSMProxyListDomain::SafeDownCast(domain) &&
      proxyPropWidget->chosenProxy())
    {
      return proxyPropWidget;
    }
  }
  return nullptr;
}

// ProxyProperties might have a domain that selects another proxy -
// we want the search tags from that proxy too, for this widget.
void addProxyTags(QStringList& SearchTags, pqPropertyWidget* propertyWidget)
{
  auto* proxyPropWidget = getChosenProxyFromDomain(propertyWidget);
  if (proxyPropWidget)
  {
    auto* chosenProxy = proxyPropWidget->chosenProxy();
    // add the property tags for the chosen proxy
    vtkSmartPointer<vtkSMPropertyIterator> iter;
    iter.TakeReference(chosenProxy->NewPropertyIterator());
    for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
      vtkSMProperty* chosenProxyProperty = iter->GetProperty();
      if (chosenProxyProperty->GetXMLLabel())
      {
        SearchTags << chosenProxyProperty->GetXMLLabel();
      }

      const QString xmlDocumentation = pqProxyWidget::documentationText(chosenProxyProperty);
      if (!xmlDocumentation.isEmpty())
      {
        SearchTags << xmlDocumentation;
      }
    }
  }
}

// if this visible propertyWidget has shortcuts or child widget(s) with shortcuts,
// enable the first shortcut decorator found, returning true if shortcuts were enabled.
bool enableShortcutDecorator(pqPropertyWidget* propertyWidget, bool changeFocus)
{
  if (propertyWidget->isVisible())
  {
    for (pqPropertyWidgetDecorator* decorator : propertyWidget->decorators())
    {
      auto* shortcutDecorator = qobject_cast<pqShortcutDecorator*>(decorator);
      if (shortcutDecorator)
      {
        shortcutDecorator->setEnabled(true, changeFocus);
        return true;
      }
    }
    // Check to see if property has a domain that selects another proxy
    auto* proxyPropWidget = getChosenProxyFromDomain(propertyWidget);
    if (proxyPropWidget)
    {
      auto* chosenProxy = proxyPropWidget->chosenProxy();
      // find the widget corresponding to this proxy
      pqPropertyWidget* chosenProxyWidget = nullptr;
      QList<pqPropertyWidget*> allPropChildren = proxyPropWidget->findChildren<pqPropertyWidget*>();
      for (auto* pWidget : allPropChildren)
      {
        if (pWidget->proxy() == chosenProxy)
        {
          chosenProxyWidget = pWidget;
          break;
        }
      }
      // recurse, check decorators.
      if (chosenProxyWidget && enableShortcutDecorator(chosenProxyWidget, changeFocus))
      {
        return true;
      }
    }
  }
  return false;
}

} // end of namespace {}

//-----------------------------------------------------------------------------------
// static
QWidget* pqProxyWidget::newGroupLabelWidget(
  const QString& labelText, QWidget* parent, const QList<QWidget*>& buttons)
{
  QWidget* widget = new QWidget(parent);

  QVBoxLayout* vbox = new QVBoxLayout(widget);
  vbox->setContentsMargins(0, pqPropertiesPanel::suggestedVerticalSpacing(), 0, 0);
  vbox->setSpacing(0);

  QLabel* label = new QLabel(QString("<html><b>%1</b></html>").arg(labelText), widget);
  label->setWordWrap(true);
  label->setAlignment(Qt::AlignBottom | Qt::AlignLeft);

  if (!buttons.empty())
  {
    auto hbox = new QHBoxLayout();
    hbox->setContentsMargins(0, 0, 0, 0);
    hbox->setSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());
    hbox->addWidget(label);
    for (auto& button : buttons)
    {
      hbox->addWidget(button);
    }

    vbox->addLayout(hbox);
  }
  else
  {
    vbox->addWidget(label);
  }
  vbox->addWidget(pqProxyWidgetItem::newHLine(parent));
  return widget;
}

//*****************************************************************************
class pqProxyWidget::pqInternals
{
public:
  vtkSmartPointer<vtkSMProxy> Proxy;
  QList<QPointer<pqProxyWidgetItem>> Items;
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
      this->Properties = nullptr;
    }
  }

  ~pqInternals()
  {
    for (pqProxyWidgetItem* item : this->Items)
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

    for (pqPropertyWidgetDecorator* decorator : item->propertyWidget()->decorators())
    {
      this->RequestUpdatePanel.connect(decorator, SIGNAL(visibilityChanged()), SLOT(start()));
      this->RequestUpdatePanel.connect(decorator, SIGNAL(enableStateChanged()), SLOT(start()));
    }
  }
};

//*****************************************************************************
pqProxyWidget::pqProxyWidget(vtkSMProxy* proxy)
  : pqProxyWidget(proxy, QStringList{}, { "default" }, { "advanced" }, /*showHeadersFooters*/ true,
      /*parent*/ nullptr, Qt::WindowFlags{})
{
}

//-----------------------------------------------------------------------------
pqProxyWidget::pqProxyWidget(vtkSMProxy* proxy, QWidget* parent, Qt::WindowFlags flags)
  : pqProxyWidget(proxy, QStringList{}, { "default" }, { "advanced" }, /*showHeadersFooters*/ true,
      /*parent*/ parent, flags)
{
}

//-----------------------------------------------------------------------------
pqProxyWidget::pqProxyWidget(vtkSMProxy* proxy, const QStringList& properties,
  bool showHeadersFooters /* = true*/, QWidget* parent /* = nullptr*/,
  Qt::WindowFlags flags /* = Qt::WindowFlags{}*/)
  : pqProxyWidget(
      proxy, properties, { "default" }, { "advanced" }, showHeadersFooters, parent, flags)
{
}

//-----------------------------------------------------------------------------
pqProxyWidget::pqProxyWidget(vtkSMProxy* proxy, std::initializer_list<QString> defaultLabels,
  std::initializer_list<QString> advancedLabels, QWidget* parent /* = nullptr*/,
  Qt::WindowFlags flags /* = Qt::WindowFlags{}*/)
  : pqProxyWidget(proxy, QStringList{}, defaultLabels, advancedLabels, /*showHeadersFooters*/ true,
      parent, flags)
{
}

//-----------------------------------------------------------------------------
pqProxyWidget::pqProxyWidget(vtkSMProxy* smproxy, const QStringList& properties,
  std::initializer_list<QString> defaultLabels, std::initializer_list<QString> advancedLabels,
  bool showHeadersFooters /* = true*/, QWidget* parent /* = nullptr*/,
  Qt::WindowFlags flags /* = Qt::WindowFlags{}*/)
  : Superclass(parent, flags)
  , DefaultVisibilityLabels{ defaultLabels }
  , AdvancedVisibilityLabels{ advancedLabels }
  , ApplyChangesImmediately(false)
  , UseDocumentationForLabels(pqProxyWidget::useDocumentationForLabels(smproxy))
  , ShowHeadersFooters{ showHeadersFooters }
  , Internals(new pqProxyWidget::pqInternals(smproxy, properties))
{
  Q_ASSERT(smproxy != nullptr);

  auto& internals = (*this->Internals);
  internals.ProxyDocumentationLabel = new QLabel(this);
  internals.ProxyDocumentationLabel->hide();
  internals.ProxyDocumentationLabel->setWordWrap(true);
  internals.RequestUpdatePanel.setInterval(0);
  internals.RequestUpdatePanel.setSingleShot(true);
  this->connect(&internals.RequestUpdatePanel, SIGNAL(timeout()), SLOT(updatePanel()));

  QGridLayout* gridLayout = new QGridLayout(this);
  gridLayout->setContentsMargins(pqPropertiesPanel::suggestedMargin(),
    pqPropertiesPanel::suggestedMargin(), pqPropertiesPanel::suggestedMargin(),
    pqPropertiesPanel::suggestedMargin());
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
  this->hideEvent(nullptr);

  // In collaboration setup any pqProxyWidget should be disable
  // when the user lose its MASTER role. And enable back when
  // user became MASTER again.
  // This is achieved by adding a PV_MUST_BE_MASTER property
  // to the current container.
  this->setProperty("PV_MUST_BE_MASTER", QVariant(true));
  this->setEnabled(pqApplicationCore::instance()->getActiveServer()->isMaster());

  // This is here to keep behaviour consistent with earlier versions of the
  // code. If an explicit lists of properties is provided, we update the panel
  // in the constructor itself.
  if (!properties.empty())
  {
    this->updatePanel();
  }
}

//-----------------------------------------------------------------------------
pqProxyWidget::~pqProxyWidget()
{
  delete this->Internals;
  this->Internals = nullptr;
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
  return nullptr;
}
}

//-----------------------------------------------------------------------------
QString pqProxyWidget::documentationText(vtkSMProperty* smProperty, DocumentationType dtype)
{
  const char* xmlDocumentation =
    smProperty ? vtkGetDocumentation(smProperty->GetDocumentation(), dtype) : nullptr;
  if (!xmlDocumentation || xmlDocumentation[0] == 0)
  {
    return QCoreApplication::translate("ServerManagerXML", smProperty->GetXMLLabel());
  }
  else
  {
    return pqProxy::rstToHtml(QCoreApplication::translate("ServerManagerXML", xmlDocumentation));
  }
}

//-----------------------------------------------------------------------------
QString pqProxyWidget::documentationText(vtkSMProxy* smProxy, DocumentationType dtype)
{
  const char* xmlDocumentation =
    smProxy ? vtkGetDocumentation(smProxy->GetDocumentation(), dtype) : nullptr;
  return (!xmlDocumentation || xmlDocumentation[0] == 0)
    ? QString()
    : pqProxy::rstToHtml(QCoreApplication::translate("ServerManagerXML", xmlDocumentation));
}

//-----------------------------------------------------------------------------
pqProxyWidget::DocumentationType pqProxyWidget::showProxyDocumentationInPanel(vtkSMProxy* smproxy)
{
  vtkPVXMLElement* xml = (smproxy && smproxy->GetHints())
    ? smproxy->GetHints()->FindNestedElementByName("ShowProxyDocumentationInPanel")
    : nullptr;
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
  if (sevent == nullptr || !sevent->spontaneous())
  {
    for (const pqProxyWidgetItem* item : this->Internals->Items)
    {
      item->select();
    }
  }
}

//-----------------------------------------------------------------------------
void pqProxyWidget::hideEvent(QHideEvent* hevent)
{
  if (hevent == nullptr || !hevent->spontaneous())
  {
    for (const pqProxyWidgetItem* item : this->Internals->Items)
    {
      item->deselect();
    }
  }
  this->Superclass::hideEvent(hevent);
}

//-----------------------------------------------------------------------------
void pqProxyWidget::applyInternal() const
{
  for (const pqProxyWidgetItem* item : this->Internals->Items)
  {
    item->apply();
  }

  if (this->shouldPreservePropertyValues())
  {
    this->saveAsDefaults();
  }
}

//-----------------------------------------------------------------------------
void pqProxyWidget::apply() const
{
  SM_SCOPED_TRACE(PropertiesModified).arg("proxy", this->proxy());
  this->applyInternal();
}

//-----------------------------------------------------------------------------
void pqProxyWidget::reset() const
{
  for (const pqProxyWidgetItem* item : this->Internals->Items)
  {
    item->reset();
  }
}

//-----------------------------------------------------------------------------
void pqProxyWidget::setView(pqView* view)
{
  bool done = false;
  for (const pqProxyWidgetItem* item : this->Internals->Items)
  {
    item->propertyWidget()->setView(view);
    // make sure the first widget shown has active keyboard shortcuts.
    if (!done)
    {
      if (enableShortcutDecorator(item->propertyWidget(), false))
      {
        done = true;
      }
    }
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
  for (const pqProxyWidgetItem* item : this->Internals->Items)
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
  std::list<std::pair<vtkSMProperty*, std::string>> ordered_properties;
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

  // group-widget name uniquification helper.
  std::map<QString, int> group_widget_names;

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
      ::skip_property(smproperty, smkey, properties, legacyHiddenProperties, this))
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
    if (smgroup != nullptr && ::skip_group(smgroup, properties, this))
    {
      if (!properties.empty())
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
            // we use group_widget_names to uniquify the names for group widgets
            // when there are multiple groups of the same type (BUG #20271).
            auto wdgName = QString(smgroup->GetPanelWidget()).remove(' ');
            const auto suffixCount = group_widget_names[wdgName]++;
            if (suffixCount != 0)
            {
              wdgName += QString::number(suffixCount);
            }
            gwidget->setObjectName(wdgName);

            bool advanced = (smgroup->GetPanelVisibility() &&
              strcmp(smgroup->GetPanelVisibility(), "advanced") == 0);

            QStringList searchTags;
            searchTags << smgroup->GetPanelWidget();
            if (smgroup->GetXMLLabel())
            {
              searchTags << QCoreApplication::translate("ServerManagerXML", smgroup->GetXMLLabel());
            }
            // SearchTags should have the labels for all the properties
            // in this group.
            for (unsigned int i = 0; i < smgroup->GetNumberOfProperties(); ++i)
            {
              auto* groupProp = smgroup->GetProperty(i);
              if (groupProp->GetXMLLabel())
              {
                searchTags << QCoreApplication::translate(
                  "ServerManagerXML", groupProp->GetXMLLabel());
              }
            }

            auto item = pqProxyWidgetItem::newGroupItem(gwidget,
              QCoreApplication::translate("ServerManagerXML", smgroup->GetXMLLabel()),
              this->ShowHeadersFooters, advanced, searchTags, this);

            this->Internals->appendToItems(item, this);
            ref_state = EnumState::Custom;
            break;
          }
        }

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
      vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "skip since could not determine widget type.");
      continue;
    }

    pwidget->setObjectName(QString(smkey.c_str()).remove(' '));

    const QString itemLabel = this->UseDocumentationForLabels
      ? QString("<p><b>%1</b>: %2</p>")
          .arg(QCoreApplication::translate("ServerManagerXML", xmllabel))
          .arg(xmlDocumentation)
      : QCoreApplication::translate("ServerManagerXML", xmllabel);

    // save record of the property widget and containing widget
    QStringList searchTags;
    searchTags << xmllabel << xmlDocumentation << smkey.c_str();
    addProxyTags(searchTags, pwidget);

    bool informationOnly = smproperty->GetInformationOnly();

    bool advanced =
      smproperty->GetPanelVisibility() && strcmp(smproperty->GetPanelVisibility(), "advanced") == 0;

    // handle group decorator, if any.
    if (smgroup)
    {
      // Create decorators, if any.
      ::add_decorators(pwidget, smgroup->GetHints());
      if (smgroup->GetXMLLabel())
      {
        // see #18498
        searchTags << QCoreApplication::translate("ServerManagerXML", smgroup->GetXMLLabel());
      }
    }

    auto item = (smgroup == nullptr)
      ? pqProxyWidgetItem::newItem(
          pwidget, QString(itemLabel), advanced, informationOnly, searchTags, this)
      : pqProxyWidgetItem::newMultiItemGroupItem(
          QCoreApplication::translate("ServerManagerXML", smgroup->GetXMLLabel()), pwidget,
          QString(itemLabel), this->ShowHeadersFooters, advanced, informationOnly, searchTags,
          this);

    if (smproperty->GetPanelVisibilityDefaultForRepresentation())
    {
      item->appendToDefaultVisibilityForRepresentations(
        smproperty->GetPanelVisibilityDefaultForRepresentation());
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
  if (hints && (hints->FindNestedElementByName("PropertyGroup") != nullptr))
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
  pqPropertyWidget* widget = nullptr;
  pqInterfaceTracker* interfaceTracker = pqApplicationCore::instance()->interfaceTracker();
  QList<pqPropertyWidgetInterface*> interfaces =
    interfaceTracker->interfaces<pqPropertyWidgetInterface*>();
  for (pqPropertyWidgetInterface* interface : interfaces)
  {
    widget = interface->createWidgetForProperty(smproxy, smproperty, parentObj);
    if (widget)
    {
      break;
    }
  }

  if (widget != nullptr)
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
    vtkSMDomain* domain = nullptr;
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

  const pqProxyWidgetItem* prevItem = nullptr;
  vtkSMProxy* smProxy = this->Internals->Proxy;
  for (const pqProxyWidgetItem* item : this->Internals->Items)
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
  return (prevItem != nullptr);
}

//-----------------------------------------------------------------------------
void pqProxyWidget::showLinkedInteractiveWidget(int portIndex, bool show, bool changeFocus)
{
  bool done = false;
  for (const pqProxyWidgetItem* item : this->Internals->Items)
  {
    if (show)
    {
      item->propertyWidget()->selectPort(portIndex);
      // make sure the first widget shown has active keyboard shortcuts.
      if (!done)
      {
        if (enableShortcutDecorator(item->propertyWidget(), changeFocus))
        {
          done = true;
        }
      }
    }
    else
    {
      item->propertyWidget()->deselect();
    }
  }
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

      // skip input and information only
      if (vtkSMInputProperty::SafeDownCast(smproperty) || smproperty->GetInformationOnly())
      {
        continue;
      }

      anyReset =
        this->resetProperty(smproperty, vtkSMSettings::GetApplicationPriority(), false) || anyReset;
    }
  }

  // The code above bypasses the changeAvailable() and
  // changeFinished() signal from the pqProxyWidget, so we check here
  // whether we should act as if changes are available only if any of
  // the properties have been reset.
  if (anyReset)
  {
    Q_EMIT changeAvailable();
    Q_EMIT changeFinished();
  }
  return anyReset;
}

//-----------------------------------------------------------------------------
void pqProxyWidget::saveAsDefaults() const
{
  vtkSMSettings* settings = vtkSMSettings::GetInstance();
  vtkSMNamedPropertyIterator* propertyIt = nullptr;
  if (this->Internals->Properties)
  {
    propertyIt = vtkSMNamedPropertyIterator::New();
    propertyIt->SetPropertyNames(this->Internals->Properties);
    propertyIt->SetProxy(this->Internals->Proxy);
  }
  settings->SetProxySettings(this->Internals->Proxy, propertyIt, false);
  if (propertyIt)
  {
    propertyIt->Delete();
  }
}

//-----------------------------------------------------------------------------
void pqProxyWidget::showContextMenu(const QPoint& pt, pqPropertyWidget* propWidget)
{
  if (!::canSaveDefault(propWidget->property()))
  {
    return;
  }
  QMenu menu(this);
  menu.setObjectName("PropertyContextMenu");
  auto* useDefault = menu.addAction(tr("Use as Default"), propWidget,
    [this, propWidget]()
    {
      // get the property name
      std::string name = this->Internals->Proxy->GetPropertyName(propWidget->property());
      // tell settings to save a single property by name
      std::vector<std::string> names{ name };
      vtkNew<vtkSMNamedPropertyIterator> propertyIt;
      propertyIt->SetProxy(this->Internals->Proxy);
      propertyIt->SetPropertyNames(names);
      vtkSMSettings* settings = vtkSMSettings::GetInstance();
      settings->SetProxySettings(this->Internals->Proxy, propertyIt, false);
    });
  useDefault->setObjectName("UseDefault");

  auto pqproxy = pqProxy::findProxy(this->proxy());
  if (pqproxy)
  {
    useDefault->setEnabled(pqproxy->modifiedState() == pqProxy::UNMODIFIED);
  }

  QAction* resetToAppDefault = menu.addAction(tr("Reset to Application Default"), propWidget,
    [this, propWidget]() { this->resetPropertyToAppDefault(propWidget); });
  resetToAppDefault->setObjectName("ResetToDefault");

  QAction* resetToSettings = menu.addAction(tr("Reset to User Settings"), propWidget,
    [this, propWidget]() { this->resetPropertyToUserDefault(propWidget); });
  resetToSettings->setObjectName("ResetToSettings");

  menu.exec(pt);
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

      if (this->shouldPreservePropertyValues())
      {
        this->saveAsDefaults();
      }
    }
  }
  Q_EMIT this->changeFinished();
}

//-----------------------------------------------------------------------------
bool pqProxyWidget::shouldPreservePropertyValues() const
{
  vtkSMProxy* proxy = this->proxy();
  std::set<std::string> possibleGroups = { "filters", "sources", "extractors" };
  bool preserveProperties = vtkPVGeneralSettings::GetInstance()->GetPreservePropertyValues();

  return preserveProperties && (possibleGroups.count(proxy->GetXMLGroup()) == 1);
}

//-----------------------------------------------------------------------------
bool pqProxyWidget::resetProperty(vtkSMProperty* prop, double settingsPriority, bool warn)
{
  bool anyReset = !prop->IsValueDefault();
  prop->ResetToDefault();

  anyReset = prop->ResetToSettings(settingsPriority) || anyReset;

  if (warn && anyReset)
  {
    Q_EMIT this->changeAvailable();
    if (this->ApplyChangesImmediately)
    {
      this->apply();
    }
    Q_EMIT this->changeFinished();
  }

  return anyReset;
}

//-----------------------------------------------------------------------------
void pqProxyWidget::resetPropertyToAppDefault(pqPropertyWidget* propWidget)
{
  vtkSMProperty* smproperty = propWidget->property();
  this->resetProperty(smproperty, vtkSMSettings::GetApplicationPriority(), true);
}

//-----------------------------------------------------------------------------
void pqProxyWidget::resetPropertyToUserDefault(pqPropertyWidget* propWidget)
{
  vtkSMProperty* smproperty = propWidget->property();
  this->resetProperty(smproperty, vtkSMSettings::GetUserPriority(), true);
}
