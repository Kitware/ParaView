#ifndef H_SCLEVEL
#define H_SCLEVEL

#define ASS_TOL 1e-16

#include "BlkMat.h"
#include <math.h>

namespace MatSolve{
  enum SCStatus {
    Unset,          ///< initialised matrix but not set
    Filled,         ///< Matrix is filled with components
    Condensed       ///< Matrix is statically condensed
  };

  class SClevel {
    friend class NekMat;
    friend class DGMatrix;
    friend class StokesMatrix;
  private:
    int  _symmetric; ///< Flag to identify if matrix is symmetric. If
                    ///< symmetric = 1 \f$ D^T = B\f$;
    int  _Asize; ///< Total number of dof in A matrix (boundary dof)
    int  _Csize; ///< Total number of dof in C matrix (interior dof)
    BlkMat *_Am; ///< if state = Filled: A \n
                ///< if state = Condensed -> \f$  A-BC^{-1}D   \f$ \n
    BlkMat *_Bm; ///< if state = Filled: B\n
                 ///< if state = Condensed = 1:\f$ BC^{-1}\f$
    BlkMat *_Cm; ///< if state = Filled: C\n
                ///< if state = Condensed: \f$ C^{-1}\f$
    SClevel *_Csc; ///< if state = Filled: C\n
                  ///< if state = Condensed: then this SC levels are condensed
                  /// This is an alternative if BlkMat not defined - assumes that All Blk Matrices are defined within.
    BlkMat *_Dm; ///< if state = Filled: D\n
                ///< if state = Condensed = 1: \f$ C^{-1}D\f$\n
                ///< (note D is stored in row major form so if matrix is
                ///< \a symmetric (=1) then \f$D^T\f$ = B)

    SCStatus _state;  ///< Flag to check whether matrix is condensed\n
                      ///<  -- Unset    : only initialised\n
                      ///<  -- Filled   : standard form\n
                      ///<  -- Condensed: condensed \n

    int   *_locmap;  /*  mapping from this level to next level.  If
      this is the innermost level then locmap is
      local to global mapping. */
    int    *_eidmap;      ///< Local id mapping matrices to next level
    double *_signchange;  ///< signchange vector for local to global mapping
                         ///< If not declare then assume not required

  public:
    SClevel(){  ///< default constructor;
      _state = Unset;
      _Asize = _Csize = 0;
      _eidmap = (int *) NULL;
      _signchange = (double *) NULL;
      _Am = _Bm = _Cm = _Dm = (BlkMat *) NULL;
    }

    ~SClevel(){ ///< default destructor
      if(_eidmap) delete[] _eidmap;
      // currently do not delete signchange as not directly attached to class
      if(_Am)   delete _Am;
      if(_Bm)   delete _Bm;
      if(_Cm)  {delete _Cm; _Cm = (BlkMat *) NULL;}
      if(_Csc) {delete[] _Csc; _Csc = (SClevel *) NULL;}
      if(_Dm)   delete _Dm;
    }

    void Setup_BlkMat(int n, int symmetric){
      _symmetric = symmetric;

      _Am = new BlkMat(n,n);
      _Bm = new BlkMat(n,n);
      _Cm = new BlkMat(n,n);
      if(!_symmetric) _Dm = new BlkMat(n,n);
      else _Dm = _Bm;
    }

    void Setup_BlkMatSC(int n, int symmetric, int m){
      _symmetric = symmetric;

      _Am  = new BlkMat(n,n);
      _Bm  = new BlkMat(n,n);
      _Csc = new SClevel [n];
      for(int i=0; i <n; ++i)
  _Csc[i].Setup_BlkMat(m,m);
      if(!_symmetric) _Dm = new BlkMat(n,n);
      else _Dm = _Bm;
    }

    void Set_State(SCStatus state){
      _state = state;
    }

    /**
       Static condense matrix so that on output.

       -   \f$ A   \rightarrow  A - BC^{-1}D\f$   (in \e row major form)
       -   \f$ B   \rightarrow  BC^{-1}\f$        (in \e row major form)
       -   \f$ C   \rightarrow  C^{-1}\f$         (in \e row major form)
       -   \f$ D^T \rightarrow  D^TC^{-T}\f$      (in \e row major form)
       or  [\f$ D   \rightarrow  C^{-1}D\f$       (in \e column major form) ]

       \n
    */
    /* Memory can be reduced for symmetric case by making a special
       operator for A-BCD i.e. ymMxDxM */
    void Condense(){
      BlkMat *BCinv;

      if(_state != Filled)
  NekError::error(fatal,"SClev::Condense()","Matrices not filled");

      _state = Condensed;

      //  BC^{-1} storage depending on symmetry of matrix
      BCinv = new BlkMat(_Bm->get_mrowblk(),_Bm->get_mcolblk());

      // C = C^{-1}
      if(_Cm){
  _Cm->invert_diag();

  BCinv->MxM(*_Bm,*_Cm);
      }
      else{
  for(int i = 0; i < _Bm->get_mcolblk(); ++i){
    _Csc[i].Condense();
    _Csc[i]._Am->invert_diag();
  }

  BCinv[0] = _Bm[0];
  Invert_T(_Csc,*BCinv);
      }

      // A = A - BC^{-1}D
      _Am->geMxM(RowMajor,ColMajor,-1.0,*BCinv,*_Dm,1.0);

      if(_symmetric){ // B = D^T C^{-1}
  // Link B and D to BCinv
  delete _Bm;
  _Dm = _Bm = BCinv;
      }
      else{ // D^T = D^T C^{-1} using B storage
  if(_Cm){
    _Bm->geMxM(RowMajor,ColMajor,1,*_Dm,*_Cm,0);
    delete _Dm;
    _Dm = _Bm;
  }
  else{
    Invert_T(_Csc,*_Dm);
    delete _Bm;
  }

  // Link B to BCinv
  _Bm = BCinv;
      }
    }

