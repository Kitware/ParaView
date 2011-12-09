#ifndef H_BLKVEC
#define H_BLKVEC


#include "Vmath.h"
#include "NekError.h"
#include "BlkSubMat.h"
#include <stdlib.h>
#include <cmath>
#include <iostream>
#include <vector>

using namespace std;

namespace NekBlkMat {
  // local Class for differential matrix structure
  class BlkVec {
    friend class BlkMat;
    friend class BlkSubMat;
  public:
    /// default constructor
    BlkVec(){_max_blk = 0; }

    /// default destructor
    ~BlkVec(){
      for(int i = 0; i < _Blk.size(); ++i) delete _Blk[i];
      delete[] _entries;
    }

    /// default destructor
     void reset(){
       for(int i = 0; i < _Blk.size(); ++i) delete _Blk[i];
       _Blk.erase(_Blk.begin(),_Blk.end());
       vmath::fill(_max_blk,-1,_entries,1);
     }

     void SetMem(int nblk){
       _max_blk = nblk;
       _entries = new int[_max_blk];
       vmath::fill(_max_blk,-1,_entries,1);
     }

     void GenBlk(const int id, const int rows, const int cols,
     const double *mat);

     void GenBlk(const int id, const int rows, const int cols,
     const int id1, const int id2, const double val);

     void PrintBlks(const int *offset){
       int i,j,k,id,cols;
       double *mat;

       if(_Blk.size())
   for(i = 0; i < _Blk[0]->get_rows(); ++i){
     for(j = 0; j < _max_blk; ++j){
       cols = offset[j+1]-offset[j];
       if((id=_entries[j])+1){
         mat = _Blk[id]->get_mat();
         for(k = 0; k < cols; ++k)
         cout << mat[i*cols+k] << " ";
       }
       else
         for(k = 0; k < cols; ++k)
     cout <<  "0 ";
     }
     cout << endl;
   }
       else
   cout << "-- Empty row --" << endl;
     }

     void PlotBlks(const int *offset){
       int i,j,k,id,cols;
       double *mat;

       if(_Blk.size())
   for(i = 0; i < _Blk[0]->get_rows(); ++i){
     for(j = 0; j < _max_blk; ++j){
       cols = offset[j+1]-offset[j];
       if((id=_entries[j])+1){
         mat = _Blk[id]->get_mat();
         for(k = 0; k < cols; ++k)
     if(fabs(mat[i*cols+k]) > 1e-12)
       if(mat[i*cols+k] > 0.0)
         cout << "+ ";
       else
         cout << "* ";
     else
       cout << "- ";
       }
       else
         for(k = 0; k < cols; ++k)
     cout << "- ";
     }
     cout << endl;
   }
       else
   cout << "-- Empty row --" << endl;
     }

     BlkVec& add    (const BlkVec& a,    const BlkVec& b);
     BlkVec& sub    (const BlkVec& a,    const BlkVec& b);
     BlkVec& MxMpM  (const BlkSubMat& a, const BlkVec& b);
     BlkVec& MtxMpM (const BlkSubMat& a, const BlkVec& b);
     BlkVec& geMxM  (MatStorage formA, MatStorage formB,
         const double& alpha, const BlkSubMat& A,
         const BlkVec& B, const double& beta);
     // (this) = (this) + alpha * A
     BlkVec& axpy  (const double alpha, const BlkVec& A);

     double* Mxvpy  (const int *offset, const double *v, double *y);
     double* Mtxvpy (const int *offset, const double *v, double *y);
     double* geMxv  (MatStorage form, const double& alpha, const double *v,
         const double& beta, double *y, const int *offset);


     BlkVec& operator = (const BlkVec &a){
       if(this != &a){
   BlkSubMat *B;

   for(int i = 0; i < a._Blk.size(); ++i){
     B = new BlkSubMat(*a._Blk[i]);
      _entries[B->get_id()] = _Blk.size();
      _Blk.push_back(B);
   }
       }
  return *this;
     }

  private:

    int _max_blk;
    int *_entries;

    vector <BlkSubMat*> _Blk; // Block entry in Row
  };


}
#endif
