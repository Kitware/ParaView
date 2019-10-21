/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGMVReader.cxx

  Copyright (c) 2009-2012 Sven Buijssen, Jens Acker, TU Dortmund
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkGMVReader.h"

#include "vtkByteSwap.h"
#include "vtkCallbackCommand.h"
#include "vtkCellArray.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArraySelection.h"
#include "vtkErrorCode.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyhedron.h"
#include "vtkRectilinearGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkStructuredGrid.h"
#include "vtkTypeInt32Array.h"
#include "vtkTypeInt64Array.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVertex.h"
#include <algorithm>
#include <set>
#include <vtksys/SystemTools.hxx>

#if VTK_MODULE_ENABLE_VTK_ParallelCore
#include "vtkMultiProcessController.h"
vtkCxxSetObjectMacro(vtkGMVReader, Controller, vtkMultiProcessController);
#else
void vtkGMVReader::SetController(vtkMultiProcessController*)
{
  vtkWarningMacro("Ignoring SetController in a non-MPI build");
}
#endif

vtkStandardNewMacro(vtkGMVReader);

//----------------------------------------------------------------------------
namespace GMVRead
{
// include GMV's I/O utility provided by GMV's author, Frank Ortega (LANL).
// import it from VisItBridge's include directories
extern "C" {
#include "gmvread.c"
}

template <class C>
void minmax(C* pointer, size_t len, C& min_out, C& max_out)
{
  if (len <= 0)
  {
    min_out = max_out = C(0);
    return;
  }
  min_out = max_out = *pointer;
  ++pointer;
  for (size_t i = 1; i < len; ++i, ++pointer)
  {
    min_out = std::min(min_out, *pointer);
    max_out = std::max(max_out, *pointer);
  }
}

template <class C>
void cleanup(C** pointer)
{
  if ((*pointer) != (C*)NULL)
  {
    free(*pointer);
    *pointer = (C*)(NULL);
  }
}

void cleanupMesh(void)
{
  gmv_meshdata.nnodes = gmv_meshdata.ncells = gmv_meshdata.nfaces = gmv_meshdata.totfaces =
    gmv_meshdata.totverts = 0;
  gmv_meshdata.intype = gmv_meshdata.nxv = gmv_meshdata.nyv = gmv_meshdata.nzv = 0;
  cleanup(&gmv_meshdata.x);
  cleanup(&gmv_meshdata.y);
  cleanup(&gmv_meshdata.z);
  cleanup(&gmv_meshdata.cellnnode);
  cleanup(&gmv_meshdata.cellnodes);
  cleanup(&gmv_meshdata.celltoface);
  cleanup(&gmv_meshdata.cellfaces);
  cleanup(&gmv_meshdata.facetoverts);
  cleanup(&gmv_meshdata.faceverts);
  cleanup(&gmv_meshdata.facecell1);
  cleanup(&gmv_meshdata.facecell2);
  cleanup(&gmv_meshdata.vfacepe);
  cleanup(&gmv_meshdata.vfaceoppface);
  cleanup(&gmv_meshdata.vfaceoppfacepe);
}

void cleanupAllData(void)
{
  gmv_data.num = gmv_data.num2 = 0;
  gmv_data.keyword = gmv_data.datatype = gmv_data.nchardata1 = gmv_data.nchardata2 = 0;
  gmv_data.ndoubledata1 = gmv_data.ndoubledata2 = gmv_data.ndoubledata3 = 0;
  gmv_data.nlongdata1 = gmv_data.nlongdata2 = 0;
  gmv_data.name1[0] = char(0);
  cleanup(&gmv_data.doubledata1);
  cleanup(&gmv_data.doubledata2);
  cleanup(&gmv_data.doubledata3);
  cleanup(&gmv_data.longdata1);
  cleanup(&gmv_data.longdata2);
  cleanup(&gmv_data.chardata1);
  cleanup(&gmv_data.chardata2);
}
}

//----------------------------------------------------------------------------
vtkGMVReader::vtkGMVReader()
{
  this->FileName = NULL;
  this->FileNames = vtkStringArray::New();
  this->NumberOfTracersMap.clear();
  this->NumberOfPolygonsMap.clear();
  this->ContainsProbtimeKeyword = false;

  this->ByteOrder = FILE_LITTLE_ENDIAN;
  this->BinaryFile = 0;
  this->NumberOfNodeFields = 0;
  this->NumberOfCellFields = 0;
  this->NumberOfFields = 0;
  this->NumberOfNodeComponents = 0;
  this->NumberOfCellComponents = 0;
  this->NumberOfFieldComponents = 0;
  this->DecrementNodeIds = true; // node numbering starts at 1, in VTK at 0

  this->Mesh = NULL;
  this->FieldDataTmp = NULL;
  this->NumberOfNodes = 0;
  this->NumberOfCells = 0;

  this->Tracers = NULL;
  this->NumberOfTracers = 0;
  this->ImportTracers = 1;

  this->Polygons = NULL;
  this->NumberOfPolygons = 0;
  this->ImportPolygons = 0;

  this->NodeDataInfo = NULL;
  this->CellDataInfo = NULL;

  this->PointDataArraySelection = vtkDataArraySelection::New();
  this->CellDataArraySelection = vtkDataArraySelection::New();
  this->FieldDataArraySelection = vtkDataArraySelection::New();

  // Setup the selection callback to modify this object when an array
  // selection is changed.
  this->SelectionObserver = vtkCallbackCommand::New();
  this->SelectionObserver->SetCallback(&vtkGMVReader::SelectionModifiedCallback);
  this->SelectionObserver->SetClientData(this);
  this->PointDataArraySelection->AddObserver(vtkCommand::ModifiedEvent, this->SelectionObserver);
  this->CellDataArraySelection->AddObserver(vtkCommand::ModifiedEvent, this->SelectionObserver);
  this->FieldDataArraySelection->AddObserver(vtkCommand::ModifiedEvent, this->SelectionObserver);

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  // this->CurrentOutput = 0;

  this->Controller = nullptr;
#if VTK_MODULE_ENABLE_VTK_ParallelCore
  this->SetController(vtkMultiProcessController::GetGlobalController());
#endif
}

//----------------------------------------------------------------------------
vtkGMVReader::~vtkGMVReader()
{
  if (this->FileNames)
  {
    this->FileNames->Delete();
    this->FileNames = NULL;
  }
  this->SetFileName(0);
  this->NumberOfTracersMap.clear();
  this->NumberOfPolygonsMap.clear();

  if (this->NodeDataInfo)
    delete[] this->NodeDataInfo;
  if (this->CellDataInfo)
    delete[] this->CellDataInfo;

  this->PointDataArraySelection->RemoveObserver(this->SelectionObserver);
  this->PointDataArraySelection->Delete();
  this->CellDataArraySelection->RemoveObserver(this->SelectionObserver);
  this->CellDataArraySelection->Delete();
  this->FieldDataArraySelection->RemoveObserver(this->SelectionObserver);
  this->FieldDataArraySelection->Delete();
  this->SelectionObserver->Delete();

  if (this->Mesh)
    this->Mesh->Delete();
  if (this->FieldDataTmp)
    this->FieldDataTmp->Delete();
  if (this->Tracers)
    this->Tracers->Delete();
  if (this->Polygons)
    this->Polygons->Delete();

#if VTK_MODULE_ENABLE_VTK_ParallelCore
  this->SetController(NULL);
#endif
}

