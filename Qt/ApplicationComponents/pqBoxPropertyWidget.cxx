// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqBoxPropertyWidget.h"
#include "ui_pqBoxPropertyWidget.h"

#include "pqActiveObjects.h"
#include "pqUndoStack.h"
#include "pqWidgetUtilities.h"

#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkSMNewWidgetRepresentationProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMUncheckedPropertyHelper.h"
#include "vtkTransform.h"

#include <QCoreApplication>
#include <QLocale>
#include <QPlainTextEdit>

//-----------------------------------------------------------------------------
class pqBoxPropertyWidget::pqUi : public Ui::BoxPropertyWidget
{
};
namespace
{
//-----------------------------------------------------------------------------
// Strict C-locale floating-point token parser.
// Valid form:
//   [sign] (integer | decimal) [exponent]
//     sign     = '+' | '-'
//     integer  = digits
//     decimal  = digits '.' [digits] | '.' digits
//     exponent = ('e' | 'E') [sign] digits
bool parseStrictNumberToken(const QString& token, double& value)
{
  // Trim whitespace before checking
  const QString trimmedToken = token.trimmed();
  if (trimmedToken.isEmpty())
  {
    return false;
  }
  // Parse strictly with C-locale rules, using '.' as decimal separator.
  // QLocale::c().toDouble() returns false on invalid formats such as "1e", "1.2.3", etc.
  bool parseOk = false;
  value = QLocale::c().toDouble(trimmedToken, &parseOk);
  return parseOk;
}

//-----------------------------------------------------------------------------
// Parse a 4×4 matrix from a text block.
//
// Expected format:
//   - Exactly 4 non-empty lines
//   - Each line contains exactly 4 numeric values
//   - Values may be separated by whitespace and/or commas
//   - Numbers must follow strict C-locale floating-point syntax
//       (digits, optional sign, optional decimal point, optional exponent)
//
// Acceptance examples:
//   1 0 0 0
//   0,1,0,0
//   0 0.0e0 1.0E-3 0
//   .5 0 0 1
//
// Rejected examples:
//   3 lines / 5 lines
//   non-numeric tokens (inf, nan, 1e, 1.2.3)
//   locale formats (1,2 meaning 1.2)
//   trailing characters or mixed text ("1.0f", "[1 0 0 0]")
bool parseMatrixText(const QString& text, double matrix[16])
{
  // Split into lines, keeping empty ones so we can trim leading/trailing empties.
  QStringList lines = text.split(QLatin1Char('\n'), Qt::KeepEmptyParts);

  // Trim whitespace on each line.
  for (QString& lineStr : lines)
  {
    lineStr = lineStr.trimmed();
  }
  // Remove empty leading lines.
  while (!lines.isEmpty() && lines.first().isEmpty())
  {
    lines.removeFirst();
  }
  // Remove empty trailing lines.
  while (!lines.isEmpty() && lines.last().isEmpty())
  {
    lines.removeLast();
  }
  if (lines.size() != 4)
  {
    vtkGenericWarningMacro(<< "InteractiveBox Matrix: expected exactly 4 lines, found "
                           << lines.size() << ".");
    return false;
  }

  int idx = 0;
  // Parse each of the 4 lines.
  for (int row = 0; row < 4; ++row)
  {
    QString line = lines[row];

    // Allow commas as separators; convert them to spaces.
    line.replace(QLatin1Char(','), QLatin1Char(' '));

    // Collapse multiple spaces into a single space.
    line = line.simplified();

    // Split into tokens. Must have exactly 4 values.
    const QStringList tokens = line.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (tokens.size() != 4)
    {
      vtkGenericWarningMacro(<< "InteractiveBox Matrix: line " << (row + 1) << " has "
                             << tokens.size() << " values; expected 4.");
      return false;
    }
    // Parse each numeric value using the strict token parser.
    for (int col = 0; col < 4; ++col)
    {
      double val = 0.0;
      if (!parseStrictNumberToken(tokens[col], val))
      {
        vtkGenericWarningMacro(<< "InteractiveBox Matrix: line " << (row + 1) << ", column "
                               << (col + 1) << ": invalid number token '"
                               << tokens[col].toStdString() << "'.");
        return false;
      }
      matrix[idx++] = val;
    }
  }
  return (idx == 16);
}

//-----------------------------------------------------------------------------
// Compose 4x4 from PRS (deg for rotation)
void composeMatrixFromPRS(
  const double pos[3], const double rotDeg[3], const double scale[3], double outMatrix[16])
{
  vtkNew<vtkTransform> transform;
  transform->Identity();
  transform->Translate(const_cast<double*>(pos));
  transform->RotateZ(rotDeg[2]);
  transform->RotateX(rotDeg[0]);
  transform->RotateY(rotDeg[1]);
  transform->Scale(const_cast<double*>(scale));

  vtkMatrix4x4* matrix4x4 = transform->GetMatrix();
  for (int row = 0; row < 4; ++row)
  {
    for (int col = 0; col < 4; ++col)
    {
      outMatrix[row * 4 + col] = matrix4x4->GetElement(row, col);
    }
  }
}

//-----------------------------------------------------------------------------
// Decompose 4x4 to PRS (deg for rotation)
void decomposeMatrixToPRS(
  const double inMatrix[16], double pos[3], double rotDeg[3], double scale[3])
{
  vtkNew<vtkMatrix4x4> matrix4x4;
  for (int row = 0; row < 4; ++row)
  {
    for (int col = 0; col < 4; ++col)
    {
      matrix4x4->SetElement(row, col, inMatrix[row * 4 + col]);
    }
  }

  vtkNew<vtkTransform> transform;
  transform->SetMatrix(matrix4x4);
  transform->GetPosition(pos);
  transform->GetOrientation(rotDeg);
  transform->GetScale(scale);
}

//-----------------------------------------------------------------------------
// Read Position, Rotation, Scale (each 3-tuple) from SM properties
// into provided arrays using unchecked property helpers. Assumes
// all three properties are non-null and have at least 3 components.
void readPRSFromProperties(vtkSMProperty* positionProperty, vtkSMProperty* rotationProperty,
  vtkSMProperty* scaleProperty, double pos[3], double rot[3], double scl[3])
{
  vtkSMUncheckedPropertyHelper positionHelper(positionProperty);
  for (int index = 0; index < 3; ++index)
  {
    pos[index] = positionHelper.GetAsDouble(index);
  }

  vtkSMUncheckedPropertyHelper rotationHelper(rotationProperty);
  for (int index = 0; index < 3; ++index)
  {
    rot[index] = rotationHelper.GetAsDouble(index);
  }

  vtkSMUncheckedPropertyHelper scaleHelper(scaleProperty);
  for (int index = 0; index < 3; ++index)
  {
    scl[index] = scaleHelper.GetAsDouble(index);
  }
}
} // end namespace

