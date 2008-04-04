/*=========================================================================

   Program: ParaView
   Module:    pqChartPlugin.h

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

/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqChartPlugin_h
#define _pqChartPlugin_h

#include <QDesignerCustomWidgetInterface>
#include <QDesignerCustomWidgetCollectionInterface>
#include <qplugin.h>
#include <QObject>

class pqChartWidgetPlugin;
class pqColorMapWidgetPlugin;


/// \class pqChartPlugin
/// \brief
///   The pqChartPlugin class contains the custom widgets for the
///   charting package.
class pqChartPlugin : public QObject,
    public QDesignerCustomWidgetCollectionInterface
{
  Q_OBJECT
  Q_INTERFACES(QDesignerCustomWidgetCollectionInterface)

public:
  /// \brief
  ///   Creates a chart plugin container.
  /// \param parent The parent object.
  pqChartPlugin(QObject *parent=0);
  virtual ~pqChartPlugin();

  /// \brief
  ///   Gets the list of plugins for the charting package.
  /// \return
  ///   A list of plugins for the charting package.
  virtual QList<QDesignerCustomWidgetInterface*> customWidgets() const;

private:
  pqChartWidgetPlugin *Chart;       ///< Stores the chart plugin.
  pqColorMapWidgetPlugin *ColorMap; ///< Stores the color map plugin.
};

#endif