    /**
  Calcuate (this){^-1} by Matrix and assuming that matrix is in Column Matrix form.
    **/

    void Invert_T(SClevel *Csc, BlkMat &M){
      int i,j,m,n;
      int rows = M.get_mrowblk();
      int cols = M.get_mcolblk();
      double *mat;

      for(i = 0; i < cols; ++i){
  for(j = 0; j < rows; ++j)
    if(mat = M.get_mat(j,i,n,m)){  // do static condensation solve
      Csc[i].SolveFull(mat+i*m);
    }
      }
    }


    /** \brief Condense interior modes from boundary modes
  \f$ rhs = f_b - B C^{-1} f_i \f$
     */
    void CondenseV(double *rhs){
      _Bm->geMxv(RowMajor,-1,rhs+_Asize,1,rhs);
      if(_signchange) vmath::vmul(_Asize,_signchange,1,rhs,1,rhs,1);
    }

    /**  Pack all boundary elements in array \a in to array \a out according
   to the mapping given by \a (*this)->locmap.

   if locmap[i] == -1 then do not pack points
    */
    void PackV(const double *in, double *out){
      int i,id;
      // Assemble current vector from last mapping
      for(i = 0; i < _Asize; ++i)
  if((id = _locmap[i])+1) // needed at lev-1 = 0
  out[id] += in[i];
    }

    /**  UnPack all boundary elements in array \a in to array \a out according
   to the mapping given by \a (*this)->_locmap.


   if locmap[i] == -1 then do not pack points
    */
    void UnPackV(const double *in, double *out){
      int i,id;
      for(i = 0; i < _Asize; ++i)
  if((id = _locmap[i])+1)
    out[i] = in[id];
  else
    out[i] = 0.0; //need to be zero for correct interior solve
    }



    void Pack_BndInt(double *f,double *fs){
      int i,n,na,nc,npm;
      int *offseta,*offsetc;

      n       = _Am->get_mrowblk();
      offseta = _Am->get_offset();
      offsetc = _Cm->get_offset();

      for(npm = i = 0; i < n; ++i){
  na = offseta[i+1]-offseta[i];
  nc = offsetc[i+1]-offsetc[i];
  vmath::vcopy(na,f+npm,1,fs+offseta[i],1);
  vmath::vcopy(nc,f+npm+na,1,fs+offseta[n]+offsetc[i],1);
  npm += na+nc;
      }
    }

    void UnPack_BndInt(double *f,double *fs){
      int i,n,na,nc,npm;
      int *offseta,*offsetc;

      n       = _Am->get_mrowblk();
      offseta = _Am->get_offset();
      offsetc = _Cm->get_offset();

      for(npm = i = 0; i < n; ++i){
  na = offseta[i+1]-offseta[i];
  nc = offsetc[i+1]-offsetc[i];
  vmath::vcopy(na,f+offseta[i],1,fs+npm,1);
  vmath::vcopy(nc,f+offseta[n]+offsetc[i],1,fs+npm+na,1);
  npm += na+nc;
      }
    }


    /** \brief Solve interior modes
  Require the vector V to be packed by a call to PackV
    */
    void SolveVd(double *V){
      int     i;

      if(_signchange) vmath::vmul(_Asize,_signchange,1,V,1,V,1);

      if(_Csize){
  double *u = new double [_Csize];
  vmath::zero(_Csize,u,1);
  // u = C^{-1} f_i
  if(_Cm)
    _Cm->Mxvpy(V+_Asize,u);
  else{
    int cols = _Bm->get_mcolblk();
    int *offset = _Bm->get_offset();

    vmath::vcopy(_Csize,V+_Asize,1,u,1);

    for(i = 0; i < cols; ++i)
      _Csc[i].SolveFull(u+offset[i]);
  }

  // u -= C^{-1}D u_b
  _Dm->geMxv(ColMajor,-1,V,1,u);
  vmath::vcopy(_Csize,u,1,V+_Asize,1);
  delete[] u;
      }
    }

