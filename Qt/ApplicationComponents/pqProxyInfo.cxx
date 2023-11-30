// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqProxyInfo.h"
#include "pqProxyCategory.h"

#include "vtkNew.h"
#include "vtkPVXMLElement.h"
#include "vtkSMObject.h"

#include <QCoreApplication>

//-----------------------------------------------------------------------------
pqProxyInfo::pqProxyInfo(pqProxyCategory* parent, const QString& name, const QString& group,
  const QString& label, const QString& icon, const QStringList& omitFromToolbar, bool hide)
  : Superclass(parent)
  , Name(name)
  , Group(group)
  , Label(label)
  , Icon(icon)
  , OmitFromToolbar(omitFromToolbar)
  , HideFromMenu(hide)
{
}

//-----------------------------------------------------------------------------
pqProxyInfo::pqProxyInfo(pqProxyCategory* parent, vtkPVXMLElement* xmlElement)
  : Superclass(parent)
{
  this->Name = xmlElement->GetAttribute("name");
  this->Label = xmlElement->GetAttribute("label");
  this->Group = xmlElement->GetAttribute("group");
  this->Icon = xmlElement->GetAttribute("icon");

  int omit = 0;
  xmlElement->GetScalarAttribute("omit_from_toolbar", &omit);
  if (omit == 1)
  {
    this->OmitFromToolbar << parent->name();
  }

  int hide = 0;
  xmlElement->GetScalarAttribute("hide_from_menu", &hide);
  this->HideFromMenu = hide == 1;
}

//-----------------------------------------------------------------------------
pqProxyInfo::pqProxyInfo(pqProxyCategory* parent, pqProxyInfo* other)
  : Superclass(parent)
  , Name(other->Name)
  , Group(other->Group)
  , Label(other->Label)
  , Icon(other->Icon)
  , OmitFromToolbar(other->OmitFromToolbar)
  , HideFromMenu(other->HideFromMenu)
{
}

//-----------------------------------------------------------------------------
void pqProxyInfo::convertToXML(vtkPVXMLElement* root)
{
  vtkNew<vtkPVXMLElement> proxyXML;
  proxyXML->SetName("Proxy");
  proxyXML->SetAttribute("group", this->group().toStdString().c_str());
  proxyXML->SetAttribute("name", this->name().toStdString().c_str());
  proxyXML->SetAttribute("label", this->label().toStdString().c_str());
  proxyXML->SetAttribute("icon", this->icon().toStdString().c_str());
  if (this->hideFromMenu())
  {
    proxyXML->SetAttribute("hide_from_menu", "1");
  }

  auto parentCategory = dynamic_cast<pqProxyCategory*>(this->parent());
  if (this->omitFromToolbar().contains(parentCategory->name()))
  {
    proxyXML->SetAttribute("omit_from_toolbar", "1");
  }

  root->AddNestedElement(proxyXML);
}

//-----------------------------------------------------------------------------
QString pqProxyInfo::label()
{
  QString label = this->Label;
  if (label.isEmpty())
  {
    label = vtkSMObject::CreatePrettyLabel(this->Name.toStdString()).c_str();
  }

  // we do not store the translation, as we need the original label when writing to xml.
  return QCoreApplication::translate("ServerManagerXML", label.toStdString().c_str());
}

//-----------------------------------------------------------------------------
void pqProxyInfo::setLabel(const QString& newLabel)
{
  this->Label = newLabel;
}

//-----------------------------------------------------------------------------
void pqProxyInfo::setHideFromMenu(bool hide)
{
  this->HideFromMenu = hide;
}

//-----------------------------------------------------------------------------
void pqProxyInfo::setIcon(const QString& iconPath)
{
  this->Icon = iconPath;
}

//-----------------------------------------------------------------------------
void pqProxyInfo::merge(pqProxyInfo* other)
{
  if (this->Name.isEmpty())
  {
    this->Name = other->Name;
  }
  if (this->Group.isEmpty())
  {
    this->Group = other->Group;
  }
  if (this->Label.isEmpty())
  {
    this->Label = other->Label;
  }
  if (this->Icon.isEmpty())
  {
    this->Icon = other->Icon;
  }

  for (const auto& toolbar : other->OmitFromToolbar)
  {
    if (!this->OmitFromToolbar.contains(toolbar))
    {
      this->OmitFromToolbar.append(toolbar);
    }
  }
}

//-----------------------------------------------------------------------------
bool pqProxyInfo::hideFromMenu()
{
  return this->HideFromMenu;
}
