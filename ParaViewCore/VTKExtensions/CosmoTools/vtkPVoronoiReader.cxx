#include <vtkPVoronoiReader.h>

#include <vtkObjectFactory.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkInformationVector.h>
#include <vtkInformation.h>
#include <vtkDataObject.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkMultiProcessController.h>
#include <vtkUnstructuredGrid.h>
#include <vtkCellArray.h>
#include <vtkFloatArray.h>
#include <vtkPoints.h>
#include <vtkFieldData.h>
#include <vtkCellData.h>

#include <iostream>
#include <string>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <assert.h>
#include <inttypes.h>
using namespace std;

#define VTK_CREATE(type, name) \
    vtkSmartPointer<type> name = vtkSmartPointer<type>::New()
#define VTK_NEW(type, name) \
    name = vtkSmartPointer<type>::New()

vtkStandardNewMacro(vtkPVoronoiReader);

vtkPVoronoiReader::vtkPVoronoiReader()
{
  this->swap_bytes = 0;
  this->HeaderSize = 256;
  this->FileName = NULL;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->Controller = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

vtkPVoronoiReader::~vtkPVoronoiReader()
{
  this->SetController(NULL);
}

void vtkPVoronoiReader::SetController(vtkMultiProcessController *c)
{
  if ((c == NULL) || (c->GetNumberOfProcesses() == 0))
  {
    this->NumProcesses = 1;
    this->MyId = 0;
  }

  if (this->Controller == c)
  {
    return;
  }

  this->Modified();

  if (this->Controller != NULL)
  {
    this->Controller->UnRegister(this);
    this->Controller = NULL;
  }

  if (c == NULL)
  {
    return;
  }

  this->Controller = c;

  c->Register(this);
  this->NumProcesses = c->GetNumberOfProcesses();
  this->MyId = c->GetLocalProcessId();
}

int vtkPVoronoiReader::FillOutputPortInformation( int port, vtkInformation* info )
{
  if ( port == 0 )
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet" );

    return 1;
  }

  return 0;
}

int vtkPVoronoiReader::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
//  int i, j;
  int i = 0;
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  int piece, numPieces;

  piece = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numPieces = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  vtkMultiBlockDataSet *output = vtkMultiBlockDataSet::SafeDownCast(
      outInfo->Get(vtkDataObject::DATA_OBJECT()));

  //read file
  vtkMultiProcessController *contr = this->Controller;

  int sum = 0;
  int oops = ((piece != this->MyId) || (numPieces != this->NumProcesses));

  contr->Reduce(&oops, &sum, 1, vtkCommunicator::SUM_OP, 0);
  contr->Broadcast(&sum, 1, 0);

  if (sum > 0) //from example, not sure whether to happen
  {
    return 1; //to be changed to the right error code
  }

  if (!contr) //from example, not sure whether to happen
  {
    return 1;
  }

  FILE *fd = fopen(FileName, "r");
  assert(fd != NULL);
  int64_t *ftr;   // footer
  int tb;         // total number of blocks
  int root = 0;   // root rank
  if (piece == root)
    ReadFooter(fd, ftr, tb);
  contr->Barrier();

  contr->Broadcast(&tb, 1, root);
  if (piece != root)
    ftr = new int64_t[tb];
  contr->Broadcast((unsigned char *)ftr, sizeof(int64_t) / sizeof(unsigned char) * tb, root);

  vblock_t *block;
  char msg[100];

  output->SetNumberOfBlocks(tb);

  for (i=piece; i<tb; i+=numPieces)
  {
    sprintf(msg, "rank %d, block %d, offset %lld\n", piece, i, (long long int)ftr[i]);
    printf("%s", msg);
    block = (vblock_t *)malloc(sizeof(vblock_t));
    ReadBlock(fd, block, ftr[i]);
    VTK_CREATE(vtkUnstructuredGrid, ugrid);
    vor2ugrid(block, ugrid);
    output->SetBlock(i, ugrid);
  }

  fclose(fd);

  if (contr != this->Controller)
  {
    contr->Delete();
  }

  delete ftr;

  return 1;
}

void vtkPVoronoiReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "File Name: "
      << (this->FileName ? this->FileName : "(none)") << "\n";
}

//----------------------------------------------------------------------------
//SER_IO related

