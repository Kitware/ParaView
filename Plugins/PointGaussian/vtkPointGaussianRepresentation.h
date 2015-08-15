#ifndef VTKPOINTGAUSSIANREPRESENTATION_H
#define VTKPOINTGAUSSIANREPRESENTATION_H

#include "vtkPVDataRepresentation.h"
#include "vtkWeakPointer.h" // for weak pointer
#include "vtkSmartPointer.h" // for smart pointer

class vtkActor;
class vtkPointGaussianMapper;
class vtkScalarsToColors;
class vtkPolyData;
class vtkPiecewiseFunction;

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

  //***************************************************************************
  // Forwarded to Actor.
  virtual void SetOrientation(double, double, double);
  virtual void SetOrigin(double, double, double);
  virtual void SetPickable(int val);
  virtual void SetPosition(double, double, double);
  virtual void SetScale(double, double, double);

  //***************************************************************************
  // Forwarded to Actor->GetProperty()
  virtual void SetAmbientColor(double r, double g, double b);
  virtual void SetColor(double r, double g, double b);
  virtual void SetDiffuseColor(double r, double g, double b);
  virtual void SetEdgeColor(double r, double g, double b);
  virtual void SetInterpolation(int val);
  virtual void SetLineWidth(double val);
  virtual void SetOpacity(double val);
  virtual void SetPointSize(double val);
  virtual void SetSpecularColor(double r, double g, double b);
  virtual void SetSpecularPower(double val);

  // Description:
  // Sets the radius of the gaussian splats if there is no scale array or if
  // the scale array is disabled.  Defaults to 1.
  virtual void SetSplatSize(double radius);

  // Description:
  // Sets the snippet of fragment shader code used to color the sprites.
  void SetCustomShader(const char* shaderString);

  // Description:
  // Sets the point array to scale the guassians by.  The array should be a
  // float array.  The first four parameters are unused and only needed for
  // the ParaView GUI's signature recognition.
  void SelectScaleArray(int, int, int, int, const char* name);

  // Description:
  // Sets a vtkPiecewiseFunction to use in mapping array values to sprite
  // sizes.  Performance decreases (along with understandability) when
  // large values are used for sprite sizes.  This is only used when
  // "SetScaleArray" is also set.
  void SetScaleTransferFunction(vtkPiecewiseFunction* pwf);

  // Description:
  // Sets a vtkPiecewiseFunction to use in mapping array values to sprite
  // opacities.  Only used when "Opacity Array" is set.
  void SetOpacityTransferFunction(vtkPiecewiseFunction* pwf);

  // Description:
  // Sets the point array to use in calculating point sprite opacities.
  // The array should be a float or double array.  The first four
  // parameters are unused and only needed for the ParaView GUI's
  // signature recognition.
  void SelectOpacityArray(int, int, int, int, const char* name);

  // Description:
  // Enables or disables setting opacity by an array.  Set which array
  // should be used for opacity with SelectOpacityArray, and set an
  // opacity transfer function with SetOpacityTransferFunction.
  void SetOpacityByArray(bool newVal);
  vtkGetMacro(OpacityByArray, bool);
  vtkBooleanMacro(OpacityByArray, bool);

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

  bool OpacityByArray;
  char* LastOpacityArray;

  vtkSetStringMacro(LastOpacityArray);
};

#endif // VTKPOINTGAUSSIANREPRESENTATION_H
