
#include "assert.h"
#include "vtkTilesHelper.h"

static void vtkTest(int rank, vtkTilesHelper* helper, int minx, int miny, int maxx, int maxy)
{
  double viewport[] = { 0, 0, 1, 1 };
  double normalized_tile_viewport[4];
  helper->GetNormalizedTileViewport(viewport, rank, normalized_tile_viewport);
  cout << "---------------------" << endl
       << "Rank : " << rank << " in " << helper->GetTileDimensions()[0] << "x"
       << helper->GetTileDimensions()[1] << endl;
  cout << normalized_tile_viewport[0] << ", " << normalized_tile_viewport[1] << ", "
       << normalized_tile_viewport[2] << ", " << normalized_tile_viewport[3] << endl;

  int tile_viewport[4];
  helper->GetTileViewport(viewport, rank, tile_viewport);

  cout << "Expected value: " << minx << ", " << miny << ", " << maxx << ", " << maxy << endl;
  cout << "From vtkTilesHelper: " << tile_viewport[0] << ", " << tile_viewport[1] << ", "
       << tile_viewport[2] << ", " << tile_viewport[3] << endl;
  assert(tile_viewport[0] == minx);
  assert(tile_viewport[1] == miny);
  assert(tile_viewport[2] == maxx);
  assert(tile_viewport[3] == maxy);
}

int TestTilesHelper(int, char* [])
{
  vtkTilesHelper* helper = vtkTilesHelper::New();

  helper->SetTileDimensions(3, 2);
  helper->SetTileWindowSize(400, 400);
  vtkTest(1, helper, 400, 400, 799, 799);
  vtkTest(2, helper, 800, 400, 1199, 799);
  vtkTest(4, helper, 400, 0, 799, 399);
  helper->Delete();
  return 0;
}
