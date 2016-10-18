#ifndef vtkCTHSource_h
#define vtkCTHSource_h

#include "vtkBoundingBox.h"
#include "vtkSmartPointer.h"

#include <vector>

class vtkCTHDataArray;
class vtkIntArray;
class vtkNonOverlappingAMR;
class vtkAMRBox;
class vtkUniformGrid;
class vtkCPInputDataDescription;

class vtkCTHSource
{
public:
  vtkCTHSource();
  ~vtkCTHSource();

  // Description:
  // Initializes the model parameters
  void Initialize(int igm, int n_blocks, int nmat, int max_mat, int NCFields, int NMFields,
    double* x0, double* x1, int max_level);
  // Description:
  // Initialize block parameters for block_id
  void InitializeBlock(int block_id, int Nx, int Ny, int Nz, double* x, double* y, double* z,
    int active, int active1, int level);
  // Description:
  // Initialize field parmaeters for field_id
  void SetCellFieldName(int field_id, char* field_name, char* comment, int matid);
  void SetMaterialFieldName(int field_id, char* field_name, char* command);
  // Description:
  // Set data pointer for horizontal field data strip
  void SetCellFieldPointer(int block_id, int field_id, int k, int j, double* istrip);
  void SetMaterialFieldPointer(int block_id, int field_id, int mat, int k, int j, double* istrip);
  // Description:
  // Update block parameters for block_id (due to refinement/coarsening)
  void UpdateBlock(int block_id, int active, int active1, int level, int max_level, int bxbot,
    int bxtop, int bybot, int bytop, int bzbot, int bztop, int* neighb_proc, int* neighb_block);

  virtual int FillInputData(vtkCPInputDataDescription* input);

protected:
  void UpdateRepresentation();

  int MaxLevel;
  int MinLevel;
  double MinLevelSpacing[3];
  int GlobalBlockSize[3];

  vtkBoundingBox Bounds;

  std::vector<std::string> CFieldNames;
  std::vector<std::vector<std::string> > MFieldNames;

  vtkIntArray* NeighborArray;

  bool AllocationsChanged;

  struct Block
  {
    int id;
    int allocated;
    int active;
    int level;
    int Nx, Ny, Nz;
    double *x, *y, *z;
    int bot[3];
    int top[3];
    int neighbor_proc[24];
    int neighbor_block[24];
    std::vector<vtkSmartPointer<vtkCTHDataArray> > CFieldData;
    int actualMaterials;
    std::vector<std::vector<vtkSmartPointer<vtkCTHDataArray> > > MFieldData;
    vtkUniformGrid* ug;
    vtkAMRBox* box;
  };
  std::vector<Block> Blocks;
  vtkSmartPointer<vtkNonOverlappingAMR> AMRSet;

  void AllocateBlock(Block& b);
  bool GetBounds(Block& b, int loCorner[3], int hiCorner[3]);
  void AddGhostArray(Block& b, int dx, int dy, int dz);
  void AddBlockIdArray(Block& b, int dx, int dy, int dz);
  void AddNeighborArray(Block& b);
  void AddAMRLevelArray(Block& b, int dx, int dy, int dz);
  void AddFieldArrays(Block& b);
  void AddFieldArrays(Block& b, int loCorner[3], int hiCorner[3]);
  void AddActivationArray(Block& b);
  void AddAttributesToAMR(vtkNonOverlappingAMR* amr);

private:
  vtkCTHSource(const vtkCTHSource&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCTHSource&) VTK_DELETE_FUNCTION;
};

#endif /* vtkCTHSource_h */