//----------------------------------------------------------------------------
int vtkGMVReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  bool keepParsing;
  bool firstPolygonParsed;
  float progress;
  int dims[3];
  int incr = 0;
  int polygonMaterialPosInDataArray;
  int posInDataArray;
  size_t k;
  size_t numFaces; // number of faces of element i
  size_t numNodes;
  size_t numNodesSoFar = 0;
  unsigned int blockNo;
  vtkCellArray* polygonCells;
  vtkCellArray* tracerCells;
  vtkFloatArray* coords;
  vtkIdType list[27];
  vtkTypeInt64Array* polygonMaterials;
  vtkPoints* points;
  vtkPoints* polygonPoints;
  vtkPolyData* pd;
  vtkFieldData* fieldptr;

  // get the info object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the output
  vtkMultiBlockDataSet* output =
    vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkMultiBlockDataSet::DATA_OBJECT()));
  // this->CurrentOutput = output;

  vtkDebugMacro(<< "Send GMV data to Paraview");

  this->UpdateProgress(0.0);

  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {

    // Get the requested time step. We only support requests of a single time
    // step in this reader right now
    double requestedTimeValue = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

    // Snapping and clamping of the requested time step to be in bounds is done by
    // FileSeriesReader class.
    vtkDebugMacro(<< "RequestData: requested time value: " << requestedTimeValue);

    output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), requestedTimeValue);
  }

  vtkDebugMacro(<< "GMVReader::RequestData: Reading from file <" << this->FileName << ">...");

  int ierr = GMVRead::gmvread_open(this->FileName);
  if (ierr > 0)
  {
    if (GMVRead::gmv_data.errormsg != NULL)
    {
      vtkErrorMacro("" << GMVRead::gmv_data.errormsg);
    }
    else
    {
      vtkErrorMacro("GMV reader library reported an unhandled error.");
    }
    // this->SetupEmptyOutput();
    // this->CurrentOutput = 0;
    return 0;
  }
  if (ierr != -1)
    this->BinaryFile = 1;

  pd = NULL;
  keepParsing = true;
  firstPolygonParsed = false;
  polygonPoints = NULL;
  polygonCells = NULL;
  polygonMaterials = NULL;
  polygonMaterialPosInDataArray = -1;

  if (this->Mesh)
  {
    this->Mesh->Delete();
    this->Mesh = NULL;
  }
  if (this->FieldDataTmp)
  {
    this->FieldDataTmp->Delete();
    this->FieldDataTmp = NULL;
  }
  if (this->Tracers)
  {
    this->Tracers->Delete();
    this->Tracers = NULL;
  }
  if (this->Polygons)
  {
    this->Polygons->Delete();
    this->Polygons = NULL;
  }

  while (keepParsing)
  {
    GMVRead::gmvread_data();
    switch (GMVRead::gmv_data.keyword)
    {
      case (GMVERROR):
        if (GMVRead::gmv_data.errormsg != NULL)
        {
          vtkErrorMacro("" << GMVRead::gmv_data.errormsg);
        }
        else
        {
          vtkErrorMacro("GMV reader library reported an unhandled error.");
        }
        GMVRead::gmvread_close();
        // this->SetupEmptyOutput();
        // this->CurrentOutput = 0;
        return 0;

      case (GMVEND):
        keepParsing = false;
        GMVRead::gmvread_close();
        break;

      case (NODES):
        switch (GMVRead::gmv_data.datatype)
        {
          case (LOGICALLY_STRUCT):
          {
            vtkPoints* pts;
            vtkStructuredGrid* sgrid;

            pts = vtkPoints::New();
            pts->SetNumberOfPoints(GMVRead::gmv_data.num);
            if (GMVRead::gmv_data.num > 0)
            {
              float* ptr = (float*)pts->GetVoidPointer(0);
              for (long i = 0; i < GMVRead::gmv_data.num; ++i)
              {
                *ptr++ = GMVRead::gmv_data.doubledata1[i];
                *ptr++ = GMVRead::gmv_data.doubledata2[i];
                *ptr++ = GMVRead::gmv_data.doubledata3[i];
              }
            }

            blockNo = output->GetNumberOfBlocks();
            vtkDebugMacro("creating new structured output");
            sgrid = vtkStructuredGrid::New();
            sgrid->SetPoints(pts);
            pts->Delete();

            dims[0] = GMVRead::gmv_data.ndoubledata1;
            dims[1] = GMVRead::gmv_data.ndoubledata2;
            dims[2] = GMVRead::gmv_data.ndoubledata3;
            sgrid->SetDimensions(dims);

            output->SetBlock(blockNo, sgrid);
            this->Mesh = sgrid;
            output->GetMetaData(blockNo)->Set(vtkCompositeDataSet::NAME(), "Element Sets");

            GMVRead::gmvread_mesh();
            // Reassign the values. Previously read values (in
            // RequestInformation()) may have been invalidated by data from
            // later time steps.
            this->NumberOfNodes = GMVRead::gmv_meshdata.nnodes;
            this->NumberOfCells = GMVRead::gmv_meshdata.ncells;
          }

          break;

          case (STRUCT):
          {
            blockNo = output->GetNumberOfBlocks();
            vtkDebugMacro("creating new rectilinear output");
            vtkRectilinearGrid* rgrid = vtkRectilinearGrid::New();
            output->SetBlock(blockNo, rgrid);
            this->Mesh = rgrid;
            output->GetMetaData(blockNo)->Set(vtkCompositeDataSet::NAME(), "Element Sets");

            dims[0] = GMVRead::gmv_data.ndoubledata1;
            dims[1] = GMVRead::gmv_data.ndoubledata2;
            dims[2] = GMVRead::gmv_data.ndoubledata3;
            rgrid->SetDimensions(dims);

            vtkFloatArray* xc = vtkFloatArray::New();
            xc->SetNumberOfTuples(dims[0]);
            for (int i = 0; i < dims[0]; ++i)
              xc->SetTuple1(i, GMVRead::gmv_data.doubledata1[i]);

            vtkFloatArray* yc = vtkFloatArray::New();
            yc->SetNumberOfTuples(dims[1]);
            for (int i = 0; i < dims[1]; ++i)
              yc->SetTuple1(i, GMVRead::gmv_data.doubledata2[i]);

            vtkFloatArray* zc = vtkFloatArray::New();
            zc->SetNumberOfTuples(dims[2]);
            for (int i = 0; i < dims[2]; ++i)
              zc->SetTuple1(i, GMVRead::gmv_data.doubledata3[i]);

            rgrid->SetXCoordinates(xc);
            xc->Delete();
            rgrid->SetYCoordinates(yc);
            yc->Delete();
            rgrid->SetZCoordinates(zc);
            zc->Delete();

            GMVRead::gmvread_mesh();
            // Reassign the values. Previously read values (in
            // RequestInformation()) may have been invalidated by data from
            // later time steps.
            this->NumberOfNodes = GMVRead::gmv_meshdata.nnodes;
            this->NumberOfCells = GMVRead::gmv_meshdata.ncells;
          }

          break;

          case (UNSTRUCT):
            vtkUnstructuredGrid* ugrid;
            GMVRead::gmvread_mesh();

            // An i/o error may have occurred
            if (GMVRead::gmv_meshdata.intype == GMVERROR)
            {
              if (GMVRead::gmv_data.errormsg != NULL)
              {
                vtkErrorMacro("" << GMVRead::gmv_data.errormsg);
              }
              else
              {
                vtkErrorMacro("GMV reader library reported an unhandled error.");
              }
              // this->SetupEmptyOutput();
              // this->CurrentOutput = 0;
              return 0;
            }

            // Reassign the values. Previously read values (in
            // RequestInformation()) may have been invalidated by data from
            // later time steps.
            this->NumberOfNodes = GMVRead::gmv_meshdata.nnodes;
            this->NumberOfCells = GMVRead::gmv_meshdata.ncells;
            // Prepare to send mesh data to Paraview

            blockNo = output->GetNumberOfBlocks();
            vtkDebugMacro("creating new unstructured output");
            ugrid = vtkUnstructuredGrid::New();
            ugrid->Allocate(this->NumberOfCells);
            output->SetBlock(blockNo, ugrid);
            this->Mesh = ugrid;
            output->GetMetaData(blockNo)->Set(vtkCompositeDataSet::NAME(), "Element Sets");

            // -------------------------
            // Handle coordinates
            coords = vtkFloatArray::New();
            coords->SetNumberOfComponents(3);
            coords->SetNumberOfTuples(GMVRead::gmv_meshdata.nnodes);
            // repackage node coordinates
            float* ptr = coords->GetPointer(0);
            for (long i = 0; i < GMVRead::gmv_meshdata.nnodes; ++i)
            {
              ptr[3 * i] = GMVRead::gmv_meshdata.x[i];
              ptr[3 * i + 1] = GMVRead::gmv_meshdata.y[i];
              ptr[3 * i + 2] = GMVRead::gmv_meshdata.z[i];
            }
            points = vtkPoints::New();
            points->SetData(coords);
            coords->Delete();

            // Set points of the mesh
            ugrid->SetPoints(points);
            points->Delete();
            this->UpdateProgress(0.1);

            // -------------------------
            // Handle cells
            if (this->DecrementNodeIds)
              // GMV node numbers start at 1, in VTK they start at 0
              incr = -1;
            else
              incr = 0;

            // Look at each cell
            for (size_t i = 0; i < this->NumberOfCells; ++i)
            {
              // Catch case that only generic cells are present. Then cellnnode is not set.
              numNodes = 0;
              numFaces = 0;
              if (GMVRead::gmv_meshdata.cellnnode != NULL)
                numNodes = GMVRead::gmv_meshdata.cellnnode[i];
              if (GMVRead::gmv_meshdata.celltoface != NULL)
                numFaces =
                  GMVRead::gmv_meshdata.celltoface[i + 1] - GMVRead::gmv_meshdata.celltoface[i];

              // Implicitly given mesh (structured regular or logically rectangular brick mesh)
              if (numNodes == 0 && numFaces == 0)
              {
                vtkErrorMacro(
                  << "An implicitly given mesh was found (like structured regular brick meshes and "
                  << "logically rectangular brick meshes), but still the code for unstructured "
                  << "grids is invoked. This should not have happened!");
                break;
              }
              // Line
              if (numNodes == 2 && numFaces == 1)
              {
                for (k = 0; k < numNodes; ++k)
                  list[k] = GMVRead::gmv_meshdata.cellnodes[numNodesSoFar + k] + incr;
                ugrid->InsertNextCell(VTK_LINE, numNodes, list);
              }
              else if (numNodes == 3 && numFaces == 2)
              {
                for (k = 0; k < numNodes; ++k)
                  list[k] = GMVRead::gmv_meshdata.cellnodes[numNodesSoFar + k] + incr;
                ugrid->InsertNextCell(VTK_QUADRATIC_EDGE, numNodes, list);
              }
              // Triangle
              else if (numNodes == 3 && numFaces == 1)
              {
                for (k = 0; k < numNodes; ++k)
                  list[k] = GMVRead::gmv_meshdata.cellnodes[numNodesSoFar + k] + incr;
                ugrid->InsertNextCell(VTK_TRIANGLE, numNodes, list);
              }
              else if (numNodes == 6 && numFaces == 1)
              {
                for (k = 0; k < numNodes; ++k)
                  list[k] = GMVRead::gmv_meshdata.cellnodes[numNodesSoFar + k] + incr;
                ugrid->InsertNextCell(VTK_QUADRATIC_TRIANGLE, numNodes, list);
              }
              // Quad
              else if (numNodes == 4 && numFaces == 1)
              {
                for (k = 0; k < numNodes; ++k)
                  list[k] = GMVRead::gmv_meshdata.cellnodes[numNodesSoFar + k] + incr;
                ugrid->InsertNextCell(VTK_QUAD, numNodes, list);
              }
              else if (numNodes == 8 && numFaces == 1)
              {
                for (k = 0; k < numNodes; ++k)
                  list[k] = GMVRead::gmv_meshdata.cellnodes[numNodesSoFar + k] + incr;
                ugrid->InsertNextCell(VTK_QUADRATIC_QUAD, numNodes, list);
              }
              // Tetraeder
              else if (numNodes == 4 && numFaces == 4)
              {
                for (k = 0; k < numNodes; ++k)
                  list[k] = GMVRead::gmv_meshdata.cellnodes[numNodesSoFar + k] + incr;
                ugrid->InsertNextCell(VTK_TETRA, numNodes, list);
              }
              else if (numNodes == 10 && numFaces == 4)
              {
                for (k = 0; k < numNodes; ++k)
                  list[k] = GMVRead::gmv_meshdata.cellnodes[numNodesSoFar + k] + incr;
                ugrid->InsertNextCell(VTK_QUADRATIC_TETRA, numNodes, list);
              }
              // Hexaeder
              else if (numNodes == 8 && numFaces == 6)
              {
                for (k = 0; k < numNodes; ++k)
                  list[k] = GMVRead::gmv_meshdata.cellnodes[numNodesSoFar + k] + incr;
                ugrid->InsertNextCell(VTK_HEXAHEDRON, numNodes, list);
              }
              else if (numNodes == 20 && numFaces == 6)
              {
                for (k = 0; k < numNodes; ++k)
                  list[k] = GMVRead::gmv_meshdata.cellnodes[numNodesSoFar + k] + incr;
                ugrid->InsertNextCell(VTK_QUADRATIC_HEXAHEDRON, numNodes, list);
              }
              else if (numNodes == 27 && numFaces == 48)
              {
                for (k = 0; k < numNodes; ++k)
                  list[k] = GMVRead::gmv_meshdata.cellnodes[numNodesSoFar + k] + incr;

                list[20] = GMVRead::gmv_meshdata.cellnodes[numNodesSoFar + 23] + incr;
                list[22] = GMVRead::gmv_meshdata.cellnodes[numNodesSoFar + 20] + incr;
                list[23] = GMVRead::gmv_meshdata.cellnodes[numNodesSoFar + 22] + incr;

                ugrid->InsertNextCell(VTK_TRIQUADRATIC_HEXAHEDRON, numNodes, list);
              }
              // Pyramid
              else if (numNodes == 5 && numFaces == 5)
              {
                // This branch catches both the GMV cell types 'pyramid' and 'ppyrmd5'.
                // Distinguish them by checking the number of vertices used for the first
                // face of the pyramdi: 3 for 'pyramid', 4 for 'ppyrmd5'.

                // face offset (= face number, counting from 0)
                const unsigned long j = 0;
                // number of unique vertices of first face of pyramid element i
                unsigned long numPtsFirstFace =
                  GMVRead::gmv_meshdata.facetoverts[GMVRead::gmv_meshdata.celltoface[i] + j + 1] -
                  GMVRead::gmv_meshdata.facetoverts[GMVRead::gmv_meshdata.celltoface[i] + j];

                if (numPtsFirstFace == 3)
                {
                  // Node numbering for GMV cell type 'pyramid', see gmvdoc.color.pdf page 80:
                  //   top of the pyramid is the first node.
                  // Pyramid node number in VTK: top of the pyramid is the last node
                  for (k = 1; k < numNodes; ++k)
                    list[k - 1] = GMVRead::gmv_meshdata.cellnodes[numNodesSoFar + k] + incr;
                  list[numNodes - 1] = GMVRead::gmv_meshdata.cellnodes[numNodesSoFar + 0] + incr;
                }
                else
                {
                  // Node numbering for GMV cell type 'ppyrmd5', see gmvdoc.color.pdf page 80,
                  // is identical to that in VTK: top of the pyramid is the last node
                  for (k = 0; k < numNodes; ++k)
                    list[k] = GMVRead::gmv_meshdata.cellnodes[numNodesSoFar + k] + incr;
                }
                ugrid->InsertNextCell(VTK_PYRAMID, numNodes, list);
              }
              else if (numNodes == 13 && numFaces == 5)
              {
                for (k = 0; k < numNodes; ++k)
                  list[k] = GMVRead::gmv_meshdata.cellnodes[numNodesSoFar + k] + incr;
                ugrid->InsertNextCell(VTK_QUADRATIC_PYRAMID, numNodes, list);
              }
              // Prism
              else if (numNodes == 6 && numFaces == 5)
              {
                for (k = 0; k < numNodes; ++k)
                  list[k] = GMVRead::gmv_meshdata.cellnodes[numNodesSoFar + k] + incr;
                ugrid->InsertNextCell(VTK_WEDGE, numNodes, list);
              }
              else if (numNodes == 15 && numFaces == 5)
              {
                for (k = 0; k < numNodes; ++k)
                  list[k] = GMVRead::gmv_meshdata.cellnodes[numNodesSoFar + k] + incr;
                ugrid->InsertNextCell(VTK_QUADRATIC_WEDGE, numNodes, list);
              }
              // General (generic cell type, the only case where
              // GMVRead::gmv_meshdata.cellnnode[i] keeps its default value of 0)
              else if (numNodes == 0 && numFaces > 0)
              {
                // Distinguish between 1 face and more than 1 face to avoid using VTK_POLYHEDRON for
                // all cases as that would introduce unnecessary complexity in downstream filters.
                if (numFaces == 1) // 2D cell
                {
                  // face offset (= face number, counting from 0)
                  const unsigned long j = 0;
                  vtkIdType* pointIds;
                  // number of unique vertices (of single face) of generic element i
                  unsigned long numPts =
                    GMVRead::gmv_meshdata.facetoverts[GMVRead::gmv_meshdata.celltoface[i] + j + 1] -
                    GMVRead::gmv_meshdata.facetoverts[GMVRead::gmv_meshdata.celltoface[i] + j];

                  pointIds = new vtkIdType[numPts];
                  for (k = 0; k < numPts; k++)
                  {
                    pointIds[k] =
                      GMVRead::gmv_meshdata
                        .faceverts[GMVRead::gmv_meshdata
                                     .facetoverts[GMVRead::gmv_meshdata.celltoface[i] + j] +
                          k] +
                      incr;
                  }

                  ugrid->InsertNextCell(VTK_POLYGON, numPts, pointIds);
                  delete[] pointIds;
                  pointIds = NULL;
                }

                else // 3D cell
                {
                  // number of vertices of face j of generic element i
                  unsigned long numVerts;
                  // number of unique vertices of generic element i
                  vtkIdType numPts;
                  vtkIdType* pointIds;
                  std::set<int> auxIds;
                  std::set<int>::iterator auxIt;

                  vtkIdType* face = NULL;
                  vtkCellArray* faces = NULL;
                  faces = vtkCellArray::New();
                  for (unsigned long j = 0; j < numFaces; j++)
                  {
                    numVerts = GMVRead::gmv_meshdata
                                 .facetoverts[GMVRead::gmv_meshdata.celltoface[i] + j + 1] -
                      GMVRead::gmv_meshdata.facetoverts[GMVRead::gmv_meshdata.celltoface[i] + j];

                    face = new vtkIdType[numVerts];
                    for (k = 0; k < numVerts; k++)
                    {
                      face[k] =
                        GMVRead::gmv_meshdata
                          .faceverts[GMVRead::gmv_meshdata
                                       .facetoverts[GMVRead::gmv_meshdata.celltoface[i] + j] +
                            k] +
                        incr;
                      auxIds.insert(face[k]);
                    }
                    faces->InsertNextCell(numVerts, face);
                    delete[] face;
                    face = NULL;
                  }

                  // number of unique vertices of generic element i
                  numPts = auxIds.size();
                  pointIds = new vtkIdType[numPts];
                  for (auxIt = auxIds.begin(), k = 0; auxIt != auxIds.end(); auxIt++, k++)
                  {
                    pointIds[k] = *auxIt;
                  }
                  vtkNew<vtkIdTypeArray> faceData;
                  faces->ExportLegacyFormat(faceData);
                  ugrid->InsertNextCell(VTK_POLYHEDRON, numPts, pointIds, faces->GetNumberOfCells(),
                    faceData->GetPointer(0));
                  delete[] pointIds;
                  pointIds = NULL;
                  faces->Delete();
                  faces = NULL;
                }
              }
              // Unknown/no handler yet
              else
              {
                vtkErrorMacro(<< "Cell no. " << i << " is of a yet unsupported cell type with "
                              << numNodes << " nodes and " << numFaces << " faces" << endl);
              }

              numNodesSoFar += numNodes;
            }

            // Catch case that no cells are defined. The nodes are still
            // rendered by GMV itself, but without topological and just
            // geometric information they will not in ParaView. Define a vertex
            // per nodes.
            if (this->NumberOfCells == 0)
            {
              vtkCellArray* vertices = vtkCellArray::New();
              for (long i = 0; i < GMVRead::gmv_meshdata.nnodes; i++)
              {
                vtkIdType id[1];
                id[0] = i;
                vertices->InsertNextCell(1, id);
              }
              ugrid->SetCells(VTK_VERTEX, vertices);
              vertices->Delete();
              this->NumberOfCells = GMVRead::gmv_meshdata.nnodes;
            }
            break;
        }

        progress = this->GetProgress();
        this->UpdateProgress(progress + 0.5 * (1.0 - progress));

        // Cleanup
        GMVRead::cleanupMesh();
        break;

      case (MATERIAL):
        switch (GMVRead::gmv_data.datatype)
        {
          // GMV file format documentation, page 91, states:
          // Only up to 1000 materials supported. (The GMV binary sets materials > 1000 to mod
          // 1000.)
          // So, vtkTypeInt32Array is sufficient.
          vtkTypeInt32Array* materials;
#define GMV_MAX_MATERIALS 1000
          long miL, mxL;
          unsigned long int count;

          case (NODE):
            // Find out whether material property has been selected for reading
            posInDataArray = -1;
            for (unsigned int i = 0; i < this->NumberOfNodeComponents; i++)
            {
              const char* name = this->PointDataArraySelection->GetArrayName(i);
              if (name && strncmp(name, "material id", 11) == 0)
              {
                posInDataArray = i;
                break;
              }
            }

            if (posInDataArray >= 0 &&
              this->PointDataArraySelection->GetArraySetting(posInDataArray))
            {
              materials = vtkTypeInt32Array::New();
              materials->SetNumberOfComponents(1);
              materials->SetNumberOfTuples(this->NumberOfNodes);
              materials->SetName("material id");
              GMVRead::minmax(GMVRead::gmv_data.longdata1, this->NumberOfNodes, miL, mxL);
              if (mxL > GMV_MAX_MATERIALS)
                vtkWarningMacro("Warning, there are more than "
                  << GMV_MAX_MATERIALS << " materials." << endl
                  << "   Note, materials > " << GMV_MAX_MATERIALS << " will be set to mod "
                  << GMV_MAX_MATERIALS);
              count = 0;
              for (unsigned long int i = 0; i < this->NumberOfNodes; ++i)
              {
                if (GMVRead::gmv_data.longdata1[i] > GMV_MAX_MATERIALS)
                  count++;
                // The GMV binary sets materials > 1000 to mod 1000.
                materials->SetComponent(
                  i, 0, vtkTypeInt32(GMVRead::gmv_data.longdata1[i] % GMV_MAX_MATERIALS));
              }
              if (count > 0)
                vtkWarningMacro("Warning, there are " << count << " nodes with material > "
                                                      << GMV_MAX_MATERIALS << ".");
              this->Mesh->GetPointData()->AddArray(materials);
              // VTK File Formats states that the attributes "Scalars"
              // and "Vectors" "of PointData and CellData are used to
              // specify the active arrays by name"
              if (!this->Mesh->GetPointData()->GetScalars())
                this->Mesh->GetPointData()->SetScalars(materials);
              materials->Delete();
            }
            break;

          case (CELL):
            // Find out whether material property has been selected for reading
            posInDataArray = -1;
            for (unsigned int i = 0; i < this->NumberOfCellComponents; i++)
            {
              const char* name = this->CellDataArraySelection->GetArrayName(i);
              if (name && strncmp(name, "material id", 11) == 0)
              {
                posInDataArray = i;
                break;
              }
            }

            if (posInDataArray >= 0 &&
              this->CellDataArraySelection->GetArraySetting(posInDataArray))
            {
              materials = vtkTypeInt32Array::New();
              materials->SetNumberOfComponents(1);
              materials->SetNumberOfTuples(this->NumberOfCells);
              materials->SetName("material id");
              GMVRead::minmax(GMVRead::gmv_data.longdata1, this->NumberOfCells, miL, mxL);
              if (mxL > GMV_MAX_MATERIALS)
                vtkWarningMacro("Warning, there are more than "
                  << GMV_MAX_MATERIALS << " materials." << endl
                  << "   Note, materials > " << GMV_MAX_MATERIALS << " will be set to mod "
                  << GMV_MAX_MATERIALS);
              count = 0;
              for (unsigned long int i = 0; i < this->NumberOfCells; ++i)
              {
                if (GMVRead::gmv_data.longdata1[i] > GMV_MAX_MATERIALS)
                  count++;
                // The GMV binary sets materials > 1000 to mod 1000.
                materials->SetComponent(
                  i, 0, vtkTypeInt32(GMVRead::gmv_data.longdata1[i] % GMV_MAX_MATERIALS));
              }
              if (count > 0)
                vtkWarningMacro("Warning, there are " << count << " elements with material > "
                                                      << GMV_MAX_MATERIALS << ".");
              this->Mesh->GetCellData()->AddArray(materials);
              // VTK File Formats states that the attributes "Scalars"
              // and "Vectors" "of PointData and CellData are used to
              // specify the active arrays by name"
              if (!this->Mesh->GetCellData()->GetScalars())
                this->Mesh->GetCellData()->SetScalars(materials);
              materials->Delete();
            }
            break;

          case (FACE):
            vtkErrorMacro(<< "Face based data is not supported by this reader.");
            break;
        }
        progress = this->GetProgress();
        this->UpdateProgress(progress + 0.5 * (1.0 - progress));
        GMVRead::cleanup(&GMVRead::gmv_data.chardata1);
        GMVRead::cleanup(&GMVRead::gmv_data.longdata1);
        break;

      case (VELOCITY):
        switch (GMVRead::gmv_data.datatype)
        {
          vtkFloatArray* vectors;

          case (NODE):
            // Find out whether velocity has been selected for reading
            posInDataArray = -1;
            for (unsigned int i = 0; i < this->NumberOfNodeComponents; i++)
            {
              const char* name = this->PointDataArraySelection->GetArrayName(i);
              if (name && strncmp(name, "velocity", 8) == 0)
              {
                posInDataArray = i;
                break;
              }
            }

            if (posInDataArray >= 0 &&
              this->PointDataArraySelection->GetArraySetting(posInDataArray))
            {
              vectors = vtkFloatArray::New();
              vectors->SetNumberOfComponents(3);
              vectors->SetNumberOfTuples(this->NumberOfNodes);
              vectors->SetName("velocity");
              for (unsigned long int i = 0; i < this->NumberOfNodes; ++i)
              {
                vectors->SetComponent(i, 0, GMVRead::gmv_data.doubledata1[i]);
                vectors->SetComponent(i, 1, GMVRead::gmv_data.doubledata2[i]);
                vectors->SetComponent(i, 2, GMVRead::gmv_data.doubledata3[i]);
              }
              this->Mesh->GetPointData()->AddArray(vectors);
              // VTK File Formats states that the attributes "Scalars"
              // and "Vectors" "of PointData and CellData are used to
              // specify the active arrays by name"
              if (!this->Mesh->GetPointData()->GetVectors())
                this->Mesh->GetPointData()->SetVectors(vectors);
              vectors->Delete();
            }
            break;

          case (CELL):
            // Find out whether velocity has been selected for reading
            posInDataArray = -1;
            for (unsigned int i = 0; i < this->NumberOfCellComponents; i++)
            {
              const char* name = this->CellDataArraySelection->GetArrayName(i);
              if (name && strncmp(name, "velocity", 8) == 0)
              {
                posInDataArray = i;
                break;
              }
            }

            if (posInDataArray >= 0 &&
              this->CellDataArraySelection->GetArraySetting(posInDataArray))
            {
              vectors = vtkFloatArray::New();
              vectors->SetNumberOfComponents(3);
              vectors->SetNumberOfTuples(this->NumberOfCells);
              vectors->SetName("velocity");
              for (unsigned long int i = 0; i < this->NumberOfCells; ++i)
              {
                vectors->SetComponent(i, 0, GMVRead::gmv_data.doubledata1[i]);
                vectors->SetComponent(i, 1, GMVRead::gmv_data.doubledata2[i]);
                vectors->SetComponent(i, 2, GMVRead::gmv_data.doubledata3[i]);
              }
              this->Mesh->GetCellData()->AddArray(vectors);
              // VTK File Formats states that the attributes "Scalars"
              // and "Vectors" "of PointData and CellData are used to
              // specify the active arrays by name"
              if (!this->Mesh->GetCellData()->GetVectors())
                this->Mesh->GetCellData()->SetVectors(vectors);
              vectors->Delete();
            }
            break;

          case (FACE):
            vtkErrorMacro(<< "Face based data is not supported by this reader.");
            break;
        }
        progress = this->GetProgress();
        this->UpdateProgress(progress + 0.5 * (1.0 - progress));
        GMVRead::cleanup(&GMVRead::gmv_data.doubledata1);
        GMVRead::cleanup(&GMVRead::gmv_data.doubledata2);
        GMVRead::cleanup(&GMVRead::gmv_data.doubledata3);
        break;

      case (VECTORS):
        switch (GMVRead::gmv_data.datatype)
        {
          vtkFloatArray* vectors;

          case (NODE):
            // Find out whether velocity has been selected for reading
            posInDataArray = -1;
            for (unsigned int i = 0; i < this->NumberOfNodeComponents; i++)
            {
              const char* name = this->PointDataArraySelection->GetArrayName(i);
              if (name &&
                strncmp(name, GMVRead::gmv_data.name1, strlen(GMVRead::gmv_data.name1)) == 0)
              {
                posInDataArray = i;
                break;
              }
            }

            if (posInDataArray >= 0 &&
              this->PointDataArraySelection->GetArraySetting(posInDataArray))
            {
              vectors = vtkFloatArray::New();
              vectors->SetNumberOfComponents(GMVRead::gmv_data.num2);
              vectors->SetNumberOfTuples(this->NumberOfNodes);
              vectors->SetName(GMVRead::gmv_data.name1);
              // VTK has support for named components not before Mon Apr 5 10:14:33 2010 -0400,
              // commit 3632f9ac5e7cb0aa611b254a28a41901fb3c2366
              for (long i = 0; i < GMVRead::gmv_data.num2; ++i)
              {
                // GMV format allows vector components to be named:
                // If they are, the vector's name is ignored by GMV.
                // If they are not, gmvread.c assigns them generic names of format
                // "{digit}-{vectorname}".
                // Either way, ParaView handles things differently: It always prefixes the vector's
                // name to every component. Try to reduce that possible redundancy by removing
                // redundant substrings:
                // * For auto-generated component names strip the trailing vector name
                // * For explicitly named components, try to remove a leading vector name plus an
                //   underscore - because users tend to follow the pattern from the GMV User's
                //   Manual which states:
                //        vectors
                //        momentum     1     3     1
                //        momentum_X momentum_Y momentum_Z
                //   where the "momentum_" substring can be omitted.
                size_t pos, n;
                std::string vectorname = GMVRead::gmv_data.name1;
                std::string componentname = &GMVRead::gmv_data.chardata1[MAXCUSTOMNAMELENGTH * i];
                // Try to strip vector name from auto-generated component names
                pos = componentname.rfind("-" + vectorname);
                if (pos != std::string::npos)
                {
                  n = vectorname.length() + 1;
                  componentname.erase(pos, n);
                }
                else
                {
                  // Try to strip vector name from explicitly named components to avoid redundantly
                  // named variables
                  pos = componentname.find(vectorname + "_");
                  if (pos != std::string::npos)
                  {
                    n = vectorname.length() + 1;
                    componentname.erase(pos, n);
                  }
                }
                vectors->SetComponentName(i, componentname.c_str());
              }
              for (long j = 0; j < GMVRead::gmv_data.num2; j++)
              {
                for (unsigned long int i = 0; i < this->NumberOfNodes; ++i)
                {
                  vectors->SetComponent(
                    i, j, GMVRead::gmv_data.doubledata1[j * this->NumberOfNodes + i]);
                }
              }
              this->Mesh->GetPointData()->AddArray(vectors);
              if (GMVRead::gmv_data.num2 == 1)
              {
                if (!this->Mesh->GetPointData()->GetScalars())
                  this->Mesh->GetPointData()->SetScalars(vectors);
              }
              else if (GMVRead::gmv_data.num2 == 3)
              {
                // vtkDataSetAttributes.cxx only allows 3-component vectors for
                // SetVectors(), see entry of array NumberOfAttributeComponents with index
                // as assigned to "VECTORS" in "enum AttributeTypes"

                // VTK File Formats states that the attributes "Scalars"
                // and "Vectors" "of PointData and CellData are used to
                // specify the active arrays by name"
                if (!this->Mesh->GetPointData()->GetVectors())
                  this->Mesh->GetPointData()->SetVectors(vectors);
              }
              vectors->Delete();
            }
            break;

          case (CELL):
            // Find out whether velocity has been selected for reading
            posInDataArray = -1;
            for (unsigned int i = 0; i < this->NumberOfCellComponents; i++)
            {
              const char* name = this->CellDataArraySelection->GetArrayName(i);
              if (name &&
                strncmp(name, GMVRead::gmv_data.name1, strlen(GMVRead::gmv_data.name1)) == 0)
              {
                posInDataArray = i;
                break;
              }
            }

            if (posInDataArray >= 0 &&
              this->CellDataArraySelection->GetArraySetting(posInDataArray))
            {
              vectors = vtkFloatArray::New();
              vectors->SetNumberOfComponents(GMVRead::gmv_data.num2);
              vectors->SetNumberOfTuples(this->NumberOfCells);
              vectors->SetName(GMVRead::gmv_data.name1);
              // VTK has support for named components not before Mon Apr 5 10:14:33 2010 -0400,
              // commit 3632f9ac5e7cb0aa611b254a28a41901fb3c2366
              for (long i = 0; i < GMVRead::gmv_data.num2; ++i)
              {
                // GMV format allows vector components to be named:
                // If they are, the vector's name is ignored by GMV.
                // If they are not, gmvread.c assigns them generic names of format
                // "{digit}-{vectorname}".
                // Either way, ParaView handles things differently: It always prefixes the vector's
                // name to every component. Try to reduce that possible redundancy by removing
                // redundant substrings:
                // * For auto-generated component names strip the trailing vector name plus the dash
                // * For explicitly named components, try to remove a leading vector name plus an
                //   underscore - because users tend to follow the pattern from the GMV User's
                //   Manual which states:
                //        vectors
                //        momentum     1     3     1
                //        momentum_X momentum_Y momentum_Z
                //   where the "momentum_" substring can be omitted.
                size_t pos, n;
                std::string vectorname = GMVRead::gmv_data.name1;
                std::string componentname = &GMVRead::gmv_data.chardata1[MAXCUSTOMNAMELENGTH * i];
                // Try to strip vector name from auto-generated component names
                pos = componentname.rfind("-" + vectorname);
                if (pos != std::string::npos)
                {
                  n = vectorname.length() + 1;
                  componentname.erase(pos, n);
                }
                else
                {
                  // Try to strip vector name from explicitly named components to avoid redundantly
                  // named variables
                  pos = componentname.find(vectorname + "_");
                  if (pos != std::string::npos)
                  {
                    n = vectorname.length() + 1;
                    componentname.erase(pos, n);
                  }
                }
                vectors->SetComponentName(i, componentname.c_str());
              }
              for (long j = 0; j < GMVRead::gmv_data.num2; j++)
              {
                for (unsigned long int i = 0; i < this->NumberOfCells; ++i)
                {
                  vectors->SetComponent(
                    i, j, GMVRead::gmv_data.doubledata1[j * this->NumberOfCells + i]);
                }
              }
              this->Mesh->GetCellData()->AddArray(vectors);
              if (GMVRead::gmv_data.num2 == 1)
              {
                if (!this->Mesh->GetCellData()->GetScalars())
                  this->Mesh->GetCellData()->SetScalars(vectors);
              }
              else if (GMVRead::gmv_data.num2 == 3)
              {
                // vtkDataSetAttributes.cxx only allows 3-component vectors for
                // SetVectors(), see entry of array NumberOfAttributeComponents with index
                // as assigned to "VECTORS" in "enum AttributeTypes"

                // VTK File Formats states that the attributes "Scalars"
                // and "Vectors" "of PointData and CellData are used to
                // specify the active arrays by name"
                if (!this->Mesh->GetCellData()->GetVectors())
                  this->Mesh->GetCellData()->SetVectors(vectors);
              }
              vectors->Delete();
            }
            break;

          case (FACE):
            vtkErrorMacro(<< "Face based data is not supported by this reader.");
            break;

          case (ENDKEYWORD):
            // End of variable data block
            break;
        }
        progress = this->GetProgress();
        this->UpdateProgress(progress + 0.5 * (1.0 - progress));
        GMVRead::cleanupAllData();
        break;

      case (VARIABLE):
        switch (GMVRead::gmv_data.datatype)
        {
          vtkFloatArray* scalars;

          case (NODE):
            // Find out whether this variable has been selected for reading
            posInDataArray = -1;
            for (unsigned int i = 0; i < this->NumberOfNodeComponents; i++)
            {
              const char* name = this->PointDataArraySelection->GetArrayName(i);
              if (name &&
                strncmp(name, GMVRead::gmv_data.name1, strlen(GMVRead::gmv_data.name1)) == 0)
              {
                posInDataArray = i;
                break;
              }
            }

            if (posInDataArray >= 0 &&
              this->PointDataArraySelection->GetArraySetting(posInDataArray))
            {
              scalars = vtkFloatArray::New();
              scalars->SetNumberOfComponents(1);
              scalars->SetNumberOfTuples(this->NumberOfNodes);
              scalars->SetName(GMVRead::gmv_data.name1);
              for (unsigned long int i = 0; i < this->NumberOfNodes; ++i)
                scalars->SetComponent(i, 0, GMVRead::gmv_data.doubledata1[i]);
              this->Mesh->GetPointData()->AddArray(scalars);
              // VTK File Formats states that the attributes "Scalars"
              // and "Vectors" "of PointData and CellData are used to
              // specify the active arrays by name"
              if (!this->Mesh->GetPointData()->GetScalars())
                this->Mesh->GetPointData()->SetScalars(scalars);
              scalars->Delete();
            }
            break;

          case (CELL):
            // Find out whether variable has been selected for reading
            posInDataArray = -1;
            for (unsigned int i = 0; i < this->NumberOfCellComponents; i++)
            {
              const char* name = this->CellDataArraySelection->GetArrayName(i);
              if (name &&
                strncmp(name, GMVRead::gmv_data.name1, strlen(GMVRead::gmv_data.name1)) == 0)
              {
                posInDataArray = i;
                break;
              }
            }

            if (posInDataArray >= 0 &&
              this->CellDataArraySelection->GetArraySetting(posInDataArray))
            {
              scalars = vtkFloatArray::New();
              scalars->SetNumberOfComponents(1);
              scalars->SetNumberOfTuples(this->NumberOfCells);
              scalars->SetName(GMVRead::gmv_data.name1);
              for (unsigned long int i = 0; i < this->NumberOfCells; ++i)
                scalars->SetComponent(i, 0, GMVRead::gmv_data.doubledata1[i]);
              this->Mesh->GetCellData()->AddArray(scalars);
              // VTK File Formats states that the attributes "Scalars"
              // and "Vectors" "of PointData and CellData are used to
              // specify the active arrays by name"
              if (!this->Mesh->GetCellData()->GetScalars())
                this->Mesh->GetCellData()->SetScalars(scalars);
              scalars->Delete();
            }
            break;

          case (FACE):
            vtkErrorMacro(<< "Face based data is not supported by this reader.");
            break;

          case (ENDKEYWORD):
            // End of variable data block
            break;
        }
        progress = this->GetProgress();
        this->UpdateProgress(progress + 0.5 * (1.0 - progress));
        GMVRead::cleanupAllData();
        break;

      case (FLAGS):
      {
        int flagNameLen = (int)strlen(GMVRead::gmv_data.name1) + 5;
        char* flagName = new char[flagNameLen + 1];
        strncpy(&flagName[0], (char*)"flag ", 6);
        strcpy(&flagName[5], GMVRead::gmv_data.name1);
        flagName[flagNameLen] = '\0';

        switch (GMVRead::gmv_data.datatype)
        {
          vtkStringArray* flags;

          case (NODE):
            // Find out whether this variable has been selected for reading
            posInDataArray = -1;
            for (unsigned int i = 0; i < this->NumberOfNodeComponents; i++)
            {
              const char* name = this->PointDataArraySelection->GetArrayName(i);
              if (name && strncmp(name, flagName, flagNameLen) == 0)
              {
                posInDataArray = i;
                break;
              }
            }

            if (posInDataArray >= 0 &&
              this->PointDataArraySelection->GetArraySetting(posInDataArray))
            {
              flags = vtkStringArray::New();
              flags->SetNumberOfComponents(1);
              flags->SetNumberOfTuples(this->NumberOfNodes);
              flags->SetName(flagName);

              for (unsigned long int i = 0; i < this->NumberOfNodes; ++i)
                // -1 because GMV file format starts to count from 1 while here we start from 0
                flags->SetValue(
                  i, &GMVRead::gmv_data
                        .chardata1[(GMVRead::gmv_data.longdata1[i] - 1) * MAXCUSTOMNAMELENGTH]);
              this->Mesh->GetPointData()->AddArray(flags);
              flags->Delete();
            }
            break;

          case (CELL):
            // Find out whether this variable has been selected for reading
            posInDataArray = -1;
            for (unsigned int i = 0; i < this->NumberOfCellComponents; i++)
            {
              const char* name = this->CellDataArraySelection->GetArrayName(i);
              if (name && strncmp(name, flagName, flagNameLen) == 0)
              {
                posInDataArray = i;
                break;
              }
            }

            if (posInDataArray >= 0 &&
              this->CellDataArraySelection->GetArraySetting(posInDataArray))
            {
              flags = vtkStringArray::New();
              flags->SetNumberOfComponents(1);
              flags->SetNumberOfTuples(this->NumberOfCells);
              flags->SetName(flagName);

              for (unsigned long int i = 0; i < this->NumberOfCells; ++i)
                // -1 because GMV file format starts to count from 1 while here we start from 0
                flags->SetValue(
                  i, &GMVRead::gmv_data
                        .chardata1[(GMVRead::gmv_data.longdata1[i] - 1) * MAXCUSTOMNAMELENGTH]);
              this->Mesh->GetCellData()->AddArray(flags);
              flags->Delete();
            }
            break;
        }
        delete[] flagName;
      }
        progress = this->GetProgress();
        this->UpdateProgress(progress + 0.5 * (1.0 - progress));
        GMVRead::cleanupAllData();
        break;

      case (POLYGONS):
        if (this->ImportPolygons)
        {
          vtkDebugMacro(
            "GMVReader::RequestData: Reading polygon points from file " << this->FileName);

          // Lookup number of polygons, stored in previous RequestInformation() call
          this->NumberOfPolygons = this->NumberOfPolygonsMap[this->FileName];

          if (!firstPolygonParsed)
          {
            // Pre-allocate polygonal dataset for polygons.
            // This compensates for the fact that the GMV file format does not
            // specify the number of polygons beforehand. Luckily, we already parsed
            // all files in RequestInformation() and stored the number per file.
            blockNo = output->GetNumberOfBlocks();

            pd = vtkPolyData::New();
            pd->Allocate(this->NumberOfPolygons);
            output->SetBlock(blockNo, pd);
            this->Polygons = pd;
            output->GetMetaData(blockNo)->Set(vtkCompositeDataSet::NAME(), "Polygons");

            polygonPoints = vtkPoints::New();
            polygonCells = vtkCellArray::New();

            // Find out whether material property has been selected for reading.
            // Done once before reading the first polygon and re-used for
            // every polygon definition.
            polygonMaterialPosInDataArray = -1;
            for (unsigned int i = 0; i < this->NumberOfNodeComponents; i++)
            {
              const char* name = this->CellDataArraySelection->GetArrayName(i);
              if (name && strncmp(name, "material id", 11) == 0)
              {
                polygonMaterialPosInDataArray = i;
                break;
              }
            }

            if (polygonMaterialPosInDataArray >= 0 &&
              this->CellDataArraySelection->GetArraySetting(polygonMaterialPosInDataArray))
            {
              polygonMaterials = vtkTypeInt64Array::New();
              polygonMaterials->SetNumberOfComponents(1);
              polygonMaterials->SetNumberOfTuples(this->NumberOfPolygons);
              polygonMaterials->SetName("material id");
            }

            firstPolygonParsed = true;
          }

          switch (GMVRead::gmv_data.datatype)
          {
            unsigned int npts;

            case (REGULAR):
              npts = int(GMVRead::gmv_data.ndoubledata1);
              vtkDebugMacro(
                "GMVReader::RequestData: Found " << npts << " points for polygon definition ");

              // Number of points polygon cell consists of
              polygonCells->InsertNextCell(npts);

              for (unsigned int i = 0; i < npts; i++)
              {
                // Insert polygon points
                vtkIdType id = polygonPoints->InsertNextPoint(GMVRead::gmv_data.doubledata1[i],
                  GMVRead::gmv_data.doubledata2[i], GMVRead::gmv_data.doubledata3[i]);
                // Add point to cell definition
                polygonCells->InsertCellPoint(id);
              }

              if (polygonMaterialPosInDataArray >= 0 &&
                this->CellDataArraySelection->GetArraySetting(polygonMaterialPosInDataArray))
              {
                vtkDebugMacro("GMVReader::RequestData: Polygon #"
                  << polygonCells->GetNumberOfCells() << "  material #" << GMVRead::gmv_data.num
                  << "  total polygon #" << this->NumberOfPolygons);
                polygonMaterials->SetComponent(
                  polygonCells->GetNumberOfCells() - 1, 0, vtkTypeInt64(GMVRead::gmv_data.num));
              }

              break;
            case (ENDKEYWORD):
              // Set polygon points and cells
              this->Polygons->SetPoints(polygonPoints);
              this->Polygons->SetPolys(polygonCells); // essential for calling BuildCells()
              this->Polygons
                ->BuildCells(); // mandatory to be able to call vtkPolyData::GetCellPoints,
                                // e.g. when using CleantoGrid filter.
              polygonPoints->Delete();
              polygonCells->Delete();

              if (polygonMaterialPosInDataArray >= 0 &&
                this->CellDataArraySelection->GetArraySetting(polygonMaterialPosInDataArray))
              {
                this->Polygons->GetCellData()->AddArray(polygonMaterials);
                // VTK File Formats states that the attributes "Scalars"
                // and "Vectors" "of PointData and CellData are used to
                // specify the active arrays by name"
                if (!this->Polygons->GetCellData()->GetScalars())
                  this->Polygons->GetCellData()->SetScalars(polygonMaterials);

                polygonMaterials->Delete();
              }

              break;
          }
        }

        progress = this->GetProgress();
        this->UpdateProgress(progress + 0.5 * (1.0 - progress));

        GMVRead::cleanupAllData();
        break;

      case (TRACERS):
        if (this->ImportTracers)
        {
          switch (GMVRead::gmv_data.datatype)
          {

            case (XYZ):
              points = vtkPoints::New();
              tracerCells = vtkCellArray::New();

              // Extract data
              // Number of tracers
              this->NumberOfTracers = GMVRead::gmv_data.num;

              blockNo = output->GetNumberOfBlocks();
              pd = vtkPolyData::New();
              pd->Allocate(this->NumberOfTracers);
              output->SetBlock(blockNo, pd);
              this->Tracers = pd;
              output->GetMetaData(blockNo)->Set(vtkCompositeDataSet::NAME(), "Tracers");

              // Coordinates of tracer points
              for (unsigned long i = 0; i < this->NumberOfTracers; i++)
              {
                // Insert tracer point
                vtkIdType id = points->InsertNextPoint(GMVRead::gmv_data.doubledata1[i],
                  GMVRead::gmv_data.doubledata2[i], GMVRead::gmv_data.doubledata3[i]);

#if 0
                // variant 1:
                // Insert cell consisting of that tracer point
                vtkVertex * pointCell = vtkVertex::New();
                pointCell->GetPointIds()->SetId(0, i);
                this->Tracers->InsertNextCell(pointCell->GetCellType(),
                                              pointCell->GetPointIds());
                pointCell->Delete();
#endif

                // variant 2:
                tracerCells->InsertNextCell(1, &id);

#if 0
                // variant 3:
                // Number of points tracer cell consists of
                tracerCells->InsertNextCell(1);
                tracerCells->InsertCellPoint(id);
#endif
              }
              // Set tracer points
              this->Tracers->SetPoints(points);
              this->Tracers->SetVerts(
                tracerCells); // important to have tracer cells appear as points
              // this->Tracers->SetPolys(tracerCells);  // seems not necessary for tracer cells
              this->Tracers
                ->BuildCells(); // mandatory to be able to call vtkPolyData::GetCellPoints,
                                // e.g. when using CleantoGrid filter.
              points->Delete();
              tracerCells->Delete();

              progress = this->GetProgress();
              this->UpdateProgress(progress + 0.5 * (1.0 - progress));

              GMVRead::cleanup(&GMVRead::gmv_data.doubledata1);
              GMVRead::cleanup(&GMVRead::gmv_data.doubledata2);
              GMVRead::cleanup(&GMVRead::gmv_data.doubledata3);
              break;

            case (TRACERDATA):
            {
              // Find out whether this variable has been selected for reading
              int tracerNameLen = (int)strlen(GMVRead::gmv_data.name1) + 7;
              char* tracerName = new char[tracerNameLen + 1];
              strncpy(&tracerName[0], (char*)"tracer ", 8);
              strcpy(&tracerName[7], GMVRead::gmv_data.name1);
              tracerName[tracerNameLen] = '\0';

              posInDataArray = -1;
              for (unsigned int i = 0; i < this->NumberOfNodeComponents; i++)
              {
                const char* selectedName = this->PointDataArraySelection->GetArrayName(i);
                if (selectedName && strncmp(selectedName, tracerName, tracerNameLen) == 0)
                {
                  posInDataArray = i;
                  break;
                }
              }

              if (posInDataArray >= 0 &&
                this->PointDataArraySelection->GetArraySetting(posInDataArray))
              {
                vtkFloatArray* tracerField = vtkFloatArray::New();
                tracerField->SetNumberOfComponents(1);
                tracerField->SetNumberOfTuples(this->NumberOfTracers);
                tracerField->SetName(tracerName);

                for (unsigned long int i = 0; i < this->NumberOfTracers; i++)
                {
                  tracerField->SetComponent(i, 0, GMVRead::gmv_data.doubledata1[i]);
                }

                this->Tracers->GetPointData()->AddArray(tracerField);
                // VTK File Formats states that the attributes "Scalars"
                // and "Vectors" "of PointData and CellData are used to
                // specify the active arrays by name"
                if (!this->Tracers->GetPointData()->GetScalars())
                  this->Tracers->GetPointData()->SetScalars(tracerField);
                tracerField->Delete();
              }
              delete[] tracerName;

              progress = this->GetProgress();
              this->UpdateProgress(progress + 0.5 * (1.0 - progress));

              GMVRead::cleanup(&GMVRead::gmv_data.doubledata1);
              break;
            }

            case (ENDKEYWORD):
              GMVRead::cleanupAllData();
              break;
          }
        }
        break;

      case (TRACEIDS):
        if (this->ImportTracers)
        {
          // Find out whether material property has been selected for reading
          posInDataArray = -1;
          for (unsigned int i = 0; i < this->NumberOfNodeComponents; i++)
          {
            const char* name = this->PointDataArraySelection->GetArrayName(i);
            if (name && strncmp(name, "tracer id", 9) == 0)
            {
              posInDataArray = i;
              break;
            }
          }

          if (posInDataArray >= 0 && this->PointDataArraySelection->GetArraySetting(posInDataArray))
          {
            vtkTypeInt64Array* tracerIds = vtkTypeInt64Array::New();
            tracerIds->SetNumberOfComponents(1);
            tracerIds->SetNumberOfTuples(this->NumberOfTracers);
            tracerIds->SetName("tracer id");

            for (unsigned long int i = 0; i < this->NumberOfTracers; i++)
            {
              tracerIds->SetComponent(i, 0, vtkTypeInt64(GMVRead::gmv_data.longdata1[i]));
            }

            this->Tracers->GetPointData()->AddArray(tracerIds);
            // VTK File Formats states that the attributes "Scalars"
            // and "Vectors" "of PointData and CellData are used to
            // specify the active arrays by name"
            if (!this->Tracers->GetPointData()->GetScalars())
              this->Tracers->GetPointData()->SetScalars(tracerIds);
            tracerIds->Delete();
          }
        }

        GMVRead::cleanupAllData();
        break;

      case (PROBTIME):
        GMVRead::cleanupAllData();
        break;

      case (CYCLENO):
        // Find out whether this variable has been selected for reading
        posInDataArray = -1;
        for (unsigned int i = 0; i < this->NumberOfFieldComponents; i++)
        {
          const char* name = this->FieldDataArraySelection->GetArrayName(i);
          if (name && strncmp(name, "cycle number", strlen("cycle number")) == 0)
          {
            posInDataArray = i;
            break;
          }
        }

        if (posInDataArray >= 0 && this->FieldDataArraySelection->GetArraySetting(posInDataArray))
        {
          // According to the GMV documentation, only CODENAME, CODEVER
          // and SIMDATE are allowed before NODES, CELLS etc. and indeed
          // gmvread.c complains if CYCLENO appears prior to them. We
          // support a premature location anyway.
          // But prior to having parsed the keyword NODES, there is no
          // mesh allocated yet. Store the data in a temporary array and
          // move it to its final location before leaving this routine.
          if (this->Mesh)
          {
            fieldptr = this->Mesh->GetFieldData();
          }
          else
          {
            // Use temporary structure to store data
            fieldptr = this->FieldDataTmp;
          }

          if (!fieldptr)
          {
            fieldptr = vtkFieldData::New();
            fieldptr->Allocate(0);
            this->FieldDataTmp = fieldptr;
          }

          vtkUnsignedIntArray* name = vtkUnsignedIntArray::New();
          name->SetNumberOfValues(1);
          name->SetName("cycle number");
          name->InsertValue(0, (unsigned int)GMVRead::gmv_data.num);
          fieldptr->AddArray(name);
          name->Delete();
        }

        GMVRead::cleanupAllData();
        break;

      case (NODEIDS):
        switch (GMVRead::gmv_data.datatype)
        {
          vtkTypeInt64Array* nodeids;

          case (REGULAR):
            // Find out whether this variable has been selected for reading
            posInDataArray = -1;
            for (unsigned int i = 0; i < this->NumberOfNodeComponents; i++)
            {
              const char* name = this->PointDataArraySelection->GetArrayName(i);
              if (name &&
                strncmp(name, "Point IDs (Alternate)", strlen("Point IDs (Alternate)")) == 0)
              {
                posInDataArray = i;
                break;
              }
            }

            if (posInDataArray >= 0 &&
              this->PointDataArraySelection->GetArraySetting(posInDataArray))
            {
              nodeids = vtkTypeInt64Array::New();
              nodeids->SetNumberOfComponents(1);
              nodeids->SetNumberOfTuples(this->NumberOfNodes);
              nodeids->SetName("Point IDs (Alternate)");
              for (unsigned long int i = 0; i < this->NumberOfNodes; ++i)
                nodeids->SetComponent(i, 0, vtkTypeInt64(GMVRead::gmv_data.longdata1[i]));
              this->Mesh->GetPointData()->AddArray(nodeids);
              // VTK File Formats states that the attributes "Scalars"
              // and "Vectors" "of PointData and CellData are used to
              // specify the active arrays by name"
              if (!this->Mesh->GetPointData()->GetScalars())
                this->Mesh->GetPointData()->SetScalars(nodeids);
              nodeids->Delete();
            }
            break;
        }

        GMVRead::cleanupAllData();
        break;

      case (CELLIDS):
        switch (GMVRead::gmv_data.datatype)
        {
          vtkTypeInt64Array* cellids;

          case (REGULAR):
            // Find out whether this variable has been selected for reading
            posInDataArray = -1;
            for (unsigned int i = 0; i < this->NumberOfCellComponents; i++)
            {
              const char* name = this->CellDataArraySelection->GetArrayName(i);
              if (name &&
                strncmp(name, "Cell IDs (Alternate)", strlen("Cell IDs (Alternate)")) == 0)
              {
                posInDataArray = i;
                break;
              }
            }

            if (posInDataArray >= 0 &&
              this->CellDataArraySelection->GetArraySetting(posInDataArray))
            {
              cellids = vtkTypeInt64Array::New();
              cellids->SetNumberOfComponents(1);
              cellids->SetNumberOfTuples(this->NumberOfCells);
              cellids->SetName("Cell IDs (Alternate)");
              for (unsigned long int i = 0; i < this->NumberOfCells; ++i)
                cellids->SetComponent(i, 0, vtkTypeInt64(GMVRead::gmv_data.longdata1[i]));
              this->Mesh->GetCellData()->AddArray(cellids);
              // VTK File Formats states that the attributes "Scalars"
              // and "Vectors" "of PointData and CellData are used to
              // specify the active arrays by name"
              if (!this->Mesh->GetCellData()->GetScalars())
                this->Mesh->GetCellData()->SetScalars(cellids);
              cellids->Delete();
            }
            break;
        }

        GMVRead::cleanupAllData();
        break;

      case (SURFACE):
        GMVRead::cleanupAllData();
        break;

      case (SURFMATS):
        GMVRead::cleanupAllData();
        break;

      case (SURFVEL):
        GMVRead::cleanupAllData();
        break;

      case (SURFVARS):
        GMVRead::cleanupAllData();
        break;

      case (SURFFLAG):
        GMVRead::cleanupAllData();
        break;

      case (UNITS):
        GMVRead::cleanupAllData();
        break;

      case (VINFO):
        GMVRead::cleanupAllData();
        break;

      case (GROUPS):
        GMVRead::cleanupAllData();
        break;

      case (FACEIDS):
        GMVRead::cleanupAllData();
        break;

      case (SURFIDS):
        GMVRead::cleanupAllData();
        break;

      case (SUBVARS):
        GMVRead::cleanupAllData();
        break;

      case (CODENAME):
        // Find out whether this variable has been selected for reading
        posInDataArray = -1;
        for (unsigned int i = 0; i < this->NumberOfFieldComponents; i++)
        {
          const char* name = this->FieldDataArraySelection->GetArrayName(i);
          if (name && strncmp(name, "code name", strlen("code name")) == 0)
          {
            posInDataArray = i;
            break;
          }
        }

        if (posInDataArray >= 0 && this->FieldDataArraySelection->GetArraySetting(posInDataArray))
        {

          // The keywords CODENAME, CODEVER and SIMDATE are allowed before
          // NODES, CELLS etc. Prior to having parsed the keyword NODES,
          // there is no mesh allocated yet. Store the data in a
          // temporary array and move it to its final location before
          // leaving this routine.
          if (this->Mesh)
          {
            fieldptr = this->Mesh->GetFieldData();
          }
          else
          {
            // Use temporary structure to store data
            fieldptr = this->FieldDataTmp;
          }

          if (!fieldptr)
          {
            fieldptr = vtkFieldData::New();
            fieldptr->Allocate(0);
            this->FieldDataTmp = fieldptr;
          }

          vtkStringArray* name = vtkStringArray::New();
          name->SetNumberOfValues(1);
          name->SetName("code name");
          name->InsertValue(0, GMVRead::gmv_data.name1);
          fieldptr->AddArray(name);
          name->Delete();
        }

        GMVRead::cleanupAllData();
        break;

      case (CODEVER):
        // Find out whether this variable has been selected for reading
        posInDataArray = -1;
        for (unsigned int i = 0; i < this->NumberOfFieldComponents; i++)
        {
          const char* name = this->FieldDataArraySelection->GetArrayName(i);
          if (name && strncmp(name, "code version", strlen("code version")) == 0)
          {
            posInDataArray = i;
            break;
          }
        }

        if (posInDataArray >= 0 && this->FieldDataArraySelection->GetArraySetting(posInDataArray))
        {

          // The keywords CODENAME, CODEVER and SIMDATE are allowed before
          // NODES, CELLS etc. Prior to having parsed the keyword NODES,
          // there is no mesh allocated yet. Store the data in a
          // temporary array and move it to its final location before
          // leaving this routine.
          if (this->Mesh)
          {
            fieldptr = this->Mesh->GetFieldData();
          }
          else
          {
            // Use temporary structure to store data
            fieldptr = this->FieldDataTmp;
          }

          if (!fieldptr)
          {
            fieldptr = vtkFieldData::New();
            fieldptr->Allocate(0);
            this->FieldDataTmp = fieldptr;
          }

          vtkStringArray* name = vtkStringArray::New();
          name->SetNumberOfValues(1);
          name->SetName("code version");
          name->InsertValue(0, GMVRead::gmv_data.name1);
          fieldptr->AddArray(name);
          name->Delete();
        }

        GMVRead::cleanupAllData();
        break;

      case (SIMDATE):
        // Find out whether this variable has been selected for reading
        posInDataArray = -1;
        for (unsigned int i = 0; i < this->NumberOfFieldComponents; i++)
        {
          const char* name = this->FieldDataArraySelection->GetArrayName(i);
          if (name && strncmp(name, "simulation date", strlen("simulation date")) == 0)
          {
            posInDataArray = i;
            break;
          }
        }

        if (posInDataArray >= 0 && this->FieldDataArraySelection->GetArraySetting(posInDataArray))
        {
          // The keywords CODENAME, CODEVER and SIMDATE are allowed before
          // NODES, CELLS etc. Prior to having parsed the keyword NODES,
          // there is no mesh allocated yet. Store the data in a
          // temporary array and move it to its final location before
          // leaving this routine.
          if (this->Mesh)
          {
            fieldptr = this->Mesh->GetFieldData();
          }
          else
          {
            // Use temporary structure to store data
            fieldptr = this->FieldDataTmp;
          }

          if (!fieldptr)
          {
            fieldptr = vtkFieldData::New();
            fieldptr->Allocate(0);
            this->FieldDataTmp = fieldptr;
          }

          vtkStringArray* name = vtkStringArray::New();
          name->SetNumberOfValues(1);
          name->SetName("simulation date");
          name->InsertValue(0, GMVRead::gmv_data.name1);
          fieldptr->AddArray(name);
          name->Delete();
        }

        GMVRead::cleanupAllData();
        break;

      default:
        GMVRead::cleanupAllData();
        break;
    }
  }

  // Move field data from temporary structure to mesh, if required
  if (this->FieldDataTmp)
  {
    vtkAbstractArray *data, *newData;
    for (int i = 0; i < FieldDataTmp->GetNumberOfArrays(); i++)
    {
      data = FieldDataTmp->GetAbstractArray(i);
      newData = data->NewInstance(); // instantiate same type of object
      newData->DeepCopy(data);
      newData->SetName(data->GetName());
      if (data->HasInformation())
      {
        newData->CopyInformation(data->GetInformation(), /*deep=*/1);
      }
      this->Mesh->GetFieldData()->AddArray(newData);
      newData->Delete();
    }
    this->FieldDataTmp->Delete();
    this->FieldDataTmp = NULL;
  }

  this->UpdateProgress(1.0);
  return 1;
}

