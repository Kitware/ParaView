// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqONNXPropertyGroupWidget.h"

#include "pqONNXInputParameter.h"
#include "pqONNXJsonVerify.h"
#include "pqONNXOutputParameters.h"
#include "pqONNXTimeParameter.h"

#include "pqDoublePropertyMultiWidgets.h"
#include "pqFileChooserWidget.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMStringVectorProperty.h"

#include <QFile>
#include <QFileInfo>
#include <QGridLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QVBoxLayout>

namespace XMLNames
{
static const char* ParametersFile = "ParametersFile";
static const char* InputParameters = "InputParameters";
static const char* TimeValues = "TimeValues";
static const char* TimeStepIndex = "TimeStepIndex";
static const char* OnCellData = "OnCellData";
static const char* OutputDimension = "OutputDimension";
}

namespace JsonNames
{
static const char* Input = "Input";
static const char* Output = "Output";
static const char* Version = "Version";
}

//-----------------------------------------------------------------------------
pqONNXPropertyGroupWidget::pqONNXPropertyGroupWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parent)
  : Superclass(smproxy, parent)
{
  auto layout = new QGridLayout(this);
  this->setLayout(layout);

  vtkSMStringVectorProperty* jsonProp =
    vtkSMStringVectorProperty::SafeDownCast(smgroup->GetProperty(XMLNames::ParametersFile));

  auto fileWidget = new pqFileChooserWidget(this);
  fileWidget->setTitle(jsonProp->GetXMLLabel());
  fileWidget->forceSingleFile();

  auto label = new QLabel(jsonProp->GetXMLLabel(), this);
  layout->addWidget(label, 0, 0);
  layout->addWidget(fileWidget, 0, 1);

  this->setChangeAvailableAsChangeFinished(true);

  vtkSMDoubleVectorProperty* inputParametersProp =
    vtkSMDoubleVectorProperty::SafeDownCast(smgroup->GetProperty(XMLNames::InputParameters));
  if (!inputParametersProp)
  {
    qWarning() << "Parameters property is required for the ONNX inference proxy";
  }

  this->MultiWidget = new pqDoublePropertyMultiWidgets(this, inputParametersProp);
  layout->addWidget(this->MultiWidget, 1, 0, 1, 2);

  auto inputPropHelper = vtkSMPropertyHelper(inputParametersProp);
  std::vector<double> values = inputPropHelper.GetDoubleArray();

  this->connect(this->MultiWidget, &pqDoublePropertyMultiWidgets::changeAvailable, this,
    &pqONNXPropertyGroupWidget::changeAvailable);

  this->connect(fileWidget, &pqFileChooserWidget::filenameChanged, this,
    &pqONNXPropertyGroupWidget::onFileChanged);

  this->addPropertyLink(this, "parametersFileName", SIGNAL(parametersFileNameChanged()), jsonProp);

  // parametersFileName requires MultiWidget instance to exist but repopulates it.
  // so we need to manually reset the multi widget to keep statefiles values.
  if (!values.empty())
  {
    QVariantList variants;
    for (const auto& val : values)
    {
      variants << val;
    }
    this->MultiWidget->setValues(variants);
  }
}

//-----------------------------------------------------------------------------
void pqONNXPropertyGroupWidget::apply()
{
  this->MultiWidget->apply();
  this->Superclass::apply();
}

//-----------------------------------------------------------------------------
QString pqONNXPropertyGroupWidget::getParametersFileName() const
{
  auto fileChooser = this->findChild<pqFileChooserWidget*>();
  if (fileChooser)
  {
    return fileChooser->singleFilename();
  }

  return QString();
}

//-----------------------------------------------------------------------------
void pqONNXPropertyGroupWidget::setParametersFileName(const QString& filePath)
{
  if (filePath.isEmpty() || !QFileInfo::exists(filePath))
  {
    return;
  }

  auto fileChooser = this->findChild<pqFileChooserWidget*>();
  if (!fileChooser)
  {
    return;
  }

  fileChooser->setSingleFilename(filePath);
  this->parseJson(filePath);
  this->updateFromJson();
}

//-----------------------------------------------------------------------------
void pqONNXPropertyGroupWidget::onFileChanged(const QString& path)
{
  this->parseJson(path);
  this->updateFromJson();

  Q_EMIT this->parametersFileNameChanged();
  Q_EMIT this->changeAvailable();
}

//-----------------------------------------------------------------------------
void pqONNXPropertyGroupWidget::parseJson(const QString& path)
{
  QFile parametersFile(path);
  if (!parametersFile.open(QIODevice::ReadOnly))
  {
    qWarning() << "Unable to read JSON file at: " << path;
    return;
  }

  const QString fileContent = parametersFile.readAll();
  this->ParametersDoc = QJsonDocument::fromJson(fileContent.toUtf8());

  if (this->ParametersDoc.isNull())
  {
    qWarning() << "Empty JSON";
  }
}

//-----------------------------------------------------------------------------
void pqONNXPropertyGroupWidget::updateFromJson()
{
  QJsonObject root = this->ParametersDoc.object();

  if (!pqONNXJsonVerify::check(root))
  {
    qWarning() << "Invalid json strucure, aborting.";
    return;
  }

  if (!pqONNXJsonVerify::isSupportedVersion(root[JsonNames::Version].toObject()))
  {
    qWarning() << "Configuration is in an unsupported version, aborting.";
    return;
  }

  this->MultiWidget->clear();

  QJsonArray jsonInputArray = root[JsonNames::Input].toArray();
  int parameterIndex = 0;
  for (const auto& inputJsonValue : jsonInputArray)
  {
    parameterIndex++;
    QJsonObject dataObject = inputJsonValue.toObject();
    if (pqONNXJsonVerify::isTime(dataObject))
    {
      pqONNXTimeParameter param(dataObject);
      this->setupTimeValues(param, parameterIndex);
      this->MultiWidget->addWidget(&param);
      this->MultiWidget->hideParameterWidgets(param.getName());
    }
    else
    {
      pqONNXInputParameter param(dataObject);
      this->MultiWidget->addWidget(&param);
    }
  }

  QJsonObject outputJson = root[JsonNames::Output].toObject();
  pqONNXOutputParameters outputParam(outputJson);
  this->setupOutputProperties(outputParam);
}

//-----------------------------------------------------------------------------
void pqONNXPropertyGroupWidget::setupTimeValues(
  const pqONNXTimeParameter& param, int parameterIndex)
{

  vtkSMPropertyHelper timeProp(this->proxy()->GetProperty(XMLNames::TimeValues));
  timeProp.RemoveAllValues();
  timeProp.Append(param.getTimes().data(), param.getTimes().size());

  vtkSMPropertyHelper timeIndexProp(this->proxy()->GetProperty(XMLNames::TimeStepIndex));
  timeIndexProp.Set(parameterIndex);
}

//-----------------------------------------------------------------------------
void pqONNXPropertyGroupWidget::setupOutputProperties(const pqONNXOutputParameters& outParameters)
{
  vtkSMPropertyHelper onCellProp(this->proxy()->GetProperty(XMLNames::OnCellData));
  onCellProp.Set(outParameters.getOnCell());

  vtkSMPropertyHelper dimensionProp(this->proxy()->GetProperty(XMLNames::OutputDimension));
  dimensionProp.Set(outParameters.getDimension());
}
