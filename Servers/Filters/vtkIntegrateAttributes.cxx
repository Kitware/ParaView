/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIntegrateAttributes.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkIntegrateAttributes.h"
#include "vtkObjectFactory.h"
#include "vtkDataSet.h"
#include "vtkUnstructuredGrid.h"
#include "vtkPointData.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkCellType.h"
#include "vtkPolygon.h"
#include "vtkTriangle.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"


vtkCxxRevisionMacro(vtkIntegrateAttributes, "1.3");
vtkStandardNewMacro(vtkIntegrateAttributes);

//-----------------------------------------------------------------------------
vtkIntegrateAttributes::vtkIntegrateAttributes()
{
  this->IntegrationDimension = 0;
  this->Sum = 0.0;
  this->SumCenter[0] = this->SumCenter[1] = this->SumCenter[2] = 0.0;
  this->Controller = vtkMultiProcessController::GetGlobalController();
  if (this->Controller)
    {
    this->Controller->Register(this);
    }
}

//-----------------------------------------------------------------------------
vtkIntegrateAttributes::~vtkIntegrateAttributes()
{
  if (this->Controller)
    {
    this->Controller->Delete();
    this->Controller = 0;
    }
}

//-----------------------------------------------------------------------------
int vtkIntegrateAttributes::CompareIntegrationDimension(int dim)
{
  // higher dimension prevails
  if (this->IntegrationDimension < dim)
    { // Throw out results from lower dimension.
    this->Sum = 0;
    this->SumCenter[0] = this->SumCenter[1] = this->SumCenter[2] = 0.0;
    this->ZeroAttributes(this->GetOutput()->GetPointData());
    this->ZeroAttributes(this->GetOutput()->GetCellData());
    this->IntegrationDimension = dim;
    return 1;
    }
  // Skip this cell if we are inetrgrting a higher dimension.
  return (this->IntegrationDimension == dim);
}