//----------------------------------------------------------------------------
int vtkGMVReader::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
#if VTK_MODULE_ENABLE_VTK_ParallelCore
  if (this->Controller)
  {
    if (this->Controller->GetNumberOfProcesses() > 1)
    {
      vtkWarningMacro(
        "GMVReader is not parallel-aware: all pvserver processes will read the entire file!");
    }
  }
#endif

  vtkDebugMacro(<< "GMVReader::RequestInformation: Parsing file " << this->FileName
                << " for fields, #polygons and time steps");
  int ierr = GMVRead::gmvread_open(this->FileName);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (ierr > 0)
  {
    if (GMVRead::gmv_data.errormsg != NULL)
    {
      vtkErrorMacro("" << GMVRead::gmv_data.errormsg);
    }
    else
    {
      vtkErrorMacro("GMV reader library reported an unhandled error.");
    }
    // this->SetupEmptyOutput();
    // this->CurrentOutput = 0;
    return 0;
  }
  if (ierr != -1)
    this->BinaryFile = 1;

  double timeStepValue = 0.0;
  bool keepParsing = true;
  this->NumberOfNodeFields = 0;
  this->NumberOfCellFields = 0;
  this->NumberOfFields = 0;
  this->NumberOfNodeComponents = 0;
  this->NumberOfCellComponents = 0;
  this->NumberOfFieldComponents = 0;
  this->NumberOfPolygons = 0;
  this->NumberOfTracers = 0;

  while (keepParsing)
  {
    GMVRead::gmvread_data();
    switch (GMVRead::gmv_data.keyword)
    {
      case (GMVERROR):
        if (GMVRead::gmv_data.errormsg != NULL)
        {
          vtkErrorMacro("" << GMVRead::gmv_data.errormsg);
        }
        else
        {
          vtkErrorMacro("GMV reader library reported an unhandled error.");
        }
        GMVRead::gmvread_close();
        // this->SetupEmptyOutput();
        // this->CurrentOutput = 0;
        return 0;

      case (GMVEND):
        keepParsing = false;
        GMVRead::gmvread_close();
        break;

      case (NODES):
        switch (GMVRead::gmv_data.datatype)
        {
          case (UNSTRUCT):
            // supported
            break;
          case (STRUCT):
            // supported
            break;
          case (LOGICALLY_STRUCT):
            // supported
            break;
        }

        GMVRead::gmvread_mesh();
        // An i/o error may have occurred
        if (GMVRead::gmv_meshdata.intype == GMVERROR)
        {
          if (GMVRead::gmv_data.errormsg != NULL)
          {
            vtkErrorMacro("" << GMVRead::gmv_data.errormsg);
          }
          else
          {
            vtkErrorMacro("GMV reader library reported an unhandled error.");
          }
          // this->SetupEmptyOutput();
          // this->CurrentOutput = 0;
          return 0;
        }

        // Extract data
        this->NumberOfNodes = GMVRead::gmv_meshdata.nnodes;
        this->NumberOfCells = GMVRead::gmv_meshdata.ncells;
        // Cleanup
        GMVRead::cleanupMesh();
        break;

      case (MATERIAL):
        switch (GMVRead::gmv_data.datatype)
        {
          case (NODE):
            this->NumberOfNodeFields += 1;
            this->NumberOfNodeComponents += 1;
            this->PointDataArraySelection->AddArray("material id");
            break;

          case (CELL):
            this->NumberOfCellFields += 1;
            this->NumberOfCellComponents += 1;
            this->CellDataArraySelection->AddArray("material id");
            break;

          case (FACE):
            vtkErrorMacro(<< "Face based data is not supported by this reader.");
            break;

          case (ENDKEYWORD):
            // End of variable data block
            break;
        }
        GMVRead::cleanup(&GMVRead::gmv_data.chardata1);
        GMVRead::cleanup(&GMVRead::gmv_data.longdata1);
        break;

      case (VELOCITY):
        switch (GMVRead::gmv_data.datatype)
        {
          case (NODE):
            this->NumberOfNodeFields += 3;
            this->NumberOfNodeComponents += 1;
            this->PointDataArraySelection->AddArray("velocity");
            break;

          case (CELL):
            this->NumberOfCellFields += 3;
            this->NumberOfCellComponents += 1;
            this->CellDataArraySelection->AddArray("velocity");
            break;

          case (FACE):
            break;
        }
        GMVRead::cleanup(&GMVRead::gmv_data.doubledata1);
        GMVRead::cleanup(&GMVRead::gmv_data.doubledata2);
        GMVRead::cleanup(&GMVRead::gmv_data.doubledata3);
        break;

      case (VECTORS):
        switch (GMVRead::gmv_data.datatype)
        {
          case (NODE):
            this->NumberOfNodeFields += 1;
            this->NumberOfNodeComponents += 1;
            this->PointDataArraySelection->AddArray(GMVRead::gmv_data.name1);
            break;

          case (CELL):
            this->NumberOfCellFields += 1;
            this->NumberOfCellComponents += 1;
            this->CellDataArraySelection->AddArray(GMVRead::gmv_data.name1);
            break;

          case (FACE):
            break;

          case (ENDKEYWORD):
            // End of vectors data block
            break;
        }
        GMVRead::cleanupAllData();
        break;

      case (VARIABLE):
        switch (GMVRead::gmv_data.datatype)
        {
          case (NODE):
            this->NumberOfNodeFields += 1;
            this->NumberOfNodeComponents += 1;
            this->PointDataArraySelection->AddArray(GMVRead::gmv_data.name1);
            break;

          case (CELL):
            this->NumberOfCellFields += 1;
            this->NumberOfCellComponents += 1;
            this->CellDataArraySelection->AddArray(GMVRead::gmv_data.name1);
            break;

          case (FACE):
            break;

          case (ENDKEYWORD):
            // End of variable data block
            break;
        }
        GMVRead::cleanupAllData();
        break;

      case (FLAGS):
      {
        int flagNameLen = (int)strlen(GMVRead::gmv_data.name1) + 5;
        char* flagName = new char[flagNameLen + 1];
        strncpy(&flagName[0], (char*)"flag ", 6);
        strcpy(&flagName[5], GMVRead::gmv_data.name1);
        flagName[flagNameLen] = '\0';

        switch (GMVRead::gmv_data.datatype)
        {
          case (NODE):
            this->NumberOfNodeFields += 1;
            this->NumberOfNodeComponents += 1;
            this->PointDataArraySelection->AddArray(flagName);
            break;

          case (CELL):
            this->NumberOfCellFields += 1;
            this->NumberOfCellComponents += 1;
            this->CellDataArraySelection->AddArray(flagName);
            break;
        }
        delete[] flagName;
      }
        GMVRead::cleanupAllData();
        break;

      case (POLYGONS):
        // GMV file format does not provide number of polygons beforehand
        // (unlike for nodes, cells, tracers etc.)
        // So, count them (per file) and store them for later in RequestData
        // (namely for allocating a polygonal dataset of correct size)
        switch (GMVRead::gmv_data.datatype)
        {
          case (REGULAR):
            this->NumberOfPolygons += 1;
            break;
          case (ENDKEYWORD):
            // Cache number of polygons in current file
            this->NumberOfPolygonsMap[this->FileName] = this->NumberOfPolygons;
            break;
        }
        GMVRead::cleanupAllData();
        break;

      case (TRACERS):
        switch (GMVRead::gmv_data.datatype)
        {
          case (XYZ):
            // Number of tracers
            this->NumberOfTracers = GMVRead::gmv_data.num;
            // Cache number of tracers in current file
            this->NumberOfTracersMap[this->FileName] = this->NumberOfTracers;
            break;

          case (TRACERDATA):
            // Determine min/max values for statistics tab
            this->NumberOfNodeFields += 1;
            this->NumberOfNodeComponents += 1;
            {
              int tracerNameLen = (int)strlen(GMVRead::gmv_data.name1) + 7;
              char* tracerName = new char[tracerNameLen + 1];
              strncpy(&tracerName[0], (char*)"tracer ", 8);
              strcpy(&tracerName[7], GMVRead::gmv_data.name1);
              tracerName[tracerNameLen] = '\0';
              this->PointDataArraySelection->AddArray(tracerName);
              delete[] tracerName;
            }
            GMVRead::cleanup(&GMVRead::gmv_data.doubledata1);
            break;

          case (ENDKEYWORD):
            GMVRead::cleanupAllData();
            break;
        }
        break;

      case (TRACEIDS):
        this->NumberOfNodeFields += 1;
        this->NumberOfNodeComponents += 1;
        this->PointDataArraySelection->AddArray("tracer id");

        GMVRead::cleanupAllData();
        break;

      case (PROBTIME):
        // PROBTIME keyword means that we need a time aware reader.
        // => parse all files of the series, automagically realised by class FileSeriesReader
        //    by calling RequestInformation() for all FileNames

        // If the user decides to set PROBTIME = 0 for all files in a series
        // instead of simply omitting the keyword, that is his problem as in
        // this case reading is slowed down unnecessarily.
        // Expensive because calling either one of
        //   outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), ...)
        //   outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), ...)
        // triggers reading all files of a series.

        timeStepValue = GMVRead::gmv_data.doubledata1[0];
        this->TimeStepValuesMap[this->FileName] = GMVRead::gmv_data.doubledata1[0];
        this->ContainsProbtimeKeyword = true;

        GMVRead::cleanupAllData();
        break;

      case (CYCLENO):
        this->NumberOfFields += 1;
        this->NumberOfFieldComponents += 1;
        this->FieldDataArraySelection->AddArray("cycle number");

        GMVRead::cleanupAllData();
        break;

      case (NODEIDS):
        switch (GMVRead::gmv_data.datatype)
        {
          case (REGULAR):
            this->NumberOfNodeFields += 1;
            this->NumberOfNodeComponents += 1;
            this->PointDataArraySelection->AddArray("Point IDs (Alternate)");
            break;
        }

        GMVRead::cleanupAllData();
        break;

      case (CELLIDS):
        switch (GMVRead::gmv_data.datatype)
        {
          case (REGULAR):
            this->NumberOfCellFields += 1;
            this->NumberOfCellComponents += 1;
            this->CellDataArraySelection->AddArray("Cell IDs (Alternate)");
            break;
        }

        GMVRead::cleanupAllData();
        break;

      case (SURFACE):
        GMVRead::cleanupAllData();
        break;

      case (SURFMATS):
        GMVRead::cleanupAllData();
        break;

      case (SURFVEL):
        GMVRead::cleanupAllData();
        break;

      case (SURFVARS):
        GMVRead::cleanupAllData();
        break;

      case (SURFFLAG):
        GMVRead::cleanupAllData();
        break;

      case (UNITS):
        GMVRead::cleanupAllData();
        break;

      case (VINFO):
        GMVRead::cleanupAllData();
        break;

      case (GROUPS):
        GMVRead::cleanupAllData();
        break;

      case (FACEIDS):
        GMVRead::cleanupAllData();
        break;

      case (SURFIDS):
        GMVRead::cleanupAllData();
        break;

      case (SUBVARS):
        GMVRead::cleanupAllData();
        break;

      case (CODENAME):
        this->NumberOfFields += 1;
        this->NumberOfFieldComponents += 1;
        this->FieldDataArraySelection->AddArray("code name");
        GMVRead::cleanupAllData();
        break;

      case (CODEVER):
        this->NumberOfFields += 1;
        this->NumberOfFieldComponents += 1;
        this->FieldDataArraySelection->AddArray("code version");
        GMVRead::cleanupAllData();
        break;

      case (SIMDATE):
        this->NumberOfFields += 1;
        this->NumberOfFieldComponents += 1;
        this->FieldDataArraySelection->AddArray("simulation date");

        GMVRead::cleanupAllData();
        break;

      default:
        GMVRead::cleanupAllData();
        break;
    }
  }

  if (this->ContainsProbtimeKeyword)
  {
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &timeStepValue, 1);
    double timeRange[2];
    timeRange[0] = timeStepValue;
    timeRange[1] = timeStepValue;
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkGMVReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "File Name: " << (this->FileName ? this->FileName : "(none)") << "\n";

  os << indent << "Number of Nodes: " << this->NumberOfNodes << endl;
  os << indent << "Number of Node Fields: " << this->NumberOfNodeFields << endl;
  os << indent << "Number of Node Components: " << this->NumberOfNodeComponents << endl;

  os << indent << "Number of Cells: " << this->NumberOfCells << endl;
  os << indent << "Number of Cell Fields: " << this->NumberOfCellFields << endl;
  os << indent << "Number of Cell Components: " << this->NumberOfCellComponents << endl;

  os << indent << "Number of Fields: " << this->NumberOfFields << endl;
  os << indent << "Number of Field Components: " << this->NumberOfFieldComponents << endl;

  os << indent << "Number of Tracers: " << this->NumberOfTracers << endl;

  os << indent << "Byte Order: " << this->ByteOrder << endl;
  os << indent << "Binary File: " << (this->BinaryFile ? "True\n" : "False\n");
}