//-----------------------------------------------------------------------------
pqBoxPropertyWidget::pqBoxPropertyWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject, bool hideReferenceBounds)
  : Superclass("representations", "BoxWidgetRepresentation", smproxy, smgroup, parentObject)
  , Ui(new pqBoxPropertyWidget::pqUi())
  , BoxIsRelativeToInput(false)
{
  this->Ui->setupUi(this);
  pqWidgetUtilities::formatChildTooltips(this);

  this->Position = smgroup->GetProperty("Position");
  this->Rotation = smgroup->GetProperty("Rotation");
  this->Scale = smgroup->GetProperty("Scale");

  vtkSMProxy* wdgProxy = this->widgetProxy();

  // Let's link some of the UI elements that only affect the interactive widget
  // properties without affecting properties on the main proxy.
  this->WidgetLinks.addPropertyLink(this->Ui->enableTranslation, "checked", SIGNAL(toggled(bool)),
    wdgProxy, wdgProxy->GetProperty("TranslationEnabled"));
  this->WidgetLinks.addPropertyLink(this->Ui->enableScaling, "checked", SIGNAL(toggled(bool)),
    wdgProxy, wdgProxy->GetProperty("ScalingEnabled"));
  this->WidgetLinks.addPropertyLink(this->Ui->enableRotation, "checked", SIGNAL(toggled(bool)),
    wdgProxy, wdgProxy->GetProperty("RotationEnabled"));
  this->WidgetLinks.addPropertyLink(this->Ui->enableMoveFaces, "checked", SIGNAL(toggled(bool)),
    wdgProxy, wdgProxy->GetProperty("MoveFacesEnabled"));

  if (vtkSMProperty* position = this->Position)
  {
    this->addPropertyLink(
      this->Ui->translateX, "text2", SIGNAL(textChangedAndEditingFinished()), position, 0);
    this->addPropertyLink(
      this->Ui->translateY, "text2", SIGNAL(textChangedAndEditingFinished()), position, 1);
    this->addPropertyLink(
      this->Ui->translateZ, "text2", SIGNAL(textChangedAndEditingFinished()), position, 2);
    this->Ui->labelTranslate->setText(
      QCoreApplication::translate("ServerManagerXML", position->GetXMLLabel()));
    QString tooltip = this->getTooltip(position);
    this->Ui->translateX->setToolTip(tooltip);
    this->Ui->translateY->setToolTip(tooltip);
    this->Ui->translateZ->setToolTip(tooltip);
    this->Ui->labelTranslate->setToolTip(tooltip);
  }
  else
  {
    this->Ui->labelTranslate->hide();
    this->Ui->translateX->hide();
    this->Ui->translateY->hide();
    this->Ui->translateZ->hide();

    // see WidgetLinks above.
    this->Ui->enableTranslation->setChecked(false);
    this->Ui->enableTranslation->hide();
  }

  if (vtkSMProperty* rotation = this->Rotation)
  {
    this->addPropertyLink(
      this->Ui->rotateX, "text2", SIGNAL(textChangedAndEditingFinished()), rotation, 0);
    this->addPropertyLink(
      this->Ui->rotateY, "text2", SIGNAL(textChangedAndEditingFinished()), rotation, 1);
    this->addPropertyLink(
      this->Ui->rotateZ, "text2", SIGNAL(textChangedAndEditingFinished()), rotation, 2);
    this->Ui->labelRotate->setText(
      QCoreApplication::translate("ServerManagerXML", rotation->GetXMLLabel()));
    QString tooltip = this->getTooltip(rotation);
    this->Ui->rotateX->setToolTip(tooltip);
    this->Ui->rotateY->setToolTip(tooltip);
    this->Ui->rotateZ->setToolTip(tooltip);
    this->Ui->labelRotate->setToolTip(tooltip);
  }
  else
  {
    this->Ui->labelRotate->hide();
    this->Ui->rotateX->hide();
    this->Ui->rotateY->hide();
    this->Ui->rotateZ->hide();

    // see WidgetLinks above.
    this->Ui->enableRotation->setChecked(false);
    this->Ui->enableRotation->hide();
  }

  if (vtkSMProperty* scale = this->Scale)
  {
    this->addPropertyLink(
      this->Ui->scaleX, "text2", SIGNAL(textChangedAndEditingFinished()), scale, 0);
    this->addPropertyLink(
      this->Ui->scaleY, "text2", SIGNAL(textChangedAndEditingFinished()), scale, 1);
    this->addPropertyLink(
      this->Ui->scaleZ, "text2", SIGNAL(textChangedAndEditingFinished()), scale, 2);
    this->Ui->labelScale->setText(
      QCoreApplication::translate("ServerManagerXML", scale->GetXMLLabel()));
    QString tooltip = this->getTooltip(scale);
    this->Ui->scaleX->setToolTip(tooltip);
    this->Ui->scaleY->setToolTip(tooltip);
    this->Ui->scaleZ->setToolTip(tooltip);
    this->Ui->labelScale->setToolTip(tooltip);
  }
  else
  {
    this->Ui->labelScale->hide();
    this->Ui->scaleX->hide();
    this->Ui->scaleY->hide();
    this->Ui->scaleZ->hide();

    // see WidgetLinks above.
    this->Ui->enableScaling->setChecked(false);
    this->Ui->enableScaling->hide();
    this->Ui->enableMoveFaces->setChecked(false);
    this->Ui->enableMoveFaces->hide();
  }

  auto useRefBounds = smgroup->GetProperty("UseReferenceBounds");
  auto refBounds = smgroup->GetProperty("ReferenceBounds");
  if (useRefBounds && refBounds)
  {
    this->addPropertyLink(
      this->Ui->useReferenceBounds, "checked", SIGNAL(toggled(bool)), useRefBounds);
    this->addPropertyLink(
      this->Ui->xmin, "text2", SIGNAL(textChangedAndEditingFinished()), refBounds, 0);
    this->addPropertyLink(
      this->Ui->xmax, "text2", SIGNAL(textChangedAndEditingFinished()), refBounds, 1);
    this->addPropertyLink(
      this->Ui->ymin, "text2", SIGNAL(textChangedAndEditingFinished()), refBounds, 2);
    this->addPropertyLink(
      this->Ui->ymax, "text2", SIGNAL(textChangedAndEditingFinished()), refBounds, 3);
    this->addPropertyLink(
      this->Ui->zmin, "text2", SIGNAL(textChangedAndEditingFinished()), refBounds, 4);
    this->addPropertyLink(
      this->Ui->zmax, "text2", SIGNAL(textChangedAndEditingFinished()), refBounds, 5);
  }
  else
  {
    this->Ui->referenceBoundsLabel->hide();
    this->Ui->referenceBoundsHLine->hide();
    this->Ui->useReferenceBounds->hide();
    this->Ui->xmin->hide();
    this->Ui->xmax->hide();
    this->Ui->ymin->hide();
    this->Ui->ymax->hide();
    this->Ui->zmin->hide();
    this->Ui->zmax->hide();

    // if `ReferenceBounds` or `UseReferenceBounds` is not present,
    // the box is either using the input bounds as reference (e.g. Transform) or
    // isn't i.e relative to unit box.  To determine that, let's if "Input" is part
    // of the property group.
    this->BoxIsRelativeToInput = (smgroup->GetProperty("Input") != nullptr);
    vtkSMPropertyHelper(wdgProxy, "UseReferenceBounds").Set(0);
    wdgProxy->UpdateVTKObjects();
  }

  if (hideReferenceBounds)
  {
    this->Ui->referenceBoundsLabel->hide();
    this->Ui->referenceBoundsHLine->hide();
    this->Ui->useReferenceBounds->hide();
    this->Ui->xmin->hide();
    this->Ui->xmax->hide();
    this->Ui->ymin->hide();
    this->Ui->ymax->hide();
    this->Ui->zmin->hide();
    this->Ui->zmax->hide();
  }

  this->connect(&this->WidgetLinks, SIGNAL(qtWidgetChanged()), SLOT(render()));
  // When any SM property linked through pqPropertyWidget::links() changes,
  // update the matrix text if currently on the Matrix tab.
  this->connect(&this->links(), SIGNAL(smPropertyChanged()), SLOT(onSMPropertiesChanged()));

  // link show3DWidget checkbox
  this->connect(this->Ui->show3DWidget, SIGNAL(toggled(bool)), SLOT(setWidgetVisible(bool)));
  this->Ui->show3DWidget->connect(
    this, SIGNAL(widgetVisibilityToggled(bool)), SLOT(setChecked(bool)));
  this->setWidgetVisible(this->Ui->show3DWidget->isChecked());

  QObject::connect(this->Ui->resetBounds, &QAbstractButton::clicked,
    [this, wdgProxy](bool)
    {
      auto bbox = this->dataBounds(this->Ui->visibleBoundsOnly->isChecked());
      if (!bbox.IsValid())
      {
        return;
      }
      if (this->BoxIsRelativeToInput ||
        vtkSMUncheckedPropertyHelper(wdgProxy, "UseReferenceBounds").GetAsInt() == 1)
      {
        double bds[6];
        bbox.GetBounds(bds);
        vtkSMPropertyHelper(wdgProxy, "ReferenceBounds").Set(bds, 6);

        const double scale[3] = { 1, 1, 1 };
        vtkSMPropertyHelper(wdgProxy, "Scale").Set(scale, 3);

        const double pos[3] = { 0, 0, 0 };
        vtkSMPropertyHelper(wdgProxy, "Position").Set(pos, 3);

        const double orient[3] = { 0, 0, 0 };
        vtkSMPropertyHelper(wdgProxy, "Rotation").Set(orient, 3);
      }
      else
      {
        double bds[6] = { 0, 1, 0, 1, 0, 1 };
        vtkSMPropertyHelper(wdgProxy, "ReferenceBounds").Set(bds, 6);

        double lengths[3];
        bbox.GetLengths(lengths);
        vtkSMPropertyHelper(wdgProxy, "Scale").Set(lengths, 3);

        const double orient[3] = { 0, 0, 0 };
        vtkSMPropertyHelper(wdgProxy, "Rotation").Set(orient, 3);

        vtkSMPropertyHelper(wdgProxy, "Position").Set(bbox.GetMinPoint(), 3);
      }
      wdgProxy->UpdateVTKObjects();
      Q_EMIT this->changeAvailable();
      if (this->PlaceWidgetConnection)
      {
        QObject::disconnect(this->PlaceWidgetConnection);
      }
      this->PlaceWidgetConnection = QObject::connect(&pqActiveObjects::instance(),
        &pqActiveObjects::dataUpdated, this, &pqBoxPropertyWidget::placeWidget);
      this->render();
    });

  // Ensure fields page is selected by default
  this->Ui->tabWidget->setCurrentWidget(this->Ui->fieldsPage);
  this->connect(this->Ui->tabWidget, SIGNAL(currentChanged(int)), SLOT(onTabChanged(int)));
  this->connect(this->Ui->matrixEdit, SIGNAL(textChanged()), SLOT(matrixTextEdited()));

  // Disable matrix tab if required properties are missing (need Position, Rotation, Scale)
  if (!this->Position || !this->Scale || !this->Rotation)
  {
    int idx = this->Ui->tabWidget->indexOf(this->Ui->matrixPage);
    if (idx >= 0)
    {
      this->Ui->tabWidget->setTabEnabled(idx, false);
    }
  }
}