//-----------------------------------------------------------------------------
void vtkIntegrateAttributes::Execute()
{
  vtkDataSet* input = this->GetInput();
  vtkUnstructuredGrid* output = this->GetOutput();
  vtkIdType numCells, cellId;
  int cellType;
  vtkIdList* cellPtIds = vtkIdList::New();
  vtkDataArray* ghostLevelArray = 0;
    
  this->IntegrationDimension = 0;
  ghostLevelArray = input->GetCellData()->GetArray("vtkGhostLevels");  
    
  // Output will have all the asm attribute arrays as input, but
  // only 1 entry per array, and arrays are double.
  // Set all values to 0.  All output attributes are type double.
  this->AllocateAttributes(input->GetPointData(), output->GetPointData());
  this->AllocateAttributes(input->GetCellData(), output->GetCellData());
  
  // Integration of imaginary attribute with constant value 1.
  this->Sum = 0;
  // For computation of point/vertext location.
  this->SumCenter[0] = this->SumCenter[1] = this->SumCenter[2] = 0.0;
  
  numCells = input->GetNumberOfCells();
  for (cellId = 0; cellId < numCells; ++cellId)
    {
    cellType = input->GetCellType(cellId); 
    // Make sure we are not integrating ghost cells.
    if (ghostLevelArray && ghostLevelArray->GetComponent(cellId,0) > 0.0)
      {
      cellType = -1;
      }
    if (cellType == VTK_POLY_LINE || cellType == VTK_LINE)
      {
      if (this->CompareIntegrationDimension(1))
        {
        input->GetCellPoints(cellId, cellPtIds);
        this->IntegratePolyLine(input, output, cellId, cellPtIds);
        }
      }
    if (cellType == VTK_TRIANGLE)
      {
      if (this->CompareIntegrationDimension(2))
        {
        input->GetCellPoints(cellId, cellPtIds);
        this->IntegrateTriangle(input,output,cellId,cellPtIds->GetId(0), 
                                cellPtIds->GetId(1),cellPtIds->GetId(2));
        }
      }
    if (cellType == VTK_TRIANGLE_STRIP)
      {
      if (this->CompareIntegrationDimension(2))
        {
        input->GetCellPoints(cellId, cellPtIds);
        this->IntegrateTriangleStrip(input, output, cellId, cellPtIds);
        }
      }
    if (cellType == VTK_POLYGON)
      {
      if (this->CompareIntegrationDimension(2))
        {
        input->GetCellPoints(cellId, cellPtIds);
        this->IntegratePolygon(input, output, cellId, cellPtIds);
        }
      }
    if (cellType == VTK_PIXEL)
      {
      if (this->CompareIntegrationDimension(2))
        {
        vtkIdType pt1Id, pt2Id, pt3Id;
        input->GetCellPoints(cellId, cellPtIds);
        pt1Id = cellPtIds->GetId(0);
        pt2Id = cellPtIds->GetId(1);
        pt3Id = cellPtIds->GetId(2);
        this->IntegrateTriangle(input, output, cellId, pt1Id, pt2Id, pt3Id);
        pt1Id = cellPtIds->GetId(3);
        this->IntegrateTriangle(input, output, cellId, pt1Id, pt2Id, pt3Id);
        }
      }
    if (cellType == VTK_QUAD)
      {
      if (this->CompareIntegrationDimension(2))
        {
        vtkIdType pt1Id, pt2Id, pt3Id;
        input->GetCellPoints(cellId, cellPtIds);
        pt1Id = cellPtIds->GetId(0);
        pt2Id = cellPtIds->GetId(1);
        pt3Id = cellPtIds->GetId(2);
        this->IntegrateTriangle(input, output, cellId, pt1Id, pt2Id, pt3Id);
        pt2Id = cellPtIds->GetId(3);
        this->IntegrateTriangle(input, output, cellId, pt1Id, pt2Id, pt3Id);
        }
      }
    } 

  // Here is the trick:  The satellites need a point and vertex to 
  // marshal the attributes.  Node zero needs to receive first...
  int localProcId = 0;  
  // Send results to process 0.
  if (this->Controller)
    {
    localProcId = this->Controller->GetLocalProcessId();
    if (localProcId == 0)
      {
      int numProcs = this->Controller->GetNumberOfProcesses();
      int id;
      for (id = 1; id < numProcs; ++id)
        {
        double msg[5];
        this->Controller->Receive(msg, 5, id, 28876);
        vtkUnstructuredGrid* tmp = vtkUnstructuredGrid::New();
        this->Controller->Receive(tmp, id, 28877);        
        if (this->CompareIntegrationDimension((int)(msg[0])))
          {
          this->Sum += msg[1];
          this->SumCenter[0] += msg[2];
          this->SumCenter[1] += msg[3];
          this->SumCenter[2] += msg[4];
          this->IntegrateSatelliteData(tmp->GetPointData(),
                                       output->GetPointData());
          this->IntegrateSatelliteData(tmp->GetCellData(),
                                       output->GetCellData());
          }
        tmp->Delete();
        tmp = 0;
        }
      }
    }        
    
  // Generate point and vertex.  Add extra attributes for area too.
  // Satellites do not need the area attribute, but it does not hurt.
  double pt[3];
  vtkPoints* newPoints = vtkPoints::New();
  newPoints->SetNumberOfPoints(1);
  // Get rid of the weight factors.
  if (this->Sum != 0.0)
    {
    pt[0] = this->SumCenter[0] / this->Sum;    
    pt[1] = this->SumCenter[1] / this->Sum;    
    pt[2] = this->SumCenter[2] / this->Sum;    
    }
  else
    {
    pt[0] = this->SumCenter[0];    
    pt[1] = this->SumCenter[1];    
    pt[2] = this->SumCenter[2];    
    }    
  newPoints->InsertPoint(0, pt);
  output->SetPoints(newPoints);
  newPoints->Delete();
  newPoints = 0;
      
  output->Allocate(1);
  vtkIdType vertexPtIds[1];
  vertexPtIds[0] = 0;
  output->InsertNextCell(VTK_VERTEX, 1, vertexPtIds);

  // Create a new cell array for the total length, area or volume.
  vtkDoubleArray* sumArray = vtkDoubleArray::New();
  switch (this->IntegrationDimension)
    {
    case 1:
      sumArray->SetName("Length");
      break;
    case 2:
      sumArray->SetName("Area");
      break;
    case 3:
      sumArray->SetName("Volume");
      break;
    }
  sumArray->SetNumberOfTuples(1);
  sumArray->SetValue(0, this->Sum);
  output->GetCellData()->AddArray(sumArray);
  sumArray->Delete();
  cellPtIds->Delete();

  if (localProcId > 0)
    {
    double msg[5];
    msg[0] = (double)(this->IntegrationDimension);
    msg[1] = this->Sum;
    msg[2] = this->SumCenter[0];
    msg[3] = this->SumCenter[1];
    msg[4] = this->SumCenter[2];
    this->Controller->Send(msg, 5, 0, 28876);
    this->Controller->Send(output, 0, 28877);
    // Done sending.  Reset output so satellites will have empty data.    
    output->Initialize();
    }
}        

