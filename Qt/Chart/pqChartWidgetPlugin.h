/*=========================================================================

   Program: ParaView
   Module:    pqChartWidgetPlugin.h

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

#ifndef _pqChartWidgetPlugin_h
#define _pqChartWidgetPlugin_h

#include <QObject>
#include <QDesignerCustomWidgetInterface>


/// \class pqChartWidgetPlugin
/// \brief
///   The pqChartWidgetPlugin class is an abstract interface used to
///   create a chart widget.
class pqChartWidgetPlugin : public QObject,
    public QDesignerCustomWidgetInterface
{
  Q_OBJECT
  Q_INTERFACES(QDesignerCustomWidgetInterface)

public:
  /// \brief
  ///   Creates an instance of the chart plugin.
  /// \param parent The parent object.
  pqChartWidgetPlugin(QObject *parent=0);
  virtual ~pqChartWidgetPlugin() {}

  /// \brief
  ///   Creates a new chart widget with the given parent.
  /// \param parent The parent object.
  /// \return
  ///   A pointer to a new chart widget.
  virtual QWidget *createWidget(QWidget *parent=0);

  /// \brief
  ///   Gets the xml string for the chart widget.
  /// \return
  ///   The xml string for the chart widget.
  virtual QString domXml() const;

  /// \brief
  ///   Gets the name of the widget's designer group.
  /// \return
  ///   The name of the widget's designer group.
  virtual QString group() const {return QLatin1String("ParaView Chart Widgets");}

  /// \brief
  ///   Gets the icon for the widget.
  /// \return
  ///   The icon for the widget.
  virtual QIcon icon() const;

  /// \brief
  ///   Gets the include file for the widget.
  /// \return
  ///   The include file for the widget.
  virtual QString includeFile() const;

  /// \brief
  ///   Gets wether or not the widget is a container.
  /// \return
  ///   True if the widget is a container.
  virtual bool isContainer() const {return false;}

  /// \brief
  ///   Gets the widget class name.
  /// \return
  ///   The widget class name.
  virtual QString name() const {return QLatin1String("pqChartWidget");}

  /// \brief
  ///   Gets the tool tip for the widget.
  /// \return
  ///   The tool tip for the widget.
  virtual QString toolTip() const;

  /// \brief
  ///   Gets the what's this tip for the widget.
  /// \return
  ///   The what's this tip for the widget.
  virtual QString whatsThis() const;
};

#endif