//-----------------------------------------------------------------------------
pqBoxPropertyWidget::~pqBoxPropertyWidget() = default;

//-----------------------------------------------------------------------------
void pqBoxPropertyWidget::apply()
{
  // If currently in matrix tab, push current matrix text into unchecked properties first.
  if (this->Ui->tabWidget->currentWidget() == this->Ui->matrixPage)
  {
    double matrixData[16];
    if (this->Ui->matrixEdit && ::parseMatrixText(this->Ui->matrixEdit->toPlainText(), matrixData))
    {
      double pos[3] = { 0, 0, 0 };
      double rot[3] = { 0, 0, 0 };
      double scl[3] = { 1, 1, 1 };
      ::decomposeMatrixToPRS(matrixData, pos, rot, scl);
      vtkSMUncheckedPropertyHelper(this->Position).Set(pos, 3);
      vtkSMUncheckedPropertyHelper(this->Rotation).Set(rot, 3);
      vtkSMUncheckedPropertyHelper(this->Scale).Set(scl, 3);
    }
  }

  this->Superclass::apply();
}

//-----------------------------------------------------------------------------
void pqBoxPropertyWidget::placeWidget()
{
  auto wdgProxy = this->widgetProxy();
  if (this->BoxIsRelativeToInput || vtkSMPropertyHelper(wdgProxy, "UseReferenceBounds").GetAsInt())
  {
    auto bbox = this->dataBounds(this->Ui->visibleBoundsOnly->isChecked());
    if (bbox.IsValid())
    {
      double bds[6];
      bbox.GetBounds(bds);

      vtkSMPropertyHelper(wdgProxy, "PlaceWidget").Set(bds, 6);
      wdgProxy->UpdateVTKObjects();
    }
  }
  else
  {
    double bds[6] = { 0, 1, 0, 1, 0, 1 };
    vtkSMPropertyHelper(wdgProxy, "PlaceWidget").Set(bds, 6);
    wdgProxy->UpdateVTKObjects();
  }
}