//
// reads the file footer
// footer in file is always ordered by global block id
// output footer is in the same order
//
// fd: open file
// ftr: footer data (output)
// tb: total number of blocks in the file (output)
//
// side effects: allocates ftr
//
void vtkPVoronoiReader::ReadFooter(FILE*& fd, int64_t*& ftr, int& tb)
{
  int ofst;
  int64_t temp;

  ofst = sizeof(int64_t);
  fseek(fd, -ofst, SEEK_END);
#ifndef NDEBUG
  int count =
#endif
    fread(&temp, sizeof(int64_t), 1, fd); // total number of blocks
  assert(count == 1); // total number of blocks

  if (swap_bytes)
    Swap((char *)&temp, 1, sizeof(int64_t));
  tb = temp;

  if (tb > 0) {

    ftr = new int64_t[tb];
    ofst = (tb + 1) * sizeof(int64_t);
    fseek(fd, -ofst, SEEK_END);
#ifndef NDEBUG
    count =
#endif
      fread(ftr, sizeof(int64_t), tb, fd);
    assert(count == tb);

    if (swap_bytes)
      Swap((char *)ftr, tb, sizeof(int64_t));
  }
}

//
// reads the header for one block from a file
//
// fd: open file
// hdr: allocated header data
// ofst: location in file of the header (bytes)
//
void vtkPVoronoiReader::ReadHeader(FILE *fd, int *hdr, int64_t ofst)
{
  fseek(fd, ofst, SEEK_SET);
#ifndef NDEBUG
  int count =
#endif
    fread(hdr, sizeof(int), this->HeaderSize, fd);
  assert(count == this->HeaderSize);

  if (swap_bytes)
    Swap((char *)hdr, this->HeaderSize, sizeof(int));
}

//
// Copies the header for one block from a buffer in memory
//
// in_buf: input buffer location
// hdr: allocated header data
//
// returns: number of bytes copies
//
int vtkPVoronoiReader::CopyHeader(unsigned char *in_buf, int *hdr)
{
  memcpy(hdr, in_buf, this->HeaderSize * sizeof(int));

  if (swap_bytes)
    Swap((char *)hdr, this->HeaderSize, sizeof(int));

  return(this->HeaderSize * sizeof(int));
}

//
// reads one block from a file
//
// fd: open file
// v: pointer to output block
// ofst: file file pointer to start of header for this block
//
// side-effects: allocates block
//
void vtkPVoronoiReader::ReadBlock(FILE *fd, vblock_t* &v, int64_t ofst)
{
  // get header info
  int hdr[this->HeaderSize];
  ReadHeader(fd, hdr, ofst);

  // create block
  v = new vblock_t;
  v->num_verts = hdr[NUM_VERTS];
  v->num_cells = hdr[NUM_CELLS];
  v->tot_num_cell_verts = hdr[TOT_NUM_CELL_VERTS];
  v->num_complete_cells = hdr[NUM_COMPLETE_CELLS];
  v->tot_num_cell_faces = hdr[TOT_NUM_CELL_FACES];
  v->tot_num_face_verts = hdr[TOT_NUM_FACE_VERTS];
  v->num_orig_particles = hdr[NUM_ORIG_PARTICLES];

  if (v->num_verts > 0)
    v->save_verts = new float[3 * v->num_verts];
  if (v->num_cells > 0) {
    v->num_cell_verts = new int[v->num_cells];
    v->sites = new float[3 * v->num_orig_particles];
  }
  if (v->tot_num_cell_verts > 0)
    v->cells = new int[v->tot_num_cell_verts];
  if (v->num_complete_cells > 0) {
    v->complete_cells = new int[v->num_complete_cells];
    v->areas = new float[v->num_complete_cells];
    v->vols = new float[v->num_complete_cells];
    v->num_cell_faces = new int[v->num_complete_cells];
  }
  if (v->tot_num_cell_faces > 0)
    v->num_face_verts = new int[v->tot_num_cell_faces];
  if (v->tot_num_face_verts > 0)
    v->face_verts = new int[v->tot_num_face_verts];

  fread(v->mins, sizeof(float), 3, fd);
  fread(v->save_verts, sizeof(float), 3 * v->num_verts, fd);
  fread(v->sites, sizeof(float), 3 * v->num_orig_particles, fd);
  fread(v->complete_cells, sizeof(int), v->num_complete_cells, fd);
  fread(v->areas, sizeof(float), v->num_complete_cells, fd);
  fread(v->vols, sizeof(float), v->num_complete_cells, fd);
  fread(v->num_cell_faces, sizeof(int), v->num_complete_cells, fd);
  fread(v->num_face_verts, sizeof(int), v->tot_num_cell_faces, fd);
  fread(v->face_verts, sizeof(int), v->tot_num_face_verts, fd);
  fread(v->maxs, sizeof(float), 3, fd);

  if (swap_bytes) {
    Swap((char *)v->mins, 3, sizeof(float));
    Swap((char *)v->save_verts, 3 * v->num_verts, sizeof(float));
    Swap((char *)v->sites, 3 * v->num_orig_particles, sizeof(float));
    Swap((char *)v->complete_cells, v->num_complete_cells, sizeof(int));
    Swap((char *)v->areas, v->num_complete_cells, sizeof(float));
    Swap((char *)v->vols, v->num_complete_cells, sizeof(float));
    Swap((char *)v->num_cell_faces, v->num_complete_cells, sizeof(int));
    Swap((char *)v->num_face_verts, v->tot_num_cell_faces, sizeof(int));
    Swap((char *)v->face_verts, v->tot_num_face_verts, sizeof(int));
    Swap((char *)v->maxs, 3, sizeof(float));
  }
}

