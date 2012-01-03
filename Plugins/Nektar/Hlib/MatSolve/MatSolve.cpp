#include "hotel.h"
#include "MatSolve.h"

namespace MatSolve{

static void Setup_LocMap(const int n, const int *map1, const int *map2,
       const int nmap2, int *newmap, const int noffset,
       const int cstart, const int bndtot, const int nsolve);

  // Standard Positive definite matrix setup from Nektar
  void NekMat::SetupSC(Element_List& U,Bsystem *B,Bndry *Ubc,SolveType Stype){
    int i,j,n,id,cnt;
    int neid,lev,nmat,nmat1;
    Element *E;
    SCmat *Abase;
    LocMat *Mat;
    Bndry *Bc;
    Recur *rdata;
    double *sc;
    int symmetric,*offset;


    /***************General memory and mapping setup **************/

    SlvType = Direct; // just direct solver at present

    if(B->lambda->wave)
      symmetric = 0; // assume setup  for advection diffusion eqn.
    else
      symmetric = 1; // use as flag to set matrices

    if(B->rslv){
      SClevels = B->rslv->nrecur+1;
      rdata = B->rslv->rdata;
    }
    else
      SClevels = 1;

    SClev = new SClevel [SClevels];
    SClev[0].Setup_BlkMat(U.nel,symmetric);

    // declare first level of matrices from Element_List

    if(B->lambda->wave)
      option_set("Oseen",1);// required for mat_mem to allocate correct storage

    // do sign change and assemble global matrix
    if(!B->signchange) setup_signchange(&U,B);
    SClev[0]._signchange = B->signchange;

    for(E = U.fhead; E;E=E->next){
      Mat = E->mat_mem (); // Really should just declare once!

      id = E->id; // reset id for this case
      switch(Stype){
      case Mass:
  if(!E->curvX)
    E->MassMat(Mat);
  else
    E->MassMatC(Mat);
  break;
      case Helm:
  if(E->identify()==Nek_Tri && !E->curvX && !B->lambda->wave)
    Tri_HelmMat(E, Mat, B->lambda[E->id].d);
  else
    E->HelmMatC (Mat,B->lambda+E->id);

  // Check for robin bc's
  for(Bc=Ubc;Bc;Bc=Bc->next)
    if(Bc->type == 'R' && Bc->elmt->id == E->id)
      Add_robin_matrix(E, Mat, Bc);

  break;
      }
      // project into sclevel
      SClev[0]._Am->GenBlk(id,id,Mat->asize,Mat->asize,Mat->a[0]);
      SClev[0]._Bm->GenBlk(id,id,Mat->asize,Mat->csize,Mat->b[0]);
      SClev[0]._Cm->GenBlk(id,id,Mat->csize,Mat->csize,Mat->c[0]);

      if(!symmetric)
  SClev[0]._Dm->GenBlk(id,id,Mat->asize,Mat->csize,Mat->d[0]);

      SClev[0].Set_State(Filled);
      SClev[0]._Asize += Mat->asize;
      SClev[0]._Csize += Mat->csize;

      E->mat_free(Mat);
    }

    SClev[0].Condense();


    /*******************Recursive setup *******************************/

    // set up multilevel information from B->rslv
    for(i = 1; i < SClevels; ++i){
      SClev[i].Setup_BlkMat(rdata[i-1].npatch,symmetric);

      // setup element mapping array for previous level
      nmat   = SClev[i-1]._Am->get_mrowblk();
      SClev[i-1]._eidmap = new int[nmat];
      vmath::vcopy(nmat,rdata[i-1].pmap,1,SClev[i-1]._eidmap,1);
    }

    int *noffsetA = new int[U.nel+1]; // zero level has most submatrices
    int *noffsetC = new int[U.nel+1]; // zero level has most submatrices

    for(lev = 0; lev < SClevels-1; ++lev){
      SClev[lev]._locmap = new int[ SClev[lev]._Am->get_tot_row_entries()];


      // set up multilevel local to local mappings
      // using local to global mappings in rslv
      noffsetA[0] = 0;       // temporary offset array
      noffsetC[0] = 0;       // temporary offset array
      nmat1   = SClev[lev+1]._Am->get_mrowblk();
      for(j =1 ; j < nmat1+1; ++j){
  noffsetA[j] = noffsetA[j-1] + rdata[lev].patchlen_a[j-1];
  noffsetC[j] = noffsetC[j-1] + rdata[lev].patchlen_c[j-1];
      }
      // add number of A matrices to C offset
      vmath::sadd(nmat1+1,noffsetA[nmat1],noffsetC,1,noffsetC,1);

      offset = SClev[lev]._Am->get_offset();
      nmat   = SClev[lev]._Am->get_mrowblk();  // redefine nmat

      for(j = 0,cnt=0; j < nmat; ++j) {
  neid = SClev[lev]._eidmap[j];
  if(lev){
    Setup_LocMap(offset[j+1]-offset[j], rdata[lev-1].map[j],
           rdata[lev].map[neid],noffsetA[neid+1]-noffsetA[neid],
           SClev[lev]._locmap+cnt,noffsetA[neid],rdata[lev].cstart,
           noffsetA[nmat1],B->nsolve);
  }
  else{
    Setup_LocMap(offset[j+1]-offset[j], B->bmap[0]+cnt,
           rdata[lev].map[neid], noffsetA[neid+1]-noffsetA[neid],
           SClev[lev]._locmap+cnt, noffsetA[neid],
           rdata[lev].cstart,noffsetA[nmat1],B->nsolve);
  }
  cnt += offset[j+1]-offset[j];
      }

      /**********Recursive matrix assembly and condensation **************/

      // assemble new matrices using previous level A matrix
      SClev[lev].Assemble(SClev[lev+1],noffsetA,noffsetC);

      // free A memory (after setting up offset)
      SClev[lev]._Am->setup_offset();
      SClev[lev]._Am->Reset_Rows();

      //statically condense new matrices;
      SClev[lev+1].Condense();
    }

    delete[] noffsetA;
    delete[] noffsetC;

    nmat   = SClev[SClevels-1]._Am->get_mrowblk();
    offset = SClev[SClevels-1]._Am->get_offset();

    SClev[SClevels-1]._locmap = new int[offset[nmat]];

    if(SClevels > 1) // use rslv mapping
      for(j = 0,cnt=0; j < nmat; ++j,cnt+=n){
  vmath::vcopy(offset[j+1]-offset[j],rdata[SClevels-2].map[j],1,
         SClev[SClevels-1]._locmap+offset[j],1);
      }
    else{ // use bmap
      vmath::vcopy(offset[nmat],B->bmap[0],1,SClev[SClevels-1]._locmap,1);
      // set values equal to or above nsolve to -1
      for(j = 0; j < offset[nmat]; ++j)
  if(SClev[SClevels-1]._locmap[j] >= B->nsolve)
    SClev[SClevels-1]._locmap[j] = -1;
    }


    /***************Inner solve Matrix setup and factorisation **************/
    if(SlvType == Direct){
      lev = SClevels-1;

      // setup direct matrix;
      Ainv = new Gmat;
      Ainv->set_lda(SClev[lev]._locmap[vmath::imax(offset[nmat],
               SClev[lev]._locmap,1)]+1);

      // Needs sorting
      Ainv->set_bwidth(SClev[lev].bandwidth());

      if(B->lambda->wave){ // advection diffusion operator
  if(3*Ainv->get_bwidth() > Ainv->get_lda())
    Ainv->mat_form = General_Full;
  else{
    Ainv->mat_form = General_Banded;
    Ainv->set_ldiag(Ainv->get_bwidth()-1);
  }
      }
      else{
  if(2*Ainv->get_bwidth() > Ainv->get_lda())
    Ainv->mat_form = Symmetric_Positive;
  else
    Ainv->mat_form = Symmetric_Positive_Banded;
      }

      //assemble into global matrix system taking account of signchange if nec.
      Ainv->Assemble(SClev[lev]._Am[0],SClev[lev]._locmap,
         SClev[lev]._signchange);

      // release memory associated with row matrix storage but set up offset !
      SClev[lev]._Am->setup_offset();
      SClev[lev]._Am->Reset_Rows();

      Ainv->Factor();
    }
    else
      Ainv = NULL;
  }

