// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqProxyInfo.h"
#include "pqProxyCategory.h"

#include "vtkNew.h"
#include "vtkPVXMLElement.h"
#include "vtkSMObject.h"

#include <QCoreApplication>

#include <algorithm>
#include <cstring>

namespace
{
/**
 * Removes leading and trailing blank lines and the indentation common to all
 * lines, so that Python code can be indented to match the XML it's part of.
 */
QString dedent(const QString& text)
{
  QStringList lines = text.split('\n');
  while (!lines.isEmpty() && lines.front().trimmed().isEmpty())
  {
    lines.pop_front();
  }
  while (!lines.isEmpty() && lines.back().trimmed().isEmpty())
  {
    lines.pop_back();
  }
  if (lines.isEmpty())
  {
    return QString();
  }

  auto indentation = [](const QString& line)
  {
    int count = 0;
    while (count < line.size() && (line[count] == ' ' || line[count] == '\t'))
    {
      ++count;
    }
    return count;
  };

  int common = -1;
  for (const QString& line : lines)
  {
    if (!line.trimmed().isEmpty())
    {
      common = common < 0 ? indentation(line) : std::min(common, indentation(line));
    }
  }
  for (QString& line : lines)
  {
    line.remove(0, std::min(std::max(common, 0), indentation(line)));
  }
  return lines.join('\n') + '\n';
}
}

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
  const char* elementName = xmlElement->GetName() ? xmlElement->GetName() : "";
  if (strcmp(elementName, "Script") == 0)
  {
    this->Type = ItemType::Script;
  }
  else if (strcmp(elementName, "Separator") == 0)
  {
    this->Type = ItemType::Separator;
  }

  this->Name = xmlElement->GetAttribute("name");
  this->Label = xmlElement->GetAttribute("label");
  this->Group = xmlElement->GetAttribute("group");
  this->Icon = xmlElement->GetAttribute("icon");
  if (this->isScript())
  {
    this->Script = ::dedent(QString::fromUtf8(xmlElement->GetCharacterData()));
    this->FileName = xmlElement->GetAttribute("filename");
    this->ToolTip = xmlElement->GetAttribute("tooltip");
    if (this->Name.isEmpty())
    {
      this->Name = QString(this->Label).remove('&');
    }
  }

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
  , Type(other->Type)
  , Name(other->Name)
  , Group(other->Group)
  , Label(other->Label)
  , Icon(other->Icon)
  , OmitFromToolbar(other->OmitFromToolbar)
  , HideFromMenu(other->HideFromMenu)
  , Script(other->Script)
  , FileName(other->FileName)
  , ToolTip(other->ToolTip)
{
}

//-----------------------------------------------------------------------------
void pqProxyInfo::convertToXML(vtkPVXMLElement* root)
{
  // Note: the code of inline scripts is not saved, scripts are expected to be
  // identified by their name.
  vtkNew<vtkPVXMLElement> proxyXML;
  proxyXML->SetName(this->isScript() ? "Script" : (this->isSeparator() ? "Separator" : "Proxy"));
  if (this->isProxy())
  {
    proxyXML->SetAttribute("group", this->group().toStdString().c_str());
  }
  proxyXML->SetAttribute("name", this->name().toStdString().c_str());
  if (!this->isSeparator())
  {
    proxyXML->SetAttribute("label", this->label().toStdString().c_str());
    proxyXML->SetAttribute("icon", this->icon().toStdString().c_str());
  }
  if (!this->FileName.isEmpty())
  {
    proxyXML->SetAttribute("filename", this->FileName.toStdString().c_str());
  }
  if (!this->ToolTip.isEmpty())
  {
    proxyXML->SetAttribute("tooltip", this->ToolTip.toStdString().c_str());
  }
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
  if (this->isSeparator())
  {
    return QString();
  }

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
void pqProxyInfo::setOmitFromToolbar(const QString& categoryName, bool omit)
{
  this->OmitFromToolbar.removeAll(categoryName);
  if (omit)
  {
    this->OmitFromToolbar << categoryName;
  }
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
  if (this->Script.isEmpty() && this->FileName.isEmpty())
  {
    this->Script = other->Script;
    this->FileName = other->FileName;
  }
  if (this->ToolTip.isEmpty())
  {
    this->ToolTip = other->ToolTip;
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

//-----------------------------------------------------------------------------
bool pqProxyInfo::isValid() const
{
  switch (this->Type)
  {
    case ItemType::Proxy:
      return !this->Name.isEmpty() && !this->Group.isEmpty();
    case ItemType::Script:
      return !this->Name.isEmpty() && (!this->Script.isEmpty() || !this->FileName.isEmpty());
    case ItemType::Separator:
      return !this->Name.isEmpty();
  }
  return false;
}

//-----------------------------------------------------------------------------
bool pqProxyInfo::hasSameCustomizableAttributes(pqProxyInfo* other)
{
  auto parentName = [](pqProxyInfo* info)
  {
    auto category = dynamic_cast<pqProxyCategory*>(info->parent());
    return category ? category->name() : QString();
  };
  return this->Icon == other->Icon && this->HideFromMenu == other->HideFromMenu &&
    this->OmitFromToolbar.contains(parentName(this)) ==
    other->OmitFromToolbar.contains(parentName(other));
}
