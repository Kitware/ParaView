
#ifndef vtkHyperTreeGridBox_h
#define vtkHyperTreeGridBox_h

#include <functional>
#include <vector>
#include <vtkAlgorithm.h>
#include <vtkFiltersGeneralModule.h> // For export macro

class vtkPointData;
class vtkBitArray;
class vtkDoubleArray;
class vtkDataSet;
class vtkHyperTreeGrid;
class vtkHyperTreeGridNonOrientedGeometryCursor;
class vtkPVMetaClipDataSet;
class vtkPointDataToCellData;

using vtkHyperTreeGridMetric = std::function<bool(vtkDataArray*)>;
using vtkHyperTreeGridValue = std::function<double(vtkDataArray*)>;
using vtkHyperTreeGridMask = std::function<int(vtkDataSet*)>;

class VTKFILTERSGENERAL_EXPORT vtkHyperTreeGridBox : public vtkAlgorithm
{
public:
  static vtkHyperTreeGridBox* New();
  vtkTypeMacro(vtkHyperTreeGridBox, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set/Get the subdivision factor in the grid refinement scheme.
   */
  vtkSetClampMacro(BranchFactor, unsigned int, 2, 3);
  vtkGetMacro(BranchFactor, unsigned int);
  //@}

  //@{
  /**
   * Set/Get the maximum tree depth.
   */
  vtkSetMacro(MaxTreeDepth, unsigned int);
  vtkGetMacro(MaxTreeDepth, unsigned int);
  //@}

  //@{
  /**
   * Get/Set the maximum grid size of the hyper tree grid. All further resolution is creaded by
   * hyper trees.
   * The default is [4, 4, 4].
   */
  vtkSetVector3Macro(Dimensions, unsigned int);
  vtkGetVector3Macro(Dimensions, unsigned int);
  //@}

  //@{
  /**
   * Only applicable if input data type is vtkImageData.
   * If enabled, the Dimensions will be calculated based on the MaxTreeDepth and the input
   * resolution.
   * The HyperTreeGrid will always match the input resolution. In Example, a 5x5x5 input image and a
   * MaxTreeDepth of 1 results in a rectiliniear grid of the HyperTreeGrid of 3x3x3. The axis will
   * be
   * like the bounds of the 4x4x4 subset of the input plus a smaller grid of the size of the input
   * resolution. Resulting in a rectilinear grid of 3x3x3 with a maximum tree depth of 1.
   */
  vtkGetMacro(CalculateDimensions, bool) vtkSetMacro(CalculateDimensions, bool)
    vtkBooleanMacro(CalculateDimensions, bool);
  //@}

  //@{
  /**
   * Enable/Disable the maximum metric. If enabled, set the MaxValue too.
   */
  vtkGetMacro(UseMax, bool) vtkSetMacro(UseMax, bool) vtkBooleanMacro(UseMax, bool);
  //@}

  //@{
  /**
   * Set/Get the threshold using a metric.
   */
  vtkGetMacro(MaxValue, double);
  vtkSetMacro(MaxValue, double);
  //@}

  //@{
  /**
   * Enable/Disable the minimum metric. If enabled, set the MinValue too.
   */
  vtkGetMacro(UseMin, bool) vtkSetMacro(UseMin, bool) vtkBooleanMacro(UseMin, bool);
  //@}

  //@{
  /**
   * Set/Get the threshold using a metric.
   */
  vtkGetMacro(MinValue, double);
  vtkSetMacro(MinValue, double);
  //@}

  //@{
  /**
   * Enable/Disable the entropy metric. If enabled, set the EntropyValue too.
   */
  vtkGetMacro(UseEntropy, bool) vtkSetMacro(UseEntropy, bool) vtkBooleanMacro(UseEntropy, bool);
  //@}

  //@{
  /**
   * Set/Get the threshold using a metric.
   */
  vtkGetMacro(EntropyValue, double);
  vtkSetMacro(EntropyValue, double);
  //@}

  //@{
  /**
   * Enable/Disable the range metric. If enabled, set the RangeValue too.
   */
  vtkGetMacro(UseRange, bool) vtkSetMacro(UseRange, bool) vtkBooleanMacro(UseRange, bool);
  //@}

  //@{
  /**
   * Set/Get the threshold using a metric.
   */
  vtkGetMacro(RangeValue, double);
  vtkSetMacro(RangeValue, double);
  //@}

  vtkDataObject* GetOutput();

  inline void SetMetricFunction(vtkHyperTreeGridMetric metric) { this->MetricFunction = metric; };
  inline vtkHyperTreeGridMetric GetMetricFunction() { return this->MetricFunction; };

  inline void SetValueFunction(vtkHyperTreeGridValue value) { this->ValueFunction = value; };
  inline vtkHyperTreeGridValue GetValueFunction() { return this->ValueFunction; };

  inline void SetMaskFunction(vtkHyperTreeGridMask mask) { this->MaskFunction = mask; };
  inline vtkHyperTreeGridMask GetMaskFunction() { return this->MaskFunction; };

protected:
  vtkHyperTreeGridBox();
  ~vtkHyperTreeGridBox() override;

  // Override to specify support for any vtkDataSet input type.
  int FillInputPortInformation(int, vtkInformation*) override;
  // Override to specify output as vtkHyperTreeGrid.
  int FillOutputPortInformation(int, vtkInformation*) override;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  // Main implementation.
  int ProcessRequest(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  // unsigned int CalculateDimensions(unsigned int, unsigned int);
  void AddAxis(vtkDoubleArray*, unsigned int, double[2], unsigned int);

  int GenerateTrees(vtkHyperTreeGrid*, vtkDataObject*);

  void SubdivideLeavesTillLevel(vtkHyperTreeGridNonOrientedGeometryCursor*);
  bool ShouldSubdivide(double[6]);
  /**
   * If at least one child boundary would be mask and at least one not, we are going closer to the
   *geometry (return true).
   * If all child boundaries contain geometry, return false.
   **/
  bool IsSubdivisionCloserToGeometry(double[6]);

  bool IsNodeWithinGeometry(vtkDataSet*, double[6]);
  void AddInterface(vtkIdType, vtkDataSet*, double[6]);

  double* GetChildBounds(double[6], vtkIdType);
  vtkDataSet* GetSubdividedData(double[6], int);

  vtkDataArray* GetDataObject(vtkDataSet*, vtkIdType);

  double Mean(vtkDataArray*);

  bool UseMax;
  double MaxValue;
  bool Max(vtkDataArray*, double);

  bool UseMin;
  double MinValue;
  bool Min(vtkDataArray*, double);

  bool UseEntropy;
  double EntropyValue;
  bool Entropy(vtkDataArray*, double);

  bool UseRange;
  double RangeValue;
  bool Range(vtkDataArray*, double);

  int MaskImplementation(vtkDataSet*);
  bool MetricImplementation(vtkDataArray*);

  unsigned int BranchFactor;
  unsigned int Dimensions[3];
  bool CalculateDimensions;
  unsigned int MaxTreeDepth;

  vtkHyperTreeGridMetric MetricFunction;
  vtkHyperTreeGridValue ValueFunction;
  vtkHyperTreeGridMask MaskFunction;

  vtkDataSet* Input;
  double* InputBounds;
  int* InputExtend;

  vtkHyperTreeGrid* Output;

  vtkBitArray* Mask;
  bool MaskUsed;

  vtkDoubleArray* Intercepts;
  vtkDoubleArray* Normals;
  bool InterceptUsed;

  std::vector<vtkAlgorithm*> Extractor;
  vtkPointDataToCellData* PointDataToCellDataConverter;

private:
  vtkHyperTreeGridBox(const vtkHyperTreeGridBox&) = delete;
  void operator=(const vtkHyperTreeGridBox&) = delete;
};

#endif
