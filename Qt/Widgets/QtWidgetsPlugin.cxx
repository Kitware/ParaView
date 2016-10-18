/*=========================================================================

   Program: ParaView
   Module:    QtWidgetsPlugin.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

#if defined(NDEBUG) && !defined(QT_NO_DEBUG)
#define QT_NO_DEBUG
#endif

#include "QtWidgetsExport.h"
#ifndef QTWIDGETS_BUILD_SHARED_LIBS
#define QT_STATICPLUGIN
#endif

#include "QtWidgetsPlugin.h"
#include "pqCollapsedGroup.h"
#include "pqDoubleRangeWidget.h"

///////////////////////////////////////////////////////////////////////////
// pqCollapsedGroupPlugin

QString pqCollapsedGroupPlugin::name() const
{
  return "pqCollapsedGroup";
}

QString pqCollapsedGroupPlugin::domXml() const
{
  return "<widget class=\"pqCollapsedGroup\" name=\"collapsedGroup\">\n"
         " <property name=\"geometry\">\n"
         "  <rect>\n"
         "   <x>0</x>\n"
         "   <y>0</y>\n"
         "   <width>100</width>\n"
         "   <height>100</height>\n"
         "  </rect>\n"
         " </property>\n"
         "</widget>\n";
}

QWidget* pqCollapsedGroupPlugin::createWidget(QWidget* parent)
{
  return new pqCollapsedGroup(parent);
}

QString pqCollapsedGroupPlugin::group() const
{
  return "ParaView Qt Widgets";
}

QIcon pqCollapsedGroupPlugin::icon() const
{
  return QIcon(":/QtWidgets/Icons/pqCollapsedGroup22.png");
}

QString pqCollapsedGroupPlugin::includeFile() const
{
  return "pqCollapsedGroup.h";
}

QString pqCollapsedGroupPlugin::toolTip() const
{
  return "Collapsed Group Widget";
}

QString pqCollapsedGroupPlugin::whatsThis() const
{
  return "The Collapsed Group Widget is a container that can show/hide its children using a button";
}

bool pqCollapsedGroupPlugin::isContainer() const
{
  return true;
}

///////////////////////////////////////////////////////////////////////////
// pqDoubleRangeWidgetPlugin

QString pqDoubleRangeWidgetPlugin::name() const
{
  return "pqDoubleRangeWidget";
}

QString pqDoubleRangeWidgetPlugin::domXml() const
{
  return "<widget class=\"pqDoubleRangeWidget\" name=\"doubleRange\">\n"
         " <property name=\"geometry\">\n"
         "  <rect>\n"
         "   <x>0</x>\n"
         "   <y>0</y>\n"
         "   <width>24</width>\n"
         "   <height>100</height>\n"
         "  </rect>\n"
         " </property>\n"
         "</widget>\n";
}

QWidget* pqDoubleRangeWidgetPlugin::createWidget(QWidget* parent)
{
  return new pqDoubleRangeWidget(parent);
}

QString pqDoubleRangeWidgetPlugin::group() const
{
  return "ParaView Qt Widgets";
}

QIcon pqDoubleRangeWidgetPlugin::icon() const
{
  return QIcon(":/QtWidgets/Icons/pqCollapsedGroup22.png");
}

QString pqDoubleRangeWidgetPlugin::includeFile() const
{
  return "pqDoubleRangeWidget.h";
}

QString pqDoubleRangeWidgetPlugin::toolTip() const
{
  return "Double Range Widget";
}

QString pqDoubleRangeWidgetPlugin::whatsThis() const
{
  return "The Double Range Widget is a tied slider and line edit.";
}

bool pqDoubleRangeWidgetPlugin::isContainer() const
{
  return false;
}

/////////////////////////////////////////////////////////////////////////////
// QtWidgetsPlugin

QtWidgetsPlugin::QtWidgetsPlugin(QObject* p)
  : QObject(p)
{
  this->List.append(new pqCollapsedGroupPlugin());
  this->List.append(new pqDoubleRangeWidgetPlugin());
}

QList<QDesignerCustomWidgetInterface*> QtWidgetsPlugin::customWidgets() const
{
  return this->List;
}

Q_EXPORT_PLUGIN2(QtWidgets, QtWidgetsPlugin);