//
// copies one block from a buffer in memory
//
// in_buf: input buffer location
// m: pointer to output block
// ofst: file file pointer to start of header for this block
//
// side-effects: allocates block
//
void vtkPVoronoiReader::CopyBlock(unsigned char *in_buf, vblock_t* &v)
{
  int ofst = 0; // offset in buffer for next section of data to read

  // get header info
  int hdr[this->HeaderSize];
  ofst += CopyHeader(in_buf, hdr);

  // create block
  v = new vblock_t;
  v->num_verts = hdr[NUM_VERTS];
  v->num_cells = hdr[NUM_CELLS];
  v->tot_num_cell_verts = hdr[TOT_NUM_CELL_VERTS];
  v->num_complete_cells = hdr[NUM_COMPLETE_CELLS];
  v->tot_num_cell_faces = hdr[TOT_NUM_CELL_FACES];
  v->tot_num_face_verts = hdr[TOT_NUM_FACE_VERTS];
  v->num_orig_particles = hdr[NUM_ORIG_PARTICLES];

  if (v->num_verts > 0)
    v->save_verts = new float[3 * v->num_verts];
  if (v->num_cells > 0) {
    v->num_cell_verts = new int[v->num_cells];
    v->sites = new float[3 * v->num_orig_particles];
  }
  if (v->tot_num_cell_verts > 0)
    v->cells = new int[v->tot_num_cell_verts];
  if (v->num_complete_cells > 0) {
    v->complete_cells = new int[v->num_complete_cells];
    v->areas = new float[v->num_complete_cells];
    v->vols = new float[v->num_complete_cells];
    v->num_cell_faces = new int[v->num_complete_cells];
  }
  if (v->tot_num_cell_faces > 0)
    v->num_face_verts = new int[v->tot_num_cell_faces];
  if (v->tot_num_face_verts > 0)
    v->face_verts = new int[v->tot_num_face_verts];

    memcpy(&v->mins, in_buf + ofst, 3 * sizeof(float));
    ofst += (3 * sizeof(float));

    memcpy(v->save_verts, in_buf + ofst, 3 * v->num_verts * sizeof(float));
    ofst += (3 * v->num_verts * sizeof(float));

    memcpy(v->sites, in_buf + ofst, 3 * v->num_orig_particles * sizeof(float));
    ofst += (3 * v->num_orig_particles * sizeof(float));

    memcpy(v->complete_cells, in_buf + ofst,
     v->num_complete_cells * sizeof(int));
    ofst += (v->num_complete_cells * sizeof(int));

    memcpy(v->areas, in_buf + ofst, v->num_complete_cells * sizeof(float));
    ofst += (v->num_complete_cells * sizeof(float));

    memcpy(v->vols, in_buf + ofst, v->num_complete_cells * sizeof(float));
    ofst += (v->num_complete_cells * sizeof(float));

    memcpy(v->num_cell_faces, in_buf + ofst,
     v->num_complete_cells * sizeof(int));
    ofst += (v->num_complete_cells * sizeof(int));

    memcpy(v->num_face_verts, in_buf + ofst,
     v->tot_num_cell_faces * sizeof(int));
    ofst += (v->tot_num_cell_faces * sizeof(int));

    memcpy(v->face_verts, in_buf + ofst, v->tot_num_face_verts * sizeof(int));
    ofst += (v->tot_num_face_verts * sizeof(int));

    memcpy(&v->maxs, in_buf + ofst, 3 * sizeof(float));
    ofst += (3 * sizeof(float));

  if (swap_bytes) {
    Swap((char *)&v->mins, 3, sizeof(float));
    Swap((char *)v->save_verts, 3 * v->num_verts, sizeof(float));
    Swap((char *)v->sites, 3 * v->num_orig_particles, sizeof(float));
    Swap((char *)v->complete_cells, v->num_complete_cells, sizeof(int));
    Swap((char *)v->areas, v->num_complete_cells, sizeof(float));
    Swap((char *)v->vols, v->num_complete_cells, sizeof(float));
    Swap((char *)v->num_cell_faces, v->num_complete_cells, sizeof(int));
    Swap((char *)v->num_face_verts, v->tot_num_cell_faces, sizeof(int));
    Swap((char *)v->face_verts, v->tot_num_face_verts, sizeof(int));
    Swap((char *)&v->maxs, 3, sizeof(float));
  }
}

//----------------------------------------------------------------------------
// SWAP related