  NekMat::~NekMat(){
    int i;

    delete[] SClev;

    if(Ainv) delete Ainv;
  }



  /* using \a map1 which is the local to global mapping for this level
     and \a map2 which is the local to global mapping of the next SC level
     generate a local to local mapping in \a newmap.

     \a cstart is the current break in the level between coupled and
     decoupled dof.

     \a cnt is an offset for the current level to determine the interior maps

     \a noffset is a constant to add to the mapping which denotes the
     subsequent spacing of the new elements
  */
  static void Setup_LocMap(const int n, const int *map1, const int *map2,
         const int nmap2, int *newmap, const int noffset,
         const int cstart, const int bndtot,
         const int nsolve){
    int i,j;

    vmath::fill(n,-10,newmap,1);  // debug value

    for(i = 0; i < n; ++i){
      if(map1[i] < nsolve){
  if(map1[i] < cstart){
    for(j = 0; j < nmap2; ++j)
      if(map1[i] == map2[j])
        newmap[i] = j + noffset;
  }
  else // replace condensed global bnd modes with local bnd modes
    newmap[i] = map1[i] - cstart + bndtot;
      }
      else
  newmap[i] = -1;
    }

  }

  /** Solve the matrix problem defined by Stype taking

  On entry:
  - U contains the initial guess.
  - Uf contains the right hand side without boundary conditions imposed
  - Ubc is Boundary condition imposition

  on exit:
  - U contains solution
  - Uf is modified.
  - All other arguments are unchanged

  Solution follows

  \f$   A u = b \f$ which is equivalent to \f$ A \Delta u = b - A u_0 = b^*  \f$
  - Step 1: Set boundary conditions in U   (SetBCs)
  - Step 2: Set up \f$ f^* = f - A u_0 \f$ (SetRHS)
  - Step 3: Solve  \f$\Delta u = f^*\f$    (Solve)
  - Step 4: set \f$ u = \Delta u + u_0 \f$
  */
  void NekMat::Solve(Element_List& U, Element_List& Uf, Bndry& Ubc,
         Bsystem& Ubsys, SolveType Stype){
    if(U.base_hj == Uf.base_hj)
      cerr << "Warning: in NekMat::Solve U must be independent of Uf"<<endl;

    SetBCs(&U,&Ubc,&Ubsys);

    // set up rhs and leave in Uf
    SetRHS(U,Uf,Ubc,Ubsys.lambda[0], Stype);

    // Solve for perturbation from initial field in U
    Solve(Uf.base_hj);

    // Add perturbation stored in Uf to U
    vmath::vadd(Uf.hjtot,Uf.base_hj,1,U.base_hj,1,U.base_hj,1);
  }


