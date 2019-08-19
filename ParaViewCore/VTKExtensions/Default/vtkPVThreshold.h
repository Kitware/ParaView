/*=========================================================================

  program:   paraview
  module:    vtkpvthreshold.h

  copyright (c) kitware, inc.
  all rights reserved.
  see copyright.txt or http://www.paraview.org/html/copyright.html for details.

     this software is distributed without any warranty; without even
     the implied warranty of merchantability or fitness for a particular
     purpose.  see the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkpvthreshold
 * @brief   threshold filter
 *
 *
 * This is a subclass of vtkAlgorithm that allows to apply threshold filters
 * to either vtkDataSet or vtkHyperTreeGrid.
*/

#ifndef vtkPVThreshold_h
#define vtkPVThreshold_h

#include "vtkAlgorithm.h"
#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkThreshold.h"

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkPVThreshold : public vtkAlgorithm
{
public:
  vtkTypeMacro(vtkPVThreshold, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPVThreshold* New();

  int ProcessRequest(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  enum
  {
    THRESHOLD_BY_LOWER,
    THRESHOLD_BY_UPPER,
    THRESHOLD_BETWEEN
  };

  //@{
  /**
   * Get the Upper and Lower thresholds.
   */
  vtkGetMacro(UpperThreshold, double);
  vtkGetMacro(LowerThreshold, double);
  //@}

  //@{
  /**
   * Control how the filter works with scalar point data and cell attribute
   * data.  By default (AttributeModeToDefault), the filter will use point
   * data, and if no point data is available, then cell data is
   * used. Alternatively you can explicitly set the filter to use point data
   * (AttributeModeToUsePointData) or cell data (AttributeModeToUseCellData).
   */
  vtkSetMacro(AttributeMode, int);
  vtkGetMacro(AttributeMode, int);
  void SetAttributeModeToDefault() { this->SetAttributeMode(VTK_ATTRIBUTE_MODE_DEFAULT); };
  void SetAttributeModeToUsePointData()
  {
    this->SetAttributeMode(VTK_ATTRIBUTE_MODE_USE_POINT_DATA);
  };
  void SetAttributeModeToUseCellData()
  {
    this->SetAttributeMode(VTK_ATTRIBUTE_MODE_USE_CELL_DATA);
  };
  const char* GetAttributeModeAsString();
  //@}

  //@{
  /**
   * Control how the decision of in / out is made with multi-component data.
   * The choices are to use the selected component (specified in the
   * SelectedComponent ivar), or to look at all components. When looking at
   * all components, the evaluation can pass if all the components satisfy
   * the rule (UseAll) or if any satisfy is (UseAny). The default value is
   * UseSelected.
   */
  vtkSetClampMacro(ComponentMode, int, VTK_COMPONENT_MODE_USE_SELECTED, VTK_COMPONENT_MODE_USE_ANY);
  vtkGetMacro(ComponentMode, int);
  void SetComponentModeToUseSelected() { this->SetComponentMode(VTK_COMPONENT_MODE_USE_SELECTED); };
  void SetComponentModeToUseAll() { this->SetComponentMode(VTK_COMPONENT_MODE_USE_ALL); };
  void SetComponentModeToUseAny() { this->SetComponentMode(VTK_COMPONENT_MODE_USE_ANY); };
  const char* GetComponentModeAsString();
  //@}

  //@{
  /**
   * When the component mode is UseSelected, this ivar indicated the selected
   * component. The default value is 0.
   */
  vtkSetClampMacro(SelectedComponent, int, 0, VTK_INT_MAX);
  vtkGetMacro(SelectedComponent, int);
  //@}

  //@{
  /**
   * If using scalars from point data, all scalars for all points in a cell
   * must satisfy the threshold criterion if AllScalars is set. Otherwise,
   * just a single scalar value satisfying the threshold criterion enables
   * will extract the cell.
   */
  vtkSetMacro(AllScalars, vtkTypeBool);
  vtkGetMacro(AllScalars, vtkTypeBool);
  vtkBooleanMacro(AllScalars, vtkTypeBool);
  //@}

  //@{
  /**
   * If this is on (default is off), we will use the continuous interval
   * [minimum cell scalar, maxmimum cell scalar] to intersect the threshold bound
   * , rather than the set of discrete scalar values from the vertices
   * *WARNING*: For higher order cells, the scalar range of the cell is
   * not the same as the vertex scalar interval used here, so the
   * result will not be accurate.
   */
  vtkSetMacro(UseContinuousCellRange, vtkTypeBool);
  vtkGetMacro(UseContinuousCellRange, vtkTypeBool);
  vtkBooleanMacro(UseContinuousCellRange, vtkTypeBool);
  //@}

  //@{
  /**
   * Invert the threshold results. That is, cells that would have been in the output with this
   * option off are excluded, while cells that would have been excluded from the output are
   * included.
   */
  vtkSetMacro(Invert, bool);
  vtkGetMacro(Invert, bool);
  vtkBooleanMacro(Invert, bool);
  //@}

  //@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  void SetOutputPointsPrecision(int precision)
  {
    this->OutputPointsPrecision = precision;
    this->Modified();
  }
  int GetOutputPointsPrecision() const;
  //@}

  /**
   * Criterion is cells whose scalars are between lower and/or upper thresholds
   * (inclusive of the end values).
   */
  void ThresholdBetween(double lower, double upper);
  void ThresholdByLower(double lower);
  void ThresholdByUpper(double upper);

  void SetInputData(vtkDataObject* input);

protected:
  vtkPVThreshold();
  virtual ~vtkPVThreshold() override;

  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  virtual int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  int FillInputPortInformation(int, vtkInformation*) override;
  int FillOutputPortInformation(int, vtkInformation*) override;

  vtkTypeBool AllScalars;
  double LowerThreshold;
  double UpperThreshold;
  int AttributeMode;
  int ComponentMode;
  int SelectedComponent;
  int OutputPointsPrecision;
  vtkTypeBool UseContinuousCellRange;
  bool Invert;
  int ThresholdMethod;

private:
  vtkPVThreshold(const vtkPVThreshold&) = delete;
  void operator=(const vtkPVThreshold&) = delete;
};

#endif