//----------------------------------------------------------------------------
int vtkGMVReader::CanReadFile(const char* name)
{
  // return value 0: can not read
  // return value 1: can read

  int rc;

  // This is derived from the instructions at the beginning of
  // gmvread.c:gmvread_open()
  int chkend;
  char magic[9], rdend[21];

// Define taken from gmvread.c
#define CHARSIZE 1

  FILE* gmvin;

  // First make sure the file exists. This prevents an empty file
  // from being created on older compilers.
  vtksys::SystemTools::Stat_t fs;
  if (vtksys::SystemTools::Stat(name, &fs) != 0)
  {
    return 0;
  }

  gmvin = fopen(name, "r");
  if (gmvin == NULL)
  {
    // Cannot open file
    fclose(gmvin);
    return 0;
  }

  // Read header.
  rc = (int)fread(magic, CHARSIZE, (long)8, gmvin);
  if (strncmp(magic, "gmvinput", 8) != 0)
  {
    // Not a GMV input file.
    fclose(gmvin);
    return 0;
  }
  else
  // Check that gmv input file has "endgmv".
  {
    // Read the last 20 characters of the file.
    fseek(gmvin, -20L, 2);
    rc = (int)fread(rdend, sizeof(char), 20, gmvin);

    // Check the 20 characters for endgmv.
    chkend = 0;
    for (int index = 0; index < 15; index++)
    {
      if (strncmp((rdend + index), "endgmv", 6) == 0)
      {
        chkend = 1;
        break;
      }
    }

    if (!chkend)
    {
      // No valid GMV file: keyword endgmv not found.
      fclose(gmvin);
      return 0;
    }
  }

  rc = 1;
  return rc;
}

