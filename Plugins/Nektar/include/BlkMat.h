#ifndef H_BLKMAT
#define H_BLKMAT

#include "Vmath.h"
#include "NekError.h"
#include "BlkSubMat.h"
#include "BlkVec.h"

#include <stdlib.h>
#include <iostream>
#include <vector>

using namespace std;

namespace NekBlkMat {

  // local Class Blk Matrix
  class BlkMat {
  public:
    /// default constructor
    BlkMat(int rowblk, int colblk){
      _max_rowblk = rowblk;
      _max_colblk = colblk;

      _offset = (int *)NULL;

      _Row = new BlkVec [_max_rowblk];
      for(int i =0; i < _max_rowblk; ++i)
  _Row[i].SetMem(_max_colblk);
    }

    /// default destructor
    ~BlkMat(){

      delete[] _Row;

      if(_offset)
  delete[] _offset;
    }

    void Reset_Rows(){
      int i,j;

      for(i=0; i < _max_rowblk; ++i)
  for(j=0;j < _Row[i]._Blk.size(); ++j)
    _Row[i].reset();
    }

    void GenBlk(const int row_id, const int col_id, const int rows,
    const int cols, const double *mat){
      _Row[row_id].GenBlk(col_id,rows,cols,mat);
    }

    void GenBlk(const int row_id, const int col_id, const int rows,
    const int cols, const int id1, const int id2,
    const double val){
      _Row[row_id].GenBlk(col_id,rows,cols,id1,id2,val);
    }

    int get_tot_row_entries(){
      if(!_offset) setup_offset();
      return _offset[_max_rowblk];
    }

    void setup_offset(){ // set up _offset if not already done at init

      if(!_offset) _offset = new int [_max_colblk+1];
      vmath::zero(_max_colblk+1,_offset,1);

      for(int i = 0; i < _max_rowblk; ++i)
  for(int j = 0; j < _Row[i]._Blk.size(); ++j)
    update_offset(_Row[i]._Blk[j]->get_id(),_Row[i]._Blk[j]->get_cols());
    }

    void update_offset(int id,int cols){
      int i;
      // check and update _offset - assumes
      if(_offset[id+1] == _offset[id]){ // assume cols not set
  for(i = id+1; i <= _max_colblk; ++i)
    _offset[i] += cols;
      }
    }


    void PrintBlks(){

      if(!_offset) setup_offset();

      for(int i = 0; i < _max_rowblk; ++i)
  _Row[i].PrintBlks(_offset);

    }

    void PlotBlks(){

      if(!_offset) setup_offset();

      cout << endl;
      cout << "Plotting Block: " << endl;
      cout << "\t + = positive value" << endl;
      cout << "\t * = negative value" << endl;
      cout << "\t - = zero value (|val|< 1e-12)" << endl;
      cout << endl;

      for(int i = 0; i < _max_rowblk; ++i)
  _Row[i].PlotBlks(_offset);

    }

    BlkMat& invert_diag();
    BlkMat& add    (const BlkMat& a, const BlkMat& b);
    BlkMat& sub    (const BlkMat& a, const BlkMat& b);
    BlkMat& geMxM  (MatStorage formA, MatStorage formB, const double& alpha,
        const BlkMat& A, const BlkMat& b,   const double& beta);
    BlkMat& MxM    (const BlkMat& a, const BlkMat& b);
    BlkMat& MtxM   (const BlkMat& a, const BlkMat& b);
    BlkMat& MxMt   (const BlkMat& a, const BlkMat& b);

    //(this) = A*(this) where A is block diagonal
    BlkMat& diagMxy (const BlkMat& A);

    // (this) = (this) + alpha * A
    BlkMat& axpy  (const double alpha, const BlkMat& A);

    double* geMxv  (MatStorage form, const double& alpha, const double *v,
        const double& beta,  double *y);
    double* Mxvpy  (double* v, double* y);
    double* Mtxvpy (double* v, double* y);

    int  get_mrowblk(){ return _max_rowblk;}
    int  get_mcolblk(){ return _max_colblk; }
    int* get_offset(){ if(!_offset) setup_offset(); return _offset;}

    int get_rows(const int i, const int j){
      int id;
      if((id = _Row[i]._entries[j])+1)
  return _Row[i]._Blk[id]->get_rows();
      else
  return -1;
    }

    int get_cols(const int i, const int j){
      int id;
      if((id = _Row[i]._entries[j])+1)
  return _Row[i]._Blk[id]->get_cols();
      else
  return -1;
    }

    double* get_mat(const int i, const int j, int& n, int &m){
      int id;
      if((id = _Row[i]._entries[j])+1){
  n = _Row[i]._Blk[id]->get_rows();
  m = _Row[i]._Blk[id]->get_cols();
  return _Row[i]._Blk[id]->get_mat();
      }
      else
  return (double*)NULL;
    }

    double get_val(const int i, const int j, const int id1, const int id2){
      int id;
      if((id = _Row[i]._entries[j])+1){
  int m = _Row[i]._Blk[id]->get_cols();
  return _Row[i]._Blk[id]->get_val(id1,id2);
      }
      else
  return (double) NULL;
    }

    void rescale(const int i, const int j, double *scale){
      int id;
      if((id = _Row[i]._entries[j])+1){
  _Row[i]._Blk[id]->rescale(scale);
      }
    }

    void neg(){
      int i,j;

      for(i=0; i < _max_rowblk; ++i)
  for(j=0;j < _Row[i]._Blk.size(); ++j)
    _Row[i]._Blk[j]->neg();
    }

    int cnt_blks(){
      int i,cnt;
      for(cnt = i =0; i < _max_rowblk; ++i)
  cnt += _Row[i]._Blk.size();

      return cnt;
    }

    double AmAt();

    // copy operator;
    BlkMat& operator = (const BlkMat& a){
      if(this != &a){
  Reset_Rows();

  _max_rowblk = a._max_rowblk;
  _max_colblk = a._max_colblk;

  for(int i = 0; i < _max_rowblk; ++i)
    _Row[i] = a._Row[i];

  setup_offset();
      }
      return *this;
    }

  private:
    int _max_colblk;
    int _max_rowblk;
    int *_offset;  ///< offset of dof entries in row

    BlkVec *_Row; // Block entry in Row
  };
}
#endif