//
// n: address of items
// nitems: number of items
// item_size: either 2, 4, or 8 bytes
// returns quietly if item_size is 1
//
void vtkPVoronoiReader::Swap(char *n, int nitems, int item_size)
{
  int i;

  switch(item_size) {
  case 1:
    break;
  case 2:
    for (i = 0; i < nitems; i++) {
      Swap2(n);
      n += 2;
    }
    break;
  case 4:
    for (i = 0; i < nitems; i++) {
      Swap4(n);
      n += 4;
    }
    break;
  case 8:
    for (i = 0; i < nitems; i++) {
      Swap8(n);
      n += 8;
    }
    break;
  default:
    fprintf(stderr, "Error: size of data must be either 1, 2, 4, or 8 bytes per item\n");
    //MPI_Abort(MPI_COMM_WORLD, 0);
  }
}

//
// Swaps 8  bytes from 1-2-3-4-5-6-7-8 to 8-7-6-5-4-3-2-1 order.
// cast the input as a char and use on any 8 byte variable
//
void vtkPVoronoiReader::Swap8(char *n)
{
  char *n1;
  char c;

  n1 = n + 7;
  c = *n;
  *n = *n1;
  *n1 = c;

  n++;
  n1--;
  c = *n;
  *n = *n1;
  *n1 = c;

  n++;
  n1--;
  c = *n;
  *n = *n1;
  *n1 = c;

  n++;
  n1--;
  c = *n;
  *n = *n1;
  *n1 = c;
}

//
// Swaps 4 bytes from 1-2-3-4 to 4-3-2-1 order.
// cast the input as a char and use on any 4 byte variable
//
void vtkPVoronoiReader::Swap4(char *n)
{
  char *n1;
  char c;

  n1 = n + 3;
  c = *n;
  *n = *n1;
  *n1 = c;

  n++;
  n1--;
  c = *n;
  *n = *n1;
  *n1 = c;
}

//
// Swaps 2 bytes from 1-2 to 2-1 order.
// cast the input as a char and use on any 2 byte variable
//
void vtkPVoronoiReader::Swap2(char *n)
{
  char c;

  c = *n;
  *n = n[1];
  n[1] = c;
}

void vtkPVoronoiReader::vor2ugrid(struct vblock_t *block, vtkSmartPointer<vtkUnstructuredGrid> &ugrid)
{
//  int i, j, k;
  int j = 0;
  int k = 0;
  int num_cells, num_verts;

  num_cells = block->num_complete_cells;
  num_verts = block->num_verts;

  float *vert_vals = block->save_verts;

  //create points
  VTK_CREATE(vtkFloatArray, points_array);
  points_array->SetNumberOfComponents(3);
  points_array->SetNumberOfTuples(num_verts);
  points_array->SetVoidArray(vert_vals, 3 * num_verts, 0);

  VTK_CREATE(vtkPoints, points);
  points->SetData(points_array);

  //create unstructure grid
  ugrid->SetPoints(points);

  //cells
  int vert_id, face_id, num_cell_faces, num_face_verts, num_tot_fverts;
  int vert_id_cell_start, num_cell_fverts;
  vtkIdType *face_verts;

  vert_id = 0;
  face_id = 0;
  num_tot_fverts = block->tot_num_face_verts;

  face_verts = (vtkIdType *)malloc(sizeof(vtkIdType) * num_tot_fverts);
  for (j=0; j<num_tot_fverts; j++)
    face_verts[j] = block->face_verts[j];

  for (j=0; j<num_cells; j++)
  {
    VTK_CREATE(vtkCellArray, cell);
    num_cell_faces = block->num_cell_faces[j];

    vert_id_cell_start = vert_id;
    num_cell_fverts = 0;

    for (k=0; k<num_cell_faces; k++)
    {
      num_face_verts = block->num_face_verts[face_id];
      cell->InsertNextCell(num_face_verts, &face_verts[vert_id]);

      face_id ++;
      vert_id += num_face_verts;
      num_cell_fverts += num_face_verts;
    }

    ugrid->InsertNextCell(VTK_POLYHEDRON, num_cell_fverts, &face_verts[vert_id_cell_start], num_cell_faces, cell->GetPointer());
  }

  VTK_CREATE(vtkFloatArray, area_array);
  area_array->SetName("Areas");
  area_array->SetNumberOfComponents(1);
  area_array->SetNumberOfTuples(num_cells);
  area_array->SetVoidArray(block->areas, num_cells, 1); //keep the array from being delete when the object is deleted
  ugrid->GetCellData()->AddArray(area_array);

  VTK_CREATE(vtkFloatArray, vol_array);
  vol_array->SetName("Volumes");
  vol_array->SetNumberOfComponents(1);
  vol_array->SetNumberOfTuples(num_cells);
  vol_array->SetVoidArray(block->vols, num_cells, 1);
  ugrid->GetCellData()->AddArray(vol_array);

}