//-----------------------------------------------------------------------------
void vtkIntegrateAttributes::AllocateAttributes(vtkDataSetAttributes* inda,
                                                vtkDataSetAttributes* outda)
{
  int numArrays, i, numComponents, j;
  vtkDataArray* inArray;
  vtkDoubleArray* outArray;
  numArrays = inda->GetNumberOfArrays();
  for (i = 0; i < numArrays; ++i)
    {
    inArray = inda->GetArray(i);
    numComponents = inArray->GetNumberOfComponents();
    // All arrays are allocated double with one tuple.
    outArray = vtkDoubleArray::New();
    outArray->SetNumberOfComponents(numComponents);
    outArray->SetNumberOfTuples(1);
    outArray->SetName(inArray->GetName());
    // It cannot hurt to zero the arrays here.
    for (j = 0; j < numComponents; ++j)
      {
      outArray->SetComponent(0, j, 0.0);
      }
    outda->AddArray(outArray);
    outArray->Delete();
    outArray = 0;
    // Should we set scalars, vectors ...
    }
}
//-----------------------------------------------------------------------------
void vtkIntegrateAttributes::ZeroAttributes(vtkDataSetAttributes* outda)
{
  int numArrays, i, numComponents, j;
  vtkDataArray* outArray;
  numArrays = outda->GetNumberOfArrays();
  for (i = 0; i < numArrays; ++i)
    {
    outArray = outda->GetArray(i);
    numComponents = outArray->GetNumberOfComponents();
    for (j = 0; j < numComponents; ++j)
      {
      outArray->SetComponent(0, j, 0.0);
      }
    }
}
//-----------------------------------------------------------------------------
void vtkIntegrateAttributes::IntegrateData1(vtkDataSetAttributes* inda,
                                            vtkDataSetAttributes* outda,
                                            vtkIdType pt1Id, double k)
{
  int numArrays, i, numComponents, j;
  vtkDataArray* inArray;
  vtkDataArray* outArray;
  numArrays = inda->GetNumberOfArrays();
  double vIn1, dv, vOut;
  for (i = 0; i < numArrays; ++i)
    {
    // We could template for speed.
    inArray = inda->GetArray(i);
    outArray = outda->GetArray(i);
    numComponents = inArray->GetNumberOfComponents();
    for (j = 0; j < numComponents; ++j)
      {
      vIn1 = inArray->GetComponent(pt1Id, j);
      vOut = outArray->GetComponent(0, j);
      dv = vIn1;  
      vOut += dv*k; 
      outArray->SetComponent(0, j, vOut);
      }
    }
}
//-----------------------------------------------------------------------------
void vtkIntegrateAttributes::IntegrateData2(vtkDataSetAttributes* inda,
                                            vtkDataSetAttributes* outda,
                                            vtkIdType pt1Id, vtkIdType pt2Id,
                                            double k)
{
  int numArrays, i, numComponents, j;
  vtkDataArray* inArray;
  vtkDataArray* outArray;
  numArrays = inda->GetNumberOfArrays();
  double vIn1, vIn2, dv, vOut;
  for (i = 0; i < numArrays; ++i)
    {
    // We could template for speed.
    inArray = inda->GetArray(i);
    outArray = outda->GetArray(i);
    numComponents = inArray->GetNumberOfComponents();
    for (j = 0; j < numComponents; ++j)
      {
      vIn1 = inArray->GetComponent(pt1Id, j);
      vIn2 = inArray->GetComponent(pt2Id, j);
      vOut = outArray->GetComponent(0, j);
      dv = 0.5*(vIn1+vIn2);  
      vOut += dv*k; 
      outArray->SetComponent(0, j, vOut);
      }
    }
}
//-----------------------------------------------------------------------------
// Is the extra performance worth duplicating this code with IntergrateData2.
void vtkIntegrateAttributes::IntegrateData3(vtkDataSetAttributes* inda,
                                            vtkDataSetAttributes* outda,
                                            vtkIdType pt1Id, vtkIdType pt2Id,
                                            vtkIdType pt3Id, double k)
{
  int numArrays, i, numComponents, j;
  vtkDataArray* inArray;
  vtkDataArray* outArray;
  numArrays = inda->GetNumberOfArrays();
  double vIn1, vIn2, vIn3, dv, vOut;
  for (i = 0; i < numArrays; ++i)
    {
    // We could template for speed.
    inArray = inda->GetArray(i);
    outArray = outda->GetArray(i);
    numComponents = inArray->GetNumberOfComponents();
    for (j = 0; j < numComponents; ++j)
      {
      vIn1 = inArray->GetComponent(pt1Id, j);
      vIn2 = inArray->GetComponent(pt2Id, j);
      vIn3 = inArray->GetComponent(pt3Id, j);
      vOut = outArray->GetComponent(0, j);
      dv = (vIn1+vIn2+vIn3)/3.0;  
      vOut += dv*k; 
      outArray->SetComponent(0, j, vOut);
      }
    }
}