    /** \brief Solve condensed system
    */
    void SolveFull(double *V){
      int     i,n;
      double *b,*sc;
      double *u = new double [_Asize+_Csize];

      // f_b - BC^{-1} f_i
      CondenseV(V);

      // u = A^{-1} f_b
      _Am->geMxv(RowMajor,1,V,0,u);

      if(_Csize){  // u = C^{-1} f_i
  _Cm->Mxvpy(V+_Asize,u);
  // u -= C^{-1}D u_b
  _Dm->geMxv(ColMajor,-1,V,1,u);
  vmath::vcopy(_Csize,u,1,V+_Asize,1);
      }
      delete[] u;
    }

    void Assemble(SClevel& NewSClev, const int *noffsetA, const int *noffsetC){
      int p,q,i,j,id,id1,id2,nr,nc,neidr,neidc;
      int nrow, max_A, *offset;
      double val,*Asub;

      nrow   = _Am->get_mrowblk(); // number of Blks in (this) SClevel sys
      offset = _Am->get_offset();

      max_A = noffsetA[NewSClev._Am->get_mrowblk()]; // max number of A entries

      for(p = 0; p < nrow; ++p) // assumes system is square
  for(q = 0; q < nrow; ++q)
    if(Asub = _Am->get_mat(p,q,nr,nc)){

      if(_signchange )  // signchange necessary on first level
        if(p == q)
    _Am->rescale(p,q,_signchange+offset[p]);
        else
    NekError::error(warning,"SClevel::Assemble",
      "Attempt to change sign on off diagonal block");

      neidr = _eidmap[p];
      neidc = _eidmap[q];

      // project local matrix
      for(i =0; i < nr; ++i)
        for(j = 0; j < nc; ++j){
    id1 = _locmap[offset[p]+i];
    id2 = _locmap[offset[q]+j];
    val = _Am->get_val(p,q,i,j);

    if((id1+1)&&(id2+1)){
      if((id1 < max_A)&&(id2 < max_A)){ // project to _A
        id1 -= noffsetA[neidr];
        id2 -= noffsetA[neidc];
        NewSClev._Am->GenBlk(neidr,neidc,noffsetA[neidr+1]-
          noffsetA[neidr],noffsetA[neidc+1]-
          noffsetA[neidc],id1,id2,val);
      }
      else if((id1 < max_A)&&(id2 >= max_A)){ // project to _B
        id1 -= noffsetA[neidr];
        id2 -= noffsetC[neidc];

        NewSClev._Bm->GenBlk(neidr,neidc,noffsetA[neidr+1]-
          noffsetA[neidr],noffsetC[neidc+1]-
          noffsetC[neidc],id1,id2,val);
      }
      else if((id1 >= max_A)&&(id2 >= max_A)){ // project to _C
        id1 -= noffsetC[neidr];
        id2 -= noffsetC[neidc];

        if(fabs(val) > ASS_TOL){
          NewSClev._Cm->GenBlk(neidr,neidc,noffsetC[neidr+1]-
             noffsetC[neidr],noffsetC[neidc+1]-
             noffsetC[neidc],id1,id2,val);
          if(neidr != neidc)
      NekError::error(warning,"SClevel::Assemble",
          "C matrix has non-diagonal blocks");
        }
      }
      else if (!_symmetric){
        if((id1 >= max_A)&&(id2 < max_A)){ // project to _D^T
          id1 -= noffsetC[neidr];
          id2 -= noffsetA[neidc];

          NewSClev._Dm->GenBlk(neidc,neidr,noffsetA[neidr+1]-
            noffsetA[neidr],noffsetC[neidc+1]-
            noffsetC[neidc],id2,id1,val);
        }
      }
    }
        }

      if(_signchange )  // signchange back if necc.
        if(p == q)
    _Am->rescale(p,q,_signchange+offset[p]);
        else
    NekError::error(warning,"SClevel::Assemble",
      "Attempt to change sign on off diagonal block");
    }

      if(_symmetric)
  NewSClev._Dm = NewSClev._Bm;


      NewSClev.Set_State(Filled);
      NewSClev._Asize = max_A;
      NewSClev._Csize = noffsetC[NewSClev._Am->get_mrowblk()] - max_A;

    }


    int bandwidth(){
      int  i,j,k,l,val1,val2,n,nblk,bwidth= 0;
      int *offset;

      // find highest and lowest entry on every element and the
      // maximum of this will be the global bandwidth

      nblk   = _Am->get_mrowblk();
      offset = _Am->get_offset();


      for(i = 0;  i < nblk; ++i)
  for(j = 0; j < nblk; ++j)
    if(_Am->get_mat(i,j,n,n))
      for(k = 0; k < n; ++k)
        if((val1 = _locmap[offset[i]+k])+1)
    for(l = 0; l < n; ++l)
      if((val2 = _locmap[offset[j]+l])+1)
        bwidth = max(bwidth,abs(val2-val1+1));

      return bwidth;
    }
  };

} // end of namespace;
#endif