//-----------------------------------------------------------------------------
void pqBoxPropertyWidget::onTabChanged(int index)
{
  if (this->Ui->tabWidget->widget(index) == this->Ui->matrixPage)
  {
    // When switching to matrix mode, compose from current PRS values
    double pos[3] = { 0, 0, 0 };
    double rot[3] = { 0, 0, 0 };
    double scl[3] = { 1, 1, 1 };
    ::readPRSFromProperties(this->Position, this->Rotation, this->Scale, pos, rot, scl);
    double matrixData[16];
    ::composeMatrixFromPRS(pos, rot, scl, matrixData);
    // show in text
    QString txt;
    for (int rId = 0; rId < 4; ++rId)
    {
      for (int cId = 0; cId < 4; ++cId)
      {
        txt += QString::number(matrixData[rId * 4 + cId], 'g', 16);
        if (cId < 3)
        {
          txt += QLatin1String(" ");
        }
      }
      if (rId < 3)
      {
        txt += QLatin1String("\n");
      }
    }
    bool oldSignalState = this->Ui->matrixEdit->blockSignals(true);
    this->Ui->matrixEdit->setPlainText(txt);
    this->Ui->matrixEdit->blockSignals(oldSignalState);
  }
  else
  {
    // Map current matrix text back to properties at the unchecked level
    double matrixData[16];
    if (::parseMatrixText(this->Ui->matrixEdit->toPlainText(), matrixData))
    {
      bool changed = false;
      double pos[3], rot[3], scl[3];
      ::decomposeMatrixToPRS(matrixData, pos, rot, scl);
      vtkSMUncheckedPropertyHelper(this->Position).Set(pos, 3);
      vtkSMUncheckedPropertyHelper(this->Rotation).Set(rot, 3);
      vtkSMUncheckedPropertyHelper(this->Scale).Set(scl, 3);
      changed = true;
      if (changed)
      {
        Q_EMIT this->changeAvailable();
      }
    }
  }
}