//-----------------------------------------------------------------------------
// Used to sum arrays from all processes.
void vtkIntegrateAttributes::IntegrateSatelliteData(vtkDataSetAttributes* inda,
                                                    vtkDataSetAttributes* outda)
{
  int numArrays, i, numComponents, j;
  vtkDataArray* inArray;
  vtkDataArray* outArray;
  numArrays = outda->GetNumberOfArrays();
  double vIn, vOut;
  for (i = 0; i < numArrays; ++i)
    {
    outArray = outda->GetArray(i);
    numComponents = outArray->GetNumberOfComponents();
    // Protect against arrays in a different order.
    const char* name = outArray->GetName();
    if (name && name[0] != '\0')
      {
      inArray = inda->GetArray(name);
      if (inArray && inArray->GetNumberOfComponents() == numComponents)
        {
        // We could template for speed.
        for (j = 0; j < numComponents; ++j)
          {
          vIn = inArray->GetComponent(0, j);
          vOut = outArray->GetComponent(0, j);
          outArray->SetComponent(0,j,vOut+vIn);
          }
        }
      }
    }
}
       
//-----------------------------------------------------------------------------
void vtkIntegrateAttributes::IntegratePolyLine(vtkDataSet* input, 
                                            vtkUnstructuredGrid* output,
                                            vtkIdType cellId, vtkIdList* ptIds)
{
  double tmp, length;
  double pt1[3], pt2[3], mid[3];
  vtkIdType numLines, lineIdx;
  vtkIdType pt1Id, pt2Id;
  
  numLines = ptIds->GetNumberOfIds()-1;
  for (lineIdx = 0; lineIdx < numLines; ++lineIdx)
    {
    pt1Id = ptIds->GetId(lineIdx);
    pt2Id = ptIds->GetId(lineIdx+1);
    input->GetPoint(pt1Id, pt1);
    input->GetPoint(pt2Id,pt2);

    // Compute the length of the line.
    tmp = pt2[0] - pt1[0];
    length = tmp*tmp;
    tmp = pt2[1] - pt1[1];
    length += tmp*tmp;
    tmp = pt2[2] - pt1[2];
    length += tmp*tmp;
    length = sqrt(length);
    this->Sum += length;

    // Compute the middle, which is really just another attribute.
    mid[0] = (pt1[0]+pt2[0])*0.5;  
    mid[1] = (pt1[1]+pt2[1])*0.5;  
    mid[2] = (pt1[2]+pt2[2])*0.5;  
    // Add weighted to sumCenter.
    this->SumCenter[0] += mid[0]*length; 
    this->SumCenter[1] += mid[1]*length; 
    this->SumCenter[2] += mid[2]*length;
    
    // Now integrate the rest of the attributes.
    this->IntegrateData2(input->GetPointData(), output->GetPointData(),
                         pt1Id, pt2Id, length);  
    this->IntegrateData1(input->GetCellData(), output->GetCellData(), 
                         cellId, length);  
    }
}

