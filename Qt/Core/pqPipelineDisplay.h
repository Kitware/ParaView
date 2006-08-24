/*=========================================================================

   Program: ParaView
   Module:    pqPipelineDisplay.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

/// \file pqPipelineDisplay.h
/// \date 4/24/2006

#ifndef _pqPipelineDisplay_h
#define _pqPipelineDisplay_h


#include "pqProxy.h"
#include <QPair>

class pqPipelineDisplayInternal;
class pqPipelineSource;
class pqRenderModule;
class pqServer;

class vtkPVDataSetAttributesInformation;
class vtkPVDataSetAttributesInformation;
class vtkPVArrayInformation;

class vtkSMDataObjectDisplayProxy;

/// This is PQ representation for a single display. A pqDisplay represents
/// a single vtkSMDataObjectDisplayProxy. The display can be added to
/// only one render module or more (ofcouse on the same server, this class
/// doesn't worry about that.
class PQCORE_EXPORT pqPipelineDisplay : public pqProxy
{
  Q_OBJECT
public:
  pqPipelineDisplay(const QString& name,
    vtkSMDataObjectDisplayProxy* display, pqServer* server,
    QObject* parent=NULL);
  virtual ~pqPipelineDisplay();

  // Get the internal display proxy.
  vtkSMDataObjectDisplayProxy* getDisplayProxy() const;

  // Get the source/filter of which this is a display.
  pqPipelineSource* getInput() const;

  // Sets the default color mapping for the display.
  // The rules are:
  // If the source created a NEW point scalar array, use it.
  // Else if the source created a NEW cell scalar array, use it.
  // Else if the input color by array exists in this source, use it.
  // Else color by property.
  void setDefaultColorParametes(); 

  // Call to select the coloring array. 
  void colorByArray(const char* arrayname, int fieldtype);

  // Returns if the display is shown in the given render module.
  // Note that for a display to be visible in a render module,
  // it must be \c shownIn that render modules as well as 
  // visibility must be set to 1.
  bool shownIn(pqRenderModule* rm) const;

  // Returns if the status of the visbility property of this display.
  // Note that for a display to be visible in a render module,
  // it must be \c shownIn that render modules as well as 
  // visibility must be set to 1.
  bool isVisible() const;

  // Set the visibility. Note that this affects the visibility of the
  // display in all render modules it is added to, and only in all the
  // render modules it is added to. This method does not call a re-render
  // on the render module, caller must call that explicitly.
  void setVisible(bool visible);

  // Get the number of render modules this display is present in.
  unsigned int getNumberOfRenderModules() const;

  // Get the render module this display is present in at the given 
  // index.
  pqRenderModule* getRenderModule(unsigned int index) const;

  // This method updates all render modules to which this 
  // display belongs, if force is true, it for an immediate render
  // otherwise render on idle.
  void renderAllViews(bool force=false);

  /// get the names of the arrays that a part may be colored by
  QList<QString> getColorFields();

  /// get the data ranges for a color field
  QList<QPair<double, double> > getColorFieldRanges(const QString& array);

  /// set the array to color the part by
  void setColorField(const QString& field);

  /// get the array the part is colored by
  /// if raw is true, it will not add (point) or (cell) but simply
  /// return the array name
  QString getColorField(bool raw=false);

signals:
  // Fired when the visibility property of the underlying display changes.
  // It must be noted that this is fired on the property change, the property
  // is not pushed yet, hence the visibility of the underlying VTK prop
  // hasn't changed.
  void visibilityChanged(bool visible);

protected slots:
  // called when input property on display changes. We must detect if
  // (and when) the display is connected to a new proxy.
  virtual void onInputChanged();

  // called when the display visibility property changes.
  void onVisibilityChanged();

protected:
  friend class pqRenderModule;
  void addRenderModule(pqRenderModule* rm);
  void removeRenderModule(pqRenderModule* rm);

private:
  pqPipelineDisplayInternal *Internal; 
  static void getColorArray(
    vtkPVDataSetAttributesInformation* attrInfo,
    vtkPVDataSetAttributesInformation* inAttrInfo,
    vtkPVArrayInformation*& arrayInfo);
};

#endif
