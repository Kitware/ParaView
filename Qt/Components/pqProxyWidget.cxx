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
#include "vtkPVXMLElement.h"
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
#include "vtkSMProxy.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSettings.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMTrace.h"
#include "vtkSmartPointer.h"
#include "vtkStringList.h"

#include <QHideEvent>
#include <QLabel>
#include <QPointer>
#include <QShowEvent>
#include <QVBoxLayout>

#include <cmath>
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

QMap<QString, vtkPVXMLElement*> getDecorators(vtkPVXMLElement* hints)
{
  QMap<QString, vtkPVXMLElement*> decoratorTypes;
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
      decoratorTypes[elem->GetAttribute("type")] = elem;
    }
  }

  return decoratorTypes;
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
  int GroupTag;

  pqProxyWidgetItem(QObject* parentObj)
    : Superclass(parentObj)
    , Group(false)
    , GroupTag(-1)
    , Advanced(false)
  {
  }

public:
  // Regular expression with tags used to match search text.
  QStringList SearchTags;
  bool Advanced;

  ~pqProxyWidgetItem()
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
  static pqProxyWidgetItem* newMultiItemGroupItem(int group_id, const QString& group_label,
    pqPropertyWidget* widget, const QString& widget_label, QObject* parentObj)
  {
    pqProxyWidgetItem* item = newItem(widget, widget_label, parentObj);
    item->Group = true;
    item->GroupTag = group_id;
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
      this->GroupHeader->setVisible(this->GroupTag == -1 || prevVisibleItem == NULL ||
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
}

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
    Q_ASSERT(gridLayout);
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
  Q_ASSERT(smproxy);
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
    QString type = xml->GetAttributeOrDefault("type", "description");
    type = type.toLower();
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

  PV_DEBUG_PANELS() << "------------------------------------------------------";
  PV_DEBUG_PANELS() << "Creating Properties Panel for" << smproxy->GetXMLLabel() << "("
                    << smproxy->GetXMLGroup() << "," << smproxy->GetXMLName() << ")";
  PV_DEBUG_PANELS() << "------------------------------------------------------";

  QGridLayout* gridLayout = qobject_cast<QGridLayout*>(this->layout());
  Q_ASSERT(gridLayout);

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

  // build collection of all properties that are contained in a group

  // map where key is a property and value is the group_tag for the group
  // the property belongs to. Several properties can belong to the same group.
  QMap<vtkSMProperty*, int> groupProperties;

  // map of a group-tag and the pqProxyWidgetItem for the group, if any.
  QMap<int, QPointer<pqProxyWidgetItem> > groupItems;
  QMap<int, QString> groupLabels;
  QMap<int, vtkPVXMLElement*> groupHints;

  for (size_t index = 0; index < smproxy->GetNumberOfPropertyGroups(); index++)
  {
    int group_tag = static_cast<int>(index);
    vtkSMPropertyGroup* group = smproxy->GetPropertyGroup(index);
    for (size_t j = 0; j < group->GetNumberOfProperties(); j++)
    {
      groupProperties[group->GetProperty(static_cast<unsigned int>(j))] = group_tag;
    }
    groupLabels[group_tag] = group->GetXMLLabel();
    groupHints[group_tag] = group->GetHints();

    if (group->GetNumberOfProperties() == 0)
    {
      // skip empty groups.
      continue;
    }

    bool ignorePanelVisibility = false;
    if (!properties.isEmpty())
    {
      if (!properties.contains(group->GetXMLLabel()))
      {
        continue;
      }
      else
      {
        ignorePanelVisibility = true;
      }
    }

    if (QString(group->GetPanelVisibility()) == "never" && !ignorePanelVisibility)
    {
      // skip property groups marked as never show
      PV_DEBUG_PANELS() << "  - Group " << group->GetXMLLabel()
                        << " gets skipped because it has panel_visibility of \"never\"";

      // set an empty pqProxyWidgetItem so we don't create a "container"
      // group for this property group.
      groupItems[group_tag] = NULL;
      continue;
    }

    pqInterfaceTracker* interfaceTracker = pqApplicationCore::instance()->interfaceTracker();
    foreach (pqPropertyWidgetInterface* interface,
      interfaceTracker->interfaces<pqPropertyWidgetInterface*>())
    {
      pqPropertyWidget* propertyWidget = interface->createWidgetForPropertyGroup(smproxy, group);
      if (propertyWidget)
      {
        PV_DEBUG_PANELS() << "Group " << group->GetXMLLabel() << " is controlled by widget "
                          << propertyWidget->metaObject()->className();

        // Create decorators, if any.
        QMap<QString, vtkPVXMLElement*> decoratorTypes = getDecorators(group->GetHints());
        foreach (const QString& type, decoratorTypes.keys())
        {
          if (interface->createWidgetDecorator(type, decoratorTypes[type], propertyWidget))
          {
            break;
          }
        }

        propertyWidget->setParent(this);
        QString groupTypeName = group->GetPanelWidget();
        groupTypeName.replace(" ", "");
        propertyWidget->setObjectName(groupTypeName);

        // save record of the property widget
        pqProxyWidgetItem* item =
          pqProxyWidgetItem::newGroupItem(propertyWidget, group->GetXMLLabel(), this);
        item->Advanced = QString(group->GetPanelVisibility()) == "advanced";
        item->SearchTags << group->GetPanelWidget() << group->GetXMLLabel();
        // FIXME: Maybe SearchTags should have the labels for all the properties
        // in this group.

        groupItems[group_tag] = item;
        break;
      }
    }
  }

  // Handle legacy-hidden properties: previously, developers hid properties from
  // panels by adding XML in the hints section. We need to ensure that that
  // works too. So, we'll scan the hints and create a list of properties to
  // hide.
  QSet<QString> legacyHiddenProperties;
  DetermineLegacyHiddenProperties(legacyHiddenProperties, smproxy);

  // iterate over each property, and create corresponding widgets
  vtkNew<vtkSMOrderedPropertyIterator> propertyIter;
  propertyIter->SetProxy(smproxy);

  bool isCompoundProxy = smproxy->IsA("vtkSMCompoundSourceProxy");

  for (propertyIter->Begin(); !propertyIter->IsAtEnd(); propertyIter->Next())
  {
    vtkSMProperty* smProperty = propertyIter->GetProperty();

    QString propertyKeyName = propertyIter->GetKey();
    propertyKeyName.replace(" ", "");
    const char* xmlLabel = (smProperty->GetXMLLabel() && !isCompoundProxy)
      ? smProperty->GetXMLLabel()
      : propertyIter->GetKey();

    QString xmlDocumentation = pqProxyWidget::documentationText(smProperty);

    bool ignorePanelVisibility = false;
    if (!properties.isEmpty())
    {
      if (!properties.contains(propertyKeyName))
      {
        PV_DEBUG_PANELS() << "Property:" << propertyIter->GetKey() << " ("
                          << smProperty->GetXMLLabel() << ")"
                          << " gets skipped because it is not listed in the properties argument";
        PV_DEBUG_PANELS() << ""; // this adds a newline.
        continue;
      }
      else
      {
        ignorePanelVisibility = true;
      }
    }

    if (smProperty->GetInformationOnly())
    {
      // skip information only properties
      PV_DEBUG_PANELS() << "Property:" << propertyIter->GetKey() << " ("
                        << smProperty->GetXMLLabel() << ")"
                        << " gets skipped because it is an information only property";
      PV_DEBUG_PANELS() << ""; // this adds a newline.
      continue;
    }

    if (smProperty->GetIsInternal())
    {
      // skip internal properties
      PV_DEBUG_PANELS() << "Property:" << propertyIter->GetKey() << " ("
                        << smProperty->GetXMLLabel() << ")"
                        << " gets skipped because it is an internal property";
      PV_DEBUG_PANELS() << ""; // this adds a newline.
      continue;
    }

    if (QString(smProperty->GetPanelVisibility()) == "never" && !ignorePanelVisibility)
    {
      // skip properties marked as never show
      PV_DEBUG_PANELS() << "Property:" << propertyIter->GetKey() << "(" << smProperty->GetXMLLabel()
                        << ")"
                        << " gets skipped because it has panel_visibility of \"never\"";
      PV_DEBUG_PANELS() << ""; // this adds a newline.
      continue;
    }

    if (legacyHiddenProperties.contains(propertyIter->GetKey()))
    {
      // skipping properties marked with "show=0" in the hints section.
      PV_DEBUG_PANELS() << "Property:" << propertyIter->GetKey() << "(" << smProperty->GetXMLLabel()
                        << ")"
                        << " gets skipped because is has show='0' specified in the Hints.";
      continue;
    }

    int property_group_tag = -1;
    if (groupProperties.contains(smProperty))
    {
      property_group_tag = groupProperties[smProperty];
      if (groupItems.contains(property_group_tag))
      {
        if (groupItems[property_group_tag] != NULL)
        {
          // insert the group-item.
          this->Internals->appendToItems(groupItems[property_group_tag], this);

          // clear the widget so we don't add it multiple times.
          groupItems[property_group_tag] = NULL;
        }
        // group widget has been added for this property. skip the rest.
        continue;
      }

      // this property belongs to a non-hidden group which doesn't have any
      // custom widget. That simply means we will add framing around this
      // property.
    }

    // create property widget
    PV_DEBUG_PANELS() << "Property:" << propertyIter->GetKey() << "(" << xmlLabel << ")";
    pqPropertyWidget* propertyWidget = this->createWidgetForProperty(smProperty, smproxy, this);
    if (!propertyWidget)
    {
      PV_DEBUG_PANELS() << "Property:" << propertyIter->GetKey() << "(" << smProperty->GetXMLLabel()
                        << ")"
                        << " gets skipped as we could not determine the widget type to create.";
      continue;
    }
    propertyWidget->setObjectName(propertyKeyName);

    QString itemLabel = this->UseDocumentationForLabels
      ? QString("<p><b>%1</b>: %2</p>").arg(xmlLabel).arg(xmlDocumentation)
      : QString(xmlLabel);

    pqProxyWidgetItem* item = property_group_tag == -1
      ? pqProxyWidgetItem::newItem(propertyWidget, QString(itemLabel), this)
      : pqProxyWidgetItem::newMultiItemGroupItem(property_group_tag,
          groupLabels[property_group_tag], propertyWidget, QString(itemLabel), this);

    // save record of the property widget and containing widget
    item->SearchTags << xmlLabel << xmlDocumentation << propertyIter->GetKey();
    item->Advanced = QString(smProperty->GetPanelVisibility()) == "advanced";

    if (smProperty->GetPanelVisibilityDefaultForRepresentation())
    {
      item->appendToDefaultVisibilityForRepresentations(
        smProperty->GetPanelVisibilityDefaultForRepresentation());
    }

    if (property_group_tag != -1)
    {
      // Create decorators, if any.
      pqInterfaceTracker* interfaceTracker = pqApplicationCore::instance()->interfaceTracker();
      foreach (pqPropertyWidgetInterface* interface,
        interfaceTracker->interfaces<pqPropertyWidgetInterface*>())
      {
        QMap<QString, vtkPVXMLElement*> decoratorTypes =
          getDecorators(groupHints[property_group_tag]);
        foreach (const QString& type, decoratorTypes.keys())
        {
          if (interface->createWidgetDecorator(type, decoratorTypes[type], propertyWidget))
          {
            break;
          }
        }
      }
    }

    this->Internals->appendToItems(item, this);
    PV_DEBUG_PANELS() << ""; // this adds a newline.
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
    widget = interface->createWidgetForProperty(smproxy, smproperty);
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
    widget = new pqStringVectorPropertyWidget(svp, smproxy, parentObj);
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
    widget->setProperty(smproperty);
  }

  // Create decorators, if any.
  QMap<QString, vtkPVXMLElement*> decoratorTypes = getDecorators(smproperty->GetHints());
  foreach (const QString& type, decoratorTypes.keys())
  {
    for (int cc = 0; cc < interfaces.size(); cc++)
    {
      pqPropertyWidgetInterface* interface = interfaces[cc];
      if (interface->createWidgetDecorator(type, decoratorTypes[type], widget))
      {
        break;
      }
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