  /**   Generates \f$ f^* = f - A u_0 \f$.

  Given a solution vector with the correct BC's in U and a rhs
  vector containing the forcing function in Uf this routine sets
  up a local vector of the rhs terms which is modified to take
  into account the Dirichet, Robin and Neumann boundary
  conditions.

  */
  void NekMat::SetRHS(Element_List& U, Element_List& Uf, Bndry& Ubc,
          Metric& lambda, SolveType Stype){
    Bndry *B;

    if(Uf.fhead->state == 'p')  /* take inner product if in physical space */
      Uf.Iprod(&Uf);

    if(Stype == Helm)            /* negate if helmholtz solve               */
      vmath::neg(Uf.hjtot, Uf.base_hj, 1);

    /* add flux terms      */
    for(B = &Ubc;B; B = B->next)
    if(B->type == 'F' || B->type == 'R')
      Uf.flist[B->elmt->id]->Add_flux_terms(B);

    // Impose dirichlet bc's

    double *u_0 = new double[U.hjtot];

    blas::dcopy(U.hjtot,U.base_hj,1,u_0,1);

    if(Stype == Helm){
      U.Trans(&U, J_to_Q);
      U.HelmHoltz(&lambda);
    }
    else{
      U.Trans(&U, J_to_Q);
      U.Iprod(&U);
    }

    vmath::vsub(U.hjtot,Uf.base_hj,1,U.base_hj,1,Uf.base_hj,1);
    blas ::dcopy(U.hjtot,u_0,1,U.base_hj,1);
    delete[] u_0;
  }

  // Solve Au = f using statically condensed system according to NekMat setup
  void NekMat::Solve(double *f){
    int i,j,id,lev,cnt;
    int lda = Ainv->get_lda();
    double *sc,*b;
    double **rhs = new double * [SClevels+1];

    // Assemble vector space for multi-level static condensation
    for(lev = 0; lev < SClevels; ++lev){
      rhs[lev] = new double [SClev[lev]._Asize + SClev[0]._Csize];
      vmath::zero(SClev[lev]._Asize + SClev[0]._Csize,rhs[lev],1);
    }
    // inner solve vector
    rhs[SClevels] = new double[lda];
    vmath::zero(lda,rhs[SClevels],1);

    // resort input vector into boundary dof followed by int dof
    SClev[0].Pack_BndInt(f,rhs[0]);

    // Do multilevel static condensation of rhs
    for(lev = 0; lev < SClevels; ++lev){
      SClev[lev].CondenseV(rhs[lev]);
      SClev[lev].PackV(rhs[lev],rhs[lev+1]);
    }

    // Solve boundary system
    Ainv->Solve(rhs[SClevels],1);


    // Unpack and solve for interior DOF in each  level
    for(lev = SClevels-1; lev >= 0; --lev){
      SClev[lev].UnPackV(rhs[lev+1],rhs[lev]);
      SClev[lev].SolveVd(rhs[lev]);
    }

    // resort  vector into elemental form
    SClev[0].UnPack_BndInt(rhs[0],f);

    // free rhs data
    for(i = 0; i <= SClevels; ++i)
      delete[] rhs[i];
    delete[] rhs;
  }


} // end of namespace