//-----------------------------------------------------------------------------
void pqBoxPropertyWidget::fillMatrixTextFromProperties()
{
  double matrixData[16] = { 0 };
  bool haveMatrix = false;
  double pos[3] = { 0, 0, 0 };
  double rot[3] = { 0, 0, 0 };
  double scl[3] = { 1, 1, 1 };
  ::readPRSFromProperties(this->Position, this->Rotation, this->Scale, pos, rot, scl);
  ::composeMatrixFromPRS(pos, rot, scl, matrixData);
  haveMatrix = true;

  if (haveMatrix)
  {
    QString txt;
    for (int row = 0; row < 4; ++row)
    {
      for (int col = 0; col < 4; ++col)
      {
        txt += QString::number(matrixData[row * 4 + col], 'g', 16);
        if (col < 3)
        {
          txt += QLatin1String(" ");
        }
      }
      if (row < 3)
      {
        txt += QLatin1String("\n");
      }
    }
    bool oldSignalState = this->Ui->matrixEdit->blockSignals(true);
    this->Ui->matrixEdit->setPlainText(txt);
    this->Ui->matrixEdit->blockSignals(oldSignalState);
  }
  else
  {
    vtkGenericWarningMacro(<< "InteractiveBox Matrix: cannot compose matrix because required"
                           << " properties are missing for proxy "
                           << (this->proxy() ? this->proxy()->GetXMLName() : "(null)"));
  }
}

//-----------------------------------------------------------------------------
void pqBoxPropertyWidget::matrixTextEdited()
{
  Q_EMIT this->changeAvailable();
}

//-----------------------------------------------------------------------------
void pqBoxPropertyWidget::onSMPropertiesChanged()
{
  if (this->Ui->tabWidget->currentWidget() == this->Ui->matrixPage)
  {
    this->fillMatrixTextFromProperties();
  }
}
