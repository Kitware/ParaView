/*=============================================================================
Copyright and License information
=============================================================================*/
/**
 * @class vtkPVExtractHistogram2D
 * @brief Extract 2D histogram for a parallel dataset
 *
 * vtkPVExtractHistogram2D is a vtkImageAlgorithm subclass for parallel datasets, to extract the 2D
 * histogram. It uses vtkExtractHistogram2D internally and gathers the histogram data on the root
 * node.
 */

#ifndef vtkPVExtractHistogram2D_h
#define vtkPVExtractHistogram2D_h

// VTK includes
#include "vtkImageAlgorithm.h"
#include "vtkPVVTKExtensionsMiscModule.h" // needed for exports
#include "vtkSmartPointer.h"              // needed for smart pointer

// Forward declarations
class vtkDataArray;
class vtkMultiProcessController;

class VTKPVVTKEXTENSIONSMISC_EXPORT vtkPVExtractHistogram2D : public vtkImageAlgorithm
{
public:
  /**
   * Instantiate the class.
   */
  static vtkPVExtractHistogram2D* New();

  ///@{
  /**
   * Standard methods for the VTK class.
   */
  vtkTypeMacro(vtkPVExtractHistogram2D, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Get/Set the multiprocess controller. If no controller is set,
   * single process is assumed.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

  ///@{
  /**
   * Set/Get the components of interest from the two arrays.
   * Component0 is the component from input array 0 and Component1 is the component from input array
   * 1. Set the input arrays using SetInputArrayToProcess.
   */
  vtkSetMacro(Component0, int);
  vtkGetMacro(Component0, int);
  vtkSetMacro(Component1, int);
  vtkGetMacro(Component1, int);
  void SetComponent(int id, int val) { (id == 0) ? SetComponent0(val) : SetComponent1(val); }
  int GetComponent(int id) { return ((id == 0) ? GetComponent0() : GetComponent1()); }
  ///@}

  ///@{
  /**
   * Set/get the number of bins to be used per dimension (x,y)
   */
  vtkSetVector2Macro(NumberOfBins, int);
  vtkGetVector2Macro(NumberOfBins, int);
  ///@}

  ///@{
  /**
   * Set/Get whether to use custom bin ranges.
   */
  vtkSetVector2Macro(CustomBinRanges0, double);
  vtkGetVector2Macro(CustomBinRanges0, double);
  vtkSetVector2Macro(CustomBinRanges1, double);
  vtkGetVector2Macro(CustomBinRanges1, double);
  vtkSetMacro(UseCustomBinRanges0, bool);
  vtkGetMacro(UseCustomBinRanges0, bool);
  vtkBooleanMacro(UseCustomBinRanges0, bool);
  vtkSetMacro(UseCustomBinRanges1, bool);
  vtkGetMacro(UseCustomBinRanges1, bool);
  vtkBooleanMacro(UseCustomBinRanges1, bool);
  ///@}

protected:
  vtkPVExtractHistogram2D();
  ~vtkPVExtractHistogram2D() override;

  // Helper members
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  void ComputeHistogram2D(vtkImageData* image);

  int Component0 = 0;
  int Component1 = 0;
  int NumberOfBins[2] = { 256, 256 };

  double CustomBinRanges0[2];
  double CustomBinRanges1[2];
  bool UseCustomBinRanges0 = false;
  bool UseCustomBinRanges1 = false;
  vtkMultiProcessController* Controller = nullptr;

  // Cache of internal array and range
  int ComponentIndexCache[2];
  vtkDataArray* ComponentArrayCache[2];
  double ComponentRangeCache[2][2];

  void InitializeCache();
  void GetInputArrays(vtkInformationVector**);
  void ComputeComponentRange();

private:
  vtkPVExtractHistogram2D(const vtkPVExtractHistogram2D&) = delete;
  void operator=(const vtkPVExtractHistogram2D) = delete;
};

#endif // vtkPVExtractHistogram2D_h
