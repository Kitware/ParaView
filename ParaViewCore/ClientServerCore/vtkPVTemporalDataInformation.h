/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTemporalDataInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVTemporalDataInformation
// .SECTION Description
// vtkPVTemporalDataInformation is used to gather data information over time.
// This information provided by this class is a sub-set of vtkPVDataInformation
// and hence this is not directly a subclass of vtkPVDataInformation. It
// internally uses vtkPVDataInformation to collect information about each
// timestep.

#ifndef __vtkPVTemporalDataInformation_h
#define __vtkPVTemporalDataInformation_h

#include "vtkPVInformation.h"

class vtkPVDataSetAttributesInformation;

class VTK_EXPORT vtkPVTemporalDataInformation : public vtkPVInformation
{
public:
  static vtkPVTemporalDataInformation* New();
  vtkTypeMacro(vtkPVTemporalDataInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Port number controls which output port the information is gathered from.
  // This is only applicable when the vtkObject from which the information being
  // gathered is a vtkAlgorithm. Set it to -1(default), to return the classname
  // of the vtkAlgorithm itself.
  // This is the only parameter that can be set on  the client-side before
  // gathering the information.
  vtkSetMacro(PortNumber, int);

  // Description:
  // Transfer information about a single object into this object.
  // This expects the \c object to be a vtkAlgorithmOutput.
  virtual void CopyFromObject(vtkObject* object);

  // Description:
  // Merge another information object. Calls AddInformation(info, 0).
  virtual void AddInformation(vtkPVInformation* info);

  // Description:
  // Manage a serialized version of the information.
  virtual void CopyToStream(vtkClientServerStream*);
  virtual void CopyFromStream(const vtkClientServerStream*);

  // Description:
  // Serialize/Deserialize the parameters that control how/what information is
  // gathered. This are different from the ivars that constitute the gathered
  // information itself. For example, PortNumber on vtkPVDataInformation
  // controls what output port the data-information is gathered from.
  virtual void CopyParametersToStream(vtkMultiProcessStream&);
  virtual void CopyParametersFromStream(vtkMultiProcessStream&);

  // Description:
  // Initializes the information object.
  void Initialize();

  // Description:
  // Returns the number of timesteps this information was gathered from.
  vtkGetMacro(NumberOfTimeSteps, int);

  // Description:
  // Returns the time-range this information was gathered from.
  vtkGetVector2Macro(TimeRange, double);

  // Description:
  // Access to information about point/cell/vertex/edge/row data.
  vtkGetObjectMacro(PointDataInformation, vtkPVDataSetAttributesInformation);
  vtkGetObjectMacro(CellDataInformation, vtkPVDataSetAttributesInformation);
  vtkGetObjectMacro(VertexDataInformation, vtkPVDataSetAttributesInformation);
  vtkGetObjectMacro(EdgeDataInformation, vtkPVDataSetAttributesInformation);
  vtkGetObjectMacro(RowDataInformation, vtkPVDataSetAttributesInformation);

  // Description:
  // Convenience method to get the attribute information given the attribute
  // type. attr can be vtkDataObject::FieldAssociations or
  // vtkDataObject::AttributeTypes (since both are identical).
  vtkPVDataSetAttributesInformation* GetAttributeInformation(int attr);

  // Description:
  // Access to information about field data, if any.
  vtkGetObjectMacro(FieldDataInformation,vtkPVDataSetAttributesInformation);

//BTX
protected:
  vtkPVTemporalDataInformation();
  ~vtkPVTemporalDataInformation();

  vtkPVDataSetAttributesInformation* PointDataInformation;
  vtkPVDataSetAttributesInformation* CellDataInformation;
  vtkPVDataSetAttributesInformation* FieldDataInformation;
  vtkPVDataSetAttributesInformation* VertexDataInformation;
  vtkPVDataSetAttributesInformation* EdgeDataInformation;
  vtkPVDataSetAttributesInformation* RowDataInformation;

  double TimeRange[2];
  int NumberOfTimeSteps;
  int PortNumber;
private:
  vtkPVTemporalDataInformation(const vtkPVTemporalDataInformation&); // Not implemented
  void operator=(const vtkPVTemporalDataInformation&); // Not implemented
//ETX
};

#endif

