/*=========================================================================

   Program: ParaView
   Module:    pqColorPresetManager.h

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

/// \file pqColorPresetManager.h
/// \date 3/12/2007

#ifndef _pqColorPresetManager_h
#define _pqColorPresetManager_h


#include "pqComponentsModule.h"
#include <QDialog>

#include "pqColorMapModel.h" // for pqColorMapModel

class pqColorPresetManagerForm;
class pqColorPresetModel;
class QItemSelectionModel;
class QModelIndex;
class QPoint;
class QStringList;
class vtkPVXMLElement;


class PQCOMPONENTS_EXPORT pqColorPresetManager : public QDialog
{
  Q_OBJECT

public:

  /// Used to control what kinds of presets are shown in the dialog.
  /// This merely affects the presets that are hidden from the view.
  enum Mode
    {
    SHOW_ALL,
    SHOW_INDEXED_COLORS_ONLY,
    SHOW_NON_INDEXED_COLORS_ONLY
    };

  pqColorPresetManager(QWidget *parent=0, Mode mode=SHOW_ALL);
  virtual ~pqColorPresetManager();

  pqColorPresetModel *getModel() const {return this->Model;}
  QItemSelectionModel *getSelectionModel() const;

  bool isUsingCloseButton() const;
  void setUsingCloseButton(bool showClose);

  /// Load compiled in presets.
  void loadBuiltinColorPresets();

  /// add a new colormap.
  void addColorMap(const pqColorMapModel& colorMap, const QString& name);

  void saveSettings();
  void restoreSettings();

  virtual bool eventFilter(QObject *object, QEvent *e);

  static pqColorMapModel createColorMapFromXML(vtkPVXMLElement *element);
  static bool saveColorMapToXML(const pqColorMapModel* colorMap, vtkPVXMLElement* element);

signals:
  void currentChanged(const pqColorMapModel* preset);

public slots:
  void importColorMap(const QStringList &files);
  void exportColorMap(const QStringList &files);

protected:
  virtual void showEvent(QShowEvent *e);

private slots:
  void importColorMap();
  void exportColorMap();
  void normalizeSelected();
  void removeSelected();
  void updateButtons();
  void showContextMenu(const QPoint &pos);
  void handleItemActivated();
  void selectNewItem(const QModelIndex &parent, int first, int last);
  void currentChanged(const QModelIndex &current);

private:
  void importColorMap(vtkPVXMLElement *element);
  void exportColorMap(const QModelIndex &index, vtkPVXMLElement *element);

private:
  pqColorPresetManagerForm *Form;
  pqColorPresetModel *Model;
  bool InitSections;
  Mode DialogMode;
};

#endif