//-----------------------------------------------------------------------------
void vtkIntegrateAttributes::IntegrateTriangleStrip(vtkDataSet* input, 
                                           vtkUnstructuredGrid* output,
                                           vtkIdType cellId, vtkIdList* ptIds)
{
  vtkIdType numTris, triIdx;
  vtkIdType pt1Id, pt2Id, pt3Id;
  
  numTris = ptIds->GetNumberOfIds()-2;
  for (triIdx = 0; triIdx < numTris; ++triIdx)
    {
    pt1Id = ptIds->GetId(triIdx);
    pt2Id = ptIds->GetId(triIdx+1);
    pt3Id = ptIds->GetId(triIdx+2);
    this->IntegrateTriangle(input, output, cellId, pt1Id, pt2Id, pt3Id);
    }
}

//-----------------------------------------------------------------------------
// WOrks for convex polygons, and interpoaltion is not correct.
void vtkIntegrateAttributes::IntegratePolygon(vtkDataSet* input, 
                                            vtkUnstructuredGrid* output,
                                            vtkIdType cellId, vtkIdList* ptIds)
{
  vtkIdType numTris, triIdx;
  vtkIdType pt1Id, pt2Id, pt3Id;
  
  numTris = ptIds->GetNumberOfIds()-2;
  pt1Id = ptIds->GetId(0);
  for (triIdx = 0; triIdx < numTris; ++triIdx)
    {
    pt2Id = ptIds->GetId(triIdx+1);
    pt3Id = ptIds->GetId(triIdx+2);
    this->IntegrateTriangle(input, output, cellId, pt1Id, pt2Id, pt3Id);
    }
}

//-----------------------------------------------------------------------------
void vtkIntegrateAttributes::IntegrateTriangle(vtkDataSet* input, 
                                             vtkUnstructuredGrid* output,
                                             vtkIdType cellId, vtkIdType pt1Id,
                                             vtkIdType pt2Id, vtkIdType pt3Id)
{
  double pt1[3], pt2[3], pt3[3];
  double mid[3], v1[3], v2[3];
  double cross[3];
  double k;
  
  input->GetPoint(pt1Id,pt1);
  input->GetPoint(pt2Id,pt2);
  input->GetPoint(pt3Id,pt3);

  // Compute two legs.
  v1[0] = pt2[0] - pt1[0];
  v1[1] = pt2[1] - pt1[1];
  v1[2] = pt2[2] - pt1[2];
  v2[0] = pt3[0] - pt1[0];
  v2[1] = pt3[1] - pt1[1];
  v2[2] = pt3[2] - pt1[2];

  // Use the cross product to compute the area of the parallelogram.
  vtkMath::Cross(v1,v2,cross);
  k = sqrt(cross[0]*cross[0] + cross[1]*cross[1] + cross[2]*cross[2]) * 0.5;
  
  if (k == 0.0)
    {
    return;
    }
  this->Sum += k;

  // Compute the middle, which is really just another attribute.
  mid[0] = (pt1[0]+pt2[0]+pt3[0])/3.0;  
  mid[1] = (pt1[1]+pt2[1]+pt3[1])/3.0;  
  mid[2] = (pt1[2]+pt2[2]+pt3[2])/3.0;  
  // Add weighted to sumCenter.
  this->SumCenter[0] += mid[0]*k; 
  this->SumCenter[1] += mid[1]*k; 
  this->SumCenter[2] += mid[2]*k;
    
  // Now integrate the rest of the attributes.
  this->IntegrateData3(input->GetPointData(), output->GetPointData(),
                       pt1Id, pt2Id, pt3Id, k);  
  this->IntegrateData1(input->GetCellData(), output->GetCellData(), cellId, k);
}


//-----------------------------------------------------------------------------
void vtkIntegrateAttributes::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "IntegrationDimension: " 
     << this->IntegrationDimension << endl;

}

