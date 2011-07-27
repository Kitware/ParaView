/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkSQPlaneSource.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSQPlaneSource.h"

#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkTransform.h"

#include "vtkSQMetaDataKeys.h"
#include "vtkSQPlaneSourceConstants.h"
#include "vtkSQPlaneSourceCellGenerator.h"
#include "SQMacros.h"
#include "postream.h"

#include <map>
using std::map;
using std::pair;

vtkCxxRevisionMacro(vtkSQPlaneSource,"$Revision: 1.0$");
vtkStandardNewMacro(vtkSQPlaneSource);

//-----------------------------------------------------------------------------
vtkSQPlaneSource::vtkSQPlaneSource()
{
  #ifdef vtkSQPlaneSourceDEBUG
  cerr << "===============================vtkSQPlaneSource" << endl;
  #endif

  this->ImmediateMode=1;

  this->XResolution=this->YResolution=1;

  this->Origin[0]=this->Origin[1]=this->Origin[2]
    =this->Point1[0]=this->Point1[1]=this->Point1[2]
    =this->Point2[0]=this->Point2[1]=this->Point2[2]
    =this->Normal[0]=this->Normal[1]=this->Normal[2]
    =this->Center[0]=this->Center[1]=this->Center[2]=0.0;

  this->Point1[0]=1.0;
  this->Point2[1]=1.0;
  this->Normal[2]=1.0;
  this->Center[0]=this->Center[1]=0.5;

  this->Constraint=SQPS_CONSTRAINT_NONE;

  this->DescriptiveName=0;

  this->SetNumberOfInputPorts(0);
}

//-----------------------------------------------------------------------------
vtkSQPlaneSource::~vtkSQPlaneSource()
{
  #ifdef vtkSQPlaneSourceDEBUG
  cerr << "===============================vtkSQPlaneSource::~vtkSQPlaneSource" << endl;
  #endif
  this->SetDescriptiveName(0);
}

//-----------------------------------------------------------------------------
void vtkSQPlaneSource::SetResolution(const int xR, const int yR)
{
  #ifdef vtkSQPlaneSourceDEBUG
  cerr << "===============================vtkSQPlaneSource::SetResolution" << endl;
  #endif
  // Set the number of x-y subdivisions in the plane.
  if ( xR != this->XResolution || yR != this->YResolution )
    {
    this->XResolution = xR;
    this->YResolution = yR;

    this->XResolution = (this->XResolution > 0 ? this->XResolution : 1);
    this->YResolution = (this->YResolution > 0 ? this->YResolution : 1);

    this->Modified();
    }
}