// //----------------------------------------------------------------------------
// vtkDataObject* vtkGMVReader::GetCurrentOutput()
// {
//   return this->CurrentOutput;
// }

// //----------------------------------------------------------------------------
// vtkInformation* vtkGMVReader::GetCurrentOutputInformation()
// {
//   return this->CurrentOutputInformation;
// }

// //----------------------------------------------------------------------------
// void vtkGMVReader::SetupEmptyOutput()
// {
//   this->GetCurrentOutput()->Initialize();
// }

//----------------------------------------------------------------------------
void vtkGMVReader::SetByteOrderToBigEndian()
{
  this->ByteOrder = FILE_BIG_ENDIAN;
}

//----------------------------------------------------------------------------
void vtkGMVReader::SetByteOrderToLittleEndian()
{
  this->ByteOrder = FILE_LITTLE_ENDIAN;
}

//----------------------------------------------------------------------------
const char* vtkGMVReader::GetByteOrderAsString()
{
  if (this->ByteOrder == FILE_LITTLE_ENDIAN)
    return "LittleEndian";
  else
    return "BigEndian";
}

//----------------------------------------------------------------------------
void vtkGMVReader::DisableAllPointArrays()
{
  this->PointDataArraySelection->DisableAllArrays();
}

//----------------------------------------------------------------------------
void vtkGMVReader::EnableAllPointArrays()
{
  this->PointDataArraySelection->EnableAllArrays();
}

