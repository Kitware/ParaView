// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqONNXPropertyGroupWidget_h
#define pqONNXPropertyGroupWidget_h

#include <pqPropertyWidget.h>

#include <QJsonDocument>
#include <QList>
#include <QVariant>

class pqDoublePropertyMultiWidgets;
class pqONNXInputParameter;
class pqONNXOutputParameters;
class pqONNXTimeParameter;
class pqScalarValueListPropertyWidget;
class vtkSMPropertyGroup;

/**
 * pqONNXPropertyGroupWidget is a property widget for ONNXPredict parameters.
 *
 * Based on an ONNX input file, this class can dynamically create a list of widgets,
 * one slider per parameter.
 *
 * It relies on the following XML properties:
 * - ParametersFile: a string property, path to a Json file describing parameters (see pqJsonVerify)
 * - InputParameters: a double vector property. The list of parameters to forward to ONNX.
 * This is a dynamic property, in the sense that the number of elements comes from the
 * ParametersFile file content.
 * - TimeValues: a double vector property. The list of time values. Current time is one parameter
 * that will be forwarded to onnx.
 * - TimeStepIndex: int property. The index of the time value in the ONNX parameters list.
 * - OnCellData: a boolean. True if ONNX output should be added on cell data. False for point.
 * - OutputDimension: int property. The number of components for the output array.
 *
 * This creates the appropriate widgets for each parameter defined in the Json file.
 */
class pqONNXPropertyGroupWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;

  Q_PROPERTY(QString parametersFileName READ getParametersFileName WRITE setParametersFileName)

public:
  /**
   * Construct a Group Widget.
   * Instanciate widgets and link them to their properties.
   * Also connect the ParameterFiles property to generate widgets to
   * control the dynamic InputParameters property.
   */
  pqONNXPropertyGroupWidget(
    vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parent = nullptr);

  ~pqONNXPropertyGroupWidget() override = default;

  /**
   * Return the path of the file set in the file widget.
   */
  QString getParametersFileName() const;

  /**
   * Forwards call to internal property widgets.
   */
  void apply() override;

Q_SIGNALS:

  /**
   * Emited when the file name widget is update. Used for property link.
   */
  void parametersFileNameChanged();

  /**
   * Emited when a parameter widget is changed. Used for property link.
   */
  void inputParametersChanged();

private Q_SLOTS:
  /**
   * When file path is changed, read the new configuration
   * and setup widgets accordingly.
   */
  void onFileChanged(const QString& path);

  /**
   * Update the widgets from input path.
   * This should point a valid Json file. See pqONNXJsonVerify for the
   * expected structure.
   *
   * Create one widget per input parameter found in the Json.
   */
  void setParametersFileName(const QString& path);

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqONNXPropertyGroupWidget);

  /**
   * Open the file and construct an internal data structure.
   */
  void parseJson(const QString& path);

  /**
   * Using the JSON, create widgets and set TimeValues property.
   */
  void updateFromJson();

  /**
   * Set TimeValues and TimeStepIndex properties from JSON object.
   */
  void setupTimeValues(const pqONNXTimeParameter& param, int parameterIndex);

  /**
   * Set the diffenrent output properties.
   */
  void setupOutputProperties(const pqONNXOutputParameters& outputParameters);

  /**
   * Handle the Json representation of the parameter file.
   */
  QJsonDocument ParametersDoc;

  pqDoublePropertyMultiWidgets* MultiWidget;
};

#endif