//----------------------------------------------------------------------------
int vtkSQPlaneSource::RequestInformation(
    vtkInformation * /*req*/,
    vtkInformationVector ** /*inInfos*/,
    vtkInformationVector *outInfos)
{
  #ifdef vtkSQPlaneSourceDEBUG
    cerr << "===============================vtkSQPlaneSource::RequestInformation" << endl;
  #endif


  // tell the excutive that we are handling our own decomposition.
  vtkInformation *outInfo=outInfos->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),-1);

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSQPlaneSource::RequestData(
      vtkInformation *req,
      vtkInformationVector ** /*inputVector*/,
      vtkInformationVector *outputVector)
{
  #ifdef vtkSQPlaneSourceDEBUG
  cerr << "===============================vtkSQPlaneSource::RequestData" << endl;
  #endif

  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the ouptut
  vtkPolyData *output
    =dynamic_cast<vtkPolyData*>(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // decompose by piece information.
  int pieceNo
    = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int nPieces
    = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  // sanity - the requst cannot be fullfilled
  if (pieceNo>=nPieces)
    {
    output->Initialize();
    return 1;
    }

  // The default domain decomposition of one cell per process
  // is used in demand mode. If immediate mode is on then these
  // will be reset.
  int nLocal=1;
  int startId=pieceNo;
  int endId=pieceNo+1;
  int resolution[2]={1,nPieces};

  if (!this->ImmediateMode)
    {
    // In demeand mode a pseduo dataset is generated here for
    // display in PV, a demand plane source is inserted into
    // the pipeline for down stream access to any of the plane's
    // cell's
    vtkSQPlaneSourceCellGenerator *gen=vtkSQPlaneSourceCellGenerator::New();
    gen->SetOrigin(this->Origin);
    gen->SetPoint1(this->Point1);
    gen->SetPoint2(this->Point2);
    gen->SetResolution(this->XResolution,this->YResolution);

    outInfo->Set(vtkSQCellGenerator::CELL_GENERATOR(),gen);
    gen->Delete();

    req->Append(
          vtkExecutive::KEYS_TO_COPY(),
          vtkSQCellGenerator::CELL_GENERATOR());
    }
  else
    {
    // Immediate mode domain decomposition
    int nCells=this->XResolution*this->YResolution;
    int pieceSize=nCells/nPieces;
    int nLarge=nCells%nPieces;
    nLocal=pieceSize+(pieceNo<nLarge?1:0);
    startId=pieceSize*pieceNo+(pieceNo<nLarge?pieceNo:nLarge);
    endId=startId+nLocal;

    resolution[0]=this->XResolution;
    resolution[1]=this->YResolution;
    }

  // Configure the output
  vtkFloatArray *X=vtkFloatArray::New();
  X->SetNumberOfComponents(3);
  X->SetNumberOfTuples(12*nLocal); // bounded by
  float *pX=X->GetPointer(0);

  vtkPoints *pts=vtkPoints::New();
  pts->SetData(X);
  X->Delete();

  output->SetPoints(pts);
  pts->Delete();

  vtkIdTypeArray *ia=vtkIdTypeArray::New();
  ia->SetNumberOfComponents(1);
  ia->SetNumberOfTuples(5*nLocal);
  vtkIdType *pIa=ia->GetPointer(0);

  vtkCellArray *polys=vtkCellArray::New();
  polys->SetCells(nLocal,ia);
  ia->Delete();

  output->SetPolys(polys);
  polys->Delete();

  // cell generator
  vtkSQPlaneSourceCellGenerator *source=vtkSQPlaneSourceCellGenerator::New();
  source->SetOrigin(this->Origin);
  source->SetPoint1(this->Point1);
  source->SetPoint2(this->Point2);
  source->SetResolution(resolution);

  int ptId=0;
  map<vtkIdType,vtkIdType> idMap;
  pair<map<vtkIdType,vtkIdType>::iterator,bool> idMapInsert; 

  for (int cid=startId; cid<endId; ++cid)
    {
    // get the grid indexes which are used as keys to
    // insure a single insertion of each coordinate.
    vtkIdType cpids[4];
    source->GetCellPointIndexes(cid,cpids);

    // get the poly's vertices coordinates
    float cpts[12];
    source->GetCellPoints(cid,cpts);

    pIa[0]=4; // set number of verts for this cell
    ++pIa;

    for (int i=0; i<4; ++i)
      {
      // Attempt an insert of the new point's index
      idMapInsert=idMap.insert(pair<vtkIdType,vtkIdType>(cpids[i],ptId));
      if (idMapInsert.second)
        {
        // this is a new point id and point. Add the point.
        int qq=3*i;
        pX[0]=cpts[qq];
        pX[1]=cpts[qq+1];
        pX[2]=cpts[qq+2];
        pX+=3;
        ++ptId;
        }

      // convert the index to a local pt id, and add to the cell. 
      pIa[0]=idMapInsert.first->second;
      ++pIa;
      }
    }

  // update point array
  X->SetNumberOfTuples(ptId);

  // Set the descriptiove name (if used).
  if (this->DescriptiveName && strlen(this->DescriptiveName))
    {
    outInfo->Set(vtkSQMetaDataKeys::DESCRIPTIVE_NAME(),this->DescriptiveName);
    }

  source->Delete();

  //output->Print(cerr);



  // double x[3], tc[2], v1[3], v2[3];
  // vtkIdType pts[4];
  // int i, j, ii;
  // int numPts;
  // int numPolys;
  // vtkPoints *newPoints; 
  // vtkFloatArray *newNormals;
  // vtkFloatArray *newTCoords;
  // vtkCellArray *newPolys;
  // 
  // // Check input
  // for ( i=0; i < 3; i++ )
  //   {
  //   v1[i] = this->Point1[i] - this->Origin[i];
  //   v2[i] = this->Point2[i] - this->Origin[i];
  //   }
  // if ( !this->UpdatePlane(v1,v2) )
  //   {
  //   return 0;
  //   }
  // 
  // // Set things up; allocate memory
  // //
  // numPts = (this->XResolution+1) * (this->YResolution+1);
  // numPolys = this->XResolution * this->YResolution;
  // 
  // newPoints = vtkPoints::New();
  // newPoints->Allocate(numPts);
  // newNormals = vtkFloatArray::New();
  // newNormals->SetNumberOfComponents(3);
  // newNormals->Allocate(3*numPts);
  // newTCoords = vtkFloatArray::New();
  // newTCoords->SetNumberOfComponents(2);
  // newTCoords->Allocate(2*numPts);
  // 
  // newPolys = vtkCellArray::New();
  // newPolys->Allocate(newPolys->EstimateSize(numPolys,4));
  // 
  // // Generate points and point data
  // //
  // for (numPts=0, i=0; i<(this->YResolution+1); i++)
  //   {
  //   tc[1] = static_cast<double>(i)/ this->YResolution;
  //   for (j=0; j<(this->XResolution+1); j++)
  //     {
  //     tc[0] = static_cast<double>(j) / this->XResolution;
  // 
  //     for ( ii=0; ii < 3; ii++)
  //       {
  //       x[ii] = this->Origin[ii] + tc[0]*v1[ii] + tc[1]*v2[ii];
  //       }
  // 
  //     newPoints->InsertPoint(numPts,x);
  //     newTCoords->InsertTuple(numPts,tc);
  //     newNormals->InsertTuple(numPts++,this->Normal);
  //     }
  //   }
  // 
  // // Generate polygon connectivity
  // //
  // for (i=0; i<this->YResolution; i++)
  //   {
  //   for (j=0; j<this->XResolution; j++)
  //     {
  //     pts[0] = j + i*(this->XResolution+1);
  //     pts[1] = pts[0] + 1;
  //     pts[2] = pts[0] + this->XResolution + 2;
  //     pts[3] = pts[0] + this->XResolution + 1;
  //     newPolys->InsertNextCell(4,pts);
  //     }
  //   }
  // 
  // // Update ourselves and release memory
  // output->SetPoints(newPoints);
  // newPoints->Delete();
  // 
  // newNormals->SetName("Normals");
  // output->GetPointData()->SetNormals(newNormals);
  // newNormals->Delete();
  // 
  // newTCoords->SetName("TextureCoordinates");
  // output->GetPointData()->SetTCoords(newTCoords);
  // newTCoords->Delete();
  // 
  // output->SetPolys(newPolys);
  // newPolys->Delete();



  return 1;
}

//-----------------------------------------------------------------------------
void vtkSQPlaneSource::SetNormal(double N[3])
{
  #ifdef vtkSQPlaneSourceDEBUG
  cerr << "===============================SetNormal" << endl;
  #endif

  // Set the normal to the plane. Will modify the Origin, Point1, and Point2
  // instance variables as necessary (i.e., rotate the plane around its center).
  double n[3], rotVector[3], theta;

  //make sure input is decent
  n[0] = N[0];
  n[1] = N[1];
  n[2] = N[2];
  if ( vtkMath::Normalize(n) == 0.0 )
    {
    vtkErrorMacro(<<"Specified zero normal");
    return;
    }

  // Compute rotation vector using a transformation matrix.
  // Note that if normals are parallel then the rotation is either
  // 0 or 180 degrees.
  double dp = vtkMath::Dot(this->Normal,n);
  if ( dp >= 1.0 )
    {
    return; //zero rotation
    }
  else if ( dp <= -1.0 )
    {
    theta = 180.0;
    rotVector[0] = this->Point1[0] - this->Origin[0];
    rotVector[1] = this->Point1[1] - this->Origin[1];
    rotVector[2] = this->Point1[2] - this->Origin[2];
    }
  else
    {
    vtkMath::Cross(this->Normal,n,rotVector);
    theta = vtkMath::DegreesFromRadians( acos( dp ) );
    }

  // create rotation matrix
  vtkTransform *transform = vtkTransform::New();
  transform->PostMultiply();

  transform->Translate(-this->Center[0],-this->Center[1],-this->Center[2]);
  transform->RotateWXYZ(theta,rotVector[0],rotVector[1],rotVector[2]);
  transform->Translate(this->Center[0],this->Center[1],this->Center[2]);

  // transform the three defining points
  transform->TransformPoint(this->Origin,this->Origin);
  transform->TransformPoint(this->Point1,this->Point1);
  transform->TransformPoint(this->Point2,this->Point2);

  this->Normal[0] = n[0]; this->Normal[1] = n[1]; this->Normal[2] = n[2];

  this->Modified();
  transform->Delete();
}


//-----------------------------------------------------------------------------
void vtkSQPlaneSource::SetNormal(double nx, double ny, double nz)
{
  // Set the normal to the plane. Will modify the Origin, Point1, and Point2
  // instance variables as necessary (i.e., rotate the plane around its center).
  double n[3];

  n[0] = nx; n[1] = ny; n[2] = nz;
  this->SetNormal(n);
}

//-----------------------------------------------------------------------------
void vtkSQPlaneSource::SetCenter(double center[3])
{
  #ifdef vtkSQPlaneSourceDEBUG
  cerr << "===============================SetCenter" << endl;
  #endif

  // Set the center of the plane. Will modify the Origin, Point1, and Point2
  // instance variables as necessary (i.e., translate the plane).
  if ( this->Center[0] == center[0] && this->Center[1] == center[1] &&
  this->Center[2] == center[2] )
    {
    return; //no change
    }
  else
    {
    int i;
    double v1[3], v2[3];

    for ( i=0; i < 3; i++ )
      {
      v1[i] = this->Point1[i] - this->Origin[i];
      v2[i] = this->Point2[i] - this->Origin[i];
      }

    for ( i=0; i < 3; i++ )
      {
      this->Center[i] = center[i];
      this->Origin[i] = this->Center[i] - 0.5*(v1[i] + v2[i]);
      this->Point1[i] = this->Origin[i] + v1[i];
      this->Point2[i] = this->Origin[i] + v2[i];
      }
    this->Modified();
    }
}

//-----------------------------------------------------------------------------
void vtkSQPlaneSource::SetCenter(double x, double y, double z)
{
  // Set the center of the plane. Will modify the Origin, Point1, and Point2
  // instance variables as necessary (i.e., translate the plane).
  double center[3];
  center[0] = x; center[1] = y; center[2] = z;
  this->SetCenter(center);
}

//-----------------------------------------------------------------------------
void vtkSQPlaneSource::ApplyConstraint()
{
  #ifdef vtkSQPlaneSourceDEBUG
  cerr << "===============================ApplyConstraint" << endl;
  #endif

  double p[3]={0.0};
  double o[3]={0.0};

  switch (this->Constraint)
  {
  case SQPS_CONSTRAINT_NONE:
    break;

  case SQPS_CONSTRAINT_XY:
    this->GetOrigin(o);

    this->GetPoint1(p);
    p[2]=o[2];
    this->SetPoint1(p);

    this->GetPoint2(p);
    p[2]=o[2];
    this->SetPoint2(p);

    break;

  case SQPS_CONSTRAINT_XZ:
    this->GetOrigin(o);

    this->GetPoint1(p);
    p[1]=o[1];
    this->SetPoint1(p);

    this->GetPoint2(p);
    p[1]=o[1];
    this->SetPoint2(p);

    break;

  case SQPS_CONSTRAINT_YZ:
    this->GetOrigin(o);

    this->GetPoint1(p);
    p[0]=o[0];
    this->SetPoint1(p);

    this->GetPoint2(p);
    p[0]=o[0];
    this->SetPoint2(p);

    break;

  default:
    sqErrorMacro(pCerr(),"Invalid constraint.");
  }
}

//-----------------------------------------------------------------------------
void vtkSQPlaneSource::SetConstraint(int type)
{
  #ifdef vtkSQPlaneSourceDEBUG
  cerr << "===============================SetConstraint" << endl;
  cerr << type << endl;
  #endif

  if (this->Constraint == type )
    {
    return; // no change
    }

  this->Constraint = type;
  this->ApplyConstraint();
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkSQPlaneSource::SetOrigin(double ox, double oy, double oz)
{
  #ifdef vtkSQPlaneSourceDEBUG
  cerr << "===============================SetOrigin" << endl;
  cerr << ox << " " << oy << " " << oz << endl;
  #endif


  // modifies the normal and origin
  if ( this->Origin[0] == ox
    && this->Origin[1] == oy
    && this->Origin[2] == oz )
    {
    return; //no change
    }
  else
    {
    double v1[3], v2[3];

    this->Origin[0] = ox;
    this->Origin[1] = oy;
    this->Origin[2] = oz;

    v1[0] = this->Point1[0]-this->Origin[0];
    v1[1] = this->Point1[1]-this->Origin[1];
    v1[2] = this->Point1[2]-this->Origin[2];

    v2[0] = this->Point2[0]-this->Origin[0];
    v2[1] = this->Point2[1]-this->Origin[1];
    v2[2] = this->Point2[2]-this->Origin[2];

    // set plane normal
    this->UpdatePlane(v1,v2);
    this->Modified();

    this->ApplyConstraint();
    }
}

//-----------------------------------------------------------------------------
void vtkSQPlaneSource::SetOrigin(double p[3])
{
  this->SetOrigin(p[0],p[1],p[2]);
}

//-----------------------------------------------------------------------------
void vtkSQPlaneSource::SetPoint1(double p[3])
{
  this->SetPoint1(p[0],p[1],p[2]);
}

//-----------------------------------------------------------------------------
void vtkSQPlaneSource::SetPoint1(double x, double y, double z)
{
#ifdef vtkSQPlaneSourceDEBUG
  cerr << "===============================SetPoint1" << endl;
  cerr << x << " " << y << " " << z << endl;
  #endif

  // modifies the normal and origin
  if ( this->Point1[0] == x
    && this->Point1[1] == y
    && this->Point1[2] == z )
    {
    return; //no change
    }
  else
    {
    double v1[3], v2[3];

    this->Point1[0] = x;
    this->Point1[1] = y;
    this->Point1[2] = z;

    v1[0] = this->Point1[0]-this->Origin[0];
    v1[1] = this->Point1[1]-this->Origin[1];
    v1[2] = this->Point1[2]-this->Origin[2];

    v2[0] = this->Point2[0]-this->Origin[0];
    v2[1] = this->Point2[1]-this->Origin[1];
    v2[2] = this->Point2[2]-this->Origin[2];

    // set plane normal
    this->UpdatePlane(v1,v2);
    this->Modified();
    }
}

//-----------------------------------------------------------------------------
void vtkSQPlaneSource::SetPoint2(double p[3])
{
  this->SetPoint2(p[0],p[1],p[2]);
}

//-----------------------------------------------------------------------------
void vtkSQPlaneSource::SetPoint2(double x, double y, double z)
{
  #ifdef vtkSQPlaneSourceDEBUG
  cerr << "===============================SetPoint2" << endl;
  cerr << x << " " << y << " " << z << endl;
  #endif

  // modifies the normal and origin
  if ( this->Point2[0] == x
    && this->Point2[1] == y
    && this->Point2[2] == z )
    {
    return; //no change
    }
  else
    {
    double v1[3], v2[3];

    this->Point2[0] = x;
    this->Point2[1] = y;
    this->Point2[2] = z;

    v1[0] = this->Point1[0]-this->Origin[0];
    v1[1] = this->Point1[1]-this->Origin[1];
    v1[2] = this->Point1[2]-this->Origin[2];

    v2[0] = this->Point2[0]-this->Origin[0];
    v2[1] = this->Point2[1]-this->Origin[1];
    v2[2] = this->Point2[2]-this->Origin[2];

    // set plane normal
    this->UpdatePlane(v1,v2);
    this->Modified();
    }
}

//-----------------------------------------------------------------------------
void vtkSQPlaneSource::Push(double distance)
{
  // Translate the plane in the direction of the normal by the distance specified.
  // Negative values move the plane in the opposite direction.
  int i;

  if ( distance == 0.0 )
    {
    return;
    }
  for (i=0; i < 3; i++ )
    {
    this->Origin[i] += distance * this->Normal[i];
    this->Point1[i] += distance * this->Normal[i];
    this->Point2[i] += distance * this->Normal[i];
    }
  // set the new center
  for ( i=0; i < 3; i++ )
    {
    this->Center[i] = 0.5*(this->Point1[i] + this->Point2[i]);
    }

  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkSQPlaneSource::UpdatePlane(double v1[3], double v2[3])
{
  // Protected method updates normals and plane center from two axes.
  // set plane center
  for ( int i=0; i < 3; i++ )
    {
    this->Center[i] = this->Origin[i] + 0.5*(v1[i] + v2[i]);
    }

  // set plane normal
  vtkMath::Cross(v1,v2,this->Normal);
  if ( vtkMath::Normalize(this->Normal) == 0.0 )
    {
    vtkErrorMacro(<<"Bad plane coordinate system");
    return 0;
    }
  else
    {
    return 1;
    }
}

//-----------------------------------------------------------------------------
void vtkSQPlaneSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "X Resolution: " << this->XResolution << "\n";
  os << indent << "Y Resolution: " << this->YResolution << "\n";

  os << indent << "Origin: (" << this->Origin[0] << ", "
                              << this->Origin[1] << ", "
                              << this->Origin[2] << ")\n";

  os << indent << "Point 1: (" << this->Point1[0] << ", "
                               << this->Point1[1] << ", "
                               << this->Point1[2] << ")\n";

  os << indent << "Point 2: (" << this->Point2[0] << ", "
                               << this->Point2[1] << ", "
                               << this->Point2[2] << ")\n";

  os << indent << "Normal: (" << this->Normal[0] << ", "
                              << this->Normal[1] << ", "
                              << this->Normal[2] << ")\n";

  os << indent << "Center: (" << this->Center[0] << ", "
                              << this->Center[1] << ", "
                              << this->Center[2] << ")\n";

}