//----------------------------------------------------------------------------
int vtkGMVReader::GetNumberOfPointArrays()
{
  return this->PointDataArraySelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
const char* vtkGMVReader::GetPointArrayName(int index)
{
  if (index >= (int)this->NumberOfNodeComponents || index < 0)
    return NULL;
  else
    return this->PointDataArraySelection->GetArrayName(index);
}

//----------------------------------------------------------------------------
int vtkGMVReader::GetPointArrayStatus(const char* name)
{
  return this->PointDataArraySelection->ArrayIsEnabled(name);
}

//----------------------------------------------------------------------------
void vtkGMVReader::SetPointArrayStatus(const char* name, int status)
{
  if (status)
    this->PointDataArraySelection->EnableArray(name);
  else
    this->PointDataArraySelection->DisableArray(name);
}

//----------------------------------------------------------------------------
void vtkGMVReader::DisableAllCellArrays()
{
  this->CellDataArraySelection->DisableAllArrays();
}

//----------------------------------------------------------------------------
void vtkGMVReader::EnableAllCellArrays()
{
  this->CellDataArraySelection->EnableAllArrays();
}

//----------------------------------------------------------------------------
int vtkGMVReader::GetNumberOfCellArrays()
{
  return this->CellDataArraySelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
const char* vtkGMVReader::GetCellArrayName(int index)
{
  if (index >= (int)this->NumberOfCellComponents || index < 0)
    return NULL;
  else
    return this->CellDataArraySelection->GetArrayName(index);
}

//----------------------------------------------------------------------------
int vtkGMVReader::GetCellArrayStatus(const char* name)
{
  return this->CellDataArraySelection->ArrayIsEnabled(name);
}

//----------------------------------------------------------------------------
void vtkGMVReader::SetCellArrayStatus(const char* name, int status)
{
  if (status)
    this->CellDataArraySelection->EnableArray(name);
  else
    this->CellDataArraySelection->DisableArray(name);
}

//----------------------------------------------------------------------------
void vtkGMVReader::DisableAllFieldArrays()
{
  this->FieldDataArraySelection->DisableAllArrays();
}

//----------------------------------------------------------------------------
void vtkGMVReader::EnableAllFieldArrays()
{
  this->FieldDataArraySelection->EnableAllArrays();
}

//----------------------------------------------------------------------------
int vtkGMVReader::GetNumberOfFieldArrays()
{
  return this->FieldDataArraySelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
const char* vtkGMVReader::GetFieldArrayName(int index)
{
  if (index >= (int)this->NumberOfFieldComponents || index < 0)
    return NULL;
  else
    return this->FieldDataArraySelection->GetArrayName(index);
}

//----------------------------------------------------------------------------
int vtkGMVReader::GetFieldArrayStatus(const char* name)
{
  return this->FieldDataArraySelection->ArrayIsEnabled(name);
}

//----------------------------------------------------------------------------
void vtkGMVReader::SetFieldArrayStatus(const char* name, int status)
{
  if (status)
    this->FieldDataArraySelection->EnableArray(name);
  else
    this->FieldDataArraySelection->DisableArray(name);
}

//----------------------------------------------------------------------------
void vtkGMVReader::SelectionModifiedCallback(vtkObject*, unsigned long, void* clientdata, void*)
{
  static_cast<vtkGMVReader*>(clientdata)->Modified();
}

//----------------------------------------------------------------------------
int vtkGMVReader::GetHasProbtimeKeyword()
{
  return (this->ContainsProbtimeKeyword ? 1 : 0);
}

//----------------------------------------------------------------------------
int vtkGMVReader::GetHasTracers()
{
  unsigned long sum;
  sum = 0;
  for (stringToULongMap::iterator itr = this->NumberOfTracersMap.begin();
       itr != this->NumberOfTracersMap.end(); ++itr)
    sum += itr->second;

  return (sum > 0);
}

//----------------------------------------------------------------------------
int vtkGMVReader::GetHasPolygons()
{
  unsigned long sum;
  sum = 0;
  for (stringToULongMap::iterator itr = this->NumberOfPolygonsMap.begin();
       itr != this->NumberOfPolygonsMap.end(); ++itr)
    sum += itr->second;

  return (sum > 0);
}
