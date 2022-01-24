#ifndef vtkPVExtractCellsByType_h
#define vtkPVExtractCellsByType_h

#include "vtkPVVTKExtensionsExtractionModule.h" // For export macro

#include <vtkExtractCellsByType.h>
#include <vtkNew.h> // for vtkNew

class vtkDataArraySelection;

class VTKPVVTKEXTENSIONSEXTRACTION_EXPORT vtkPVExtractCellsByType : public vtkExtractCellsByType
{
public:
  static vtkPVExtractCellsByType* New();
  vtkTypeMacro(vtkPVExtractCellsByType, vtkExtractCellsByType);

  /**
   * Get the current selection of cell types for extraction.
   */
  vtkGetNewMacro(CellTypeSelection, vtkDataArraySelection);

protected:
  vtkPVExtractCellsByType();
  ~vtkPVExtractCellsByType() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkPVExtractCellsByType(const vtkPVExtractCellsByType&) = delete;
  void operator=(const vtkPVExtractCellsByType&) = delete;

  vtkNew<vtkDataArraySelection> CellTypeSelection;
};

#endif
