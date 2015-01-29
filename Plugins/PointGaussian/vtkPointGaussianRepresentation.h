#ifndef VTKPOINTGAUSSIANREPRESENTATION_H
#define VTKPOINTGAUSSIANREPRESENTATION_H

#include "vtkPVDataRepresentation.h"
#include "vtkWeakPointer.h" // for weak pointer
#include "vtkSmartPointer.h" // for smart pointer

class vtkActor;
class vtkPointGaussianMapper;
class vtkScalarsToColors;

class vtkPointGaussianRepresentation : public vtkPVDataRepresentation
{
public:
  vtkTypeMacro(vtkPointGaussianRepresentation, vtkPVDataRepresentation)
  static vtkPointGaussianRepresentation* New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  virtual int ProcessViewRequest(vtkInformationRequestKey *request_type,
                                 vtkInformation *inInfo,
                                 vtkInformation *outInfo);
  // Description:
  // Set the input data arrays that this algorithm will process. Overridden to
  // pass the array selection to the mapper.
  virtual void SetInputArrayToProcess(int idx, int port, int connection,
    int fieldAssociation, const char *name);
  virtual void SetInputArrayToProcess(int idx, int port, int connection,
    int fieldAssociation, int fieldAttributeType)
    {
    this->Superclass::SetInputArrayToProcess(
      idx, port, connection, fieldAssociation, fieldAttributeType);
    }
  virtual void SetInputArrayToProcess(int idx, vtkInformation *info)
    {
    this->Superclass::SetInputArrayToProcess(idx, info);
    }
  virtual void SetInputArrayToProcess(int idx, int port, int connection,
                              const char* fieldAssociation,
                              const char* attributeTypeorName)
    {
    this->Superclass::SetInputArrayToProcess(idx, port, connection,
      fieldAssociation, attributeTypeorName);
    }
  // Description:
  // Use to set the color map for the data in this representation
  void SetLookupTable(vtkScalarsToColors* lut);

  // Description:
  // Use to set whether the data in this representation is visible or not
  virtual void SetVisibility(bool val);

  // Description:
  // Use to set the opactiy of the data in this representation
  virtual void SetOpacity(double val);

  // Description:
  // Override to set the ambient color of the dataset
  virtual void SetAmbientColor(double r, double g, double b);
  // Description:
  // Override to set the color of the dataset
  virtual void SetColor(double r, double g, double b);
  // Description:
  // Override to set the diffuse color of the dataset
  virtual void SetDiffuseColor(double r, double g, double b);
  // Description:
  // Override to set the specular color of the dataset
  virtual void SetSpecularColor(double r, double g, double b);

  // Description:
  // Sets the radius of the gaussian splats if there is no scale array or if
  // the scale array is disabled.  Defaults to 1.
  virtual void SetSplatSize(double radius);

  // Description:
  // Sets the point array to scale the guassians by.  The array should be a
  // float array.  The first four parameters are unused and only needed for
  // the ParaView GUI's signature recognition.
  void SelectScaleArray(int, int, int, int, const char* name);

  // Description:
  // Enables or disables scaling by a data array vs. a constant factor.  Set
  // which data array with SelectScaleArray and SetSplatSize.
  void SetScaleByArray(bool newVal);
  vtkGetMacro(ScaleByArray, bool);
  vtkBooleanMacro(ScaleByArray, bool);

protected:
  vtkPointGaussianRepresentation();
  virtual ~vtkPointGaussianRepresentation();

  virtual bool AddToView(vtkView *view);
  virtual bool RemoveFromView(vtkView *view);

  virtual int FillInputPortInformation(int port, vtkInformation* info);
  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);

  vtkSmartPointer< vtkActor > Actor;
  vtkSmartPointer< vtkPointGaussianMapper > Mapper;
  vtkSmartPointer< vtkPolyData > ProcessedData;

  bool ScaleByArray;
  char* LastScaleArray;

  vtkSetStringMacro(LastScaleArray);
};

#endif // VTKPOINTGAUSSIANREPRESENTATION_H
