/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_SparseMatrix.h
 *  \brief  Definition of class \c GW_SparseMatrix
 *  \author Gabriel Peyré
 *  \date   6-8-2003
 */
/*------------------------------------------------------------------------------*/

#ifndef _GW_SPARSEMATRIX_H_
#define _GW_SPARSEMATRIX_H_

#include "GW_MathsConfig.h"


extern "C" {
#include <laspack/errhandl.h>
#include <laspack/vector.h>
#include <laspack/qmatrix.h>
#include <laspack/operats.h>
#include <laspack/precond.h>
#include <laspack/rtc.h>
#include <xc/getopts.h>
}

namespace GW {

typedef Vector LSP_Vector;
typedef QMatrix LSP_Matrix;

/*------------------------------------------------------------------------------*/
/**
 *  \class  GW_SparseMatrix
 *  \brief  A sparse matrix.
 *  \author Gabriel Peyré
 *  \date   6-8-2003
 *
 *  It is a wrapper \c LASPACK library.
 */
/*------------------------------------------------------------------------------*/

std::ostream& operator<<(std::ostream &s, class GW_SparseMatrix& A);

class GW_SparseMatrix
{
public:

    enum T_IterativeSolverType
    {
        /* classical iterative methods */
        IterativeSolver_Jacobi,
        IterativeSolver_SORForw,
        IterativeSolver_SORBackw,
        IterativeSolver_SSOR,
        /* semi-iterative methods */
        IterativeSolver_Chebyshev,
        /* CG and CG-like methods */
        IterativeSolver_CG,
        IterativeSolver_CGN,
        IterativeSolver_GMRES,
        IterativeSolver_BiCG,
        IterativeSolver_QMR,
        IterativeSolver_CGS,
        IterativeSolver_BiCGSTAB,
    };

    enum T_PreconditionerType
    {
        Preconditioner_Jacobi,
        Preconditioner_SSOR,
        Preconditioner_ILU,
        Preconditioner_NULL            // no preconditioner
    };

    GW_SparseMatrix( GW_U32 nSize, ElOrderType ElOrder = Rowws )
    {
        Q_Constr( &M_,  "GW_SparseMatrix", nSize, False, ElOrder, Normal, True );
        RowPosition_ = new GW_U32[nSize];
        memset( RowPosition_, 0, nSize*sizeof(GW_U32) );
    }
    GW_SparseMatrix( GW_SparseMatrix& Original )
    {
        Q_Constr( &M_,  "GW_SparseMatrix", Original.GetDim(), False, Q_GetElOrder( &Original.M_ ), Normal, True );
        RowPosition_ = new GW_U32[Original.GetDim()];
        memset( RowPosition_, 0, Original.GetDim()*sizeof(GW_U32) );
        (*this) = Original;
    }
    GW_SparseMatrix( GW_MatrixNxP& Original, ElOrderType ElOrder = Rowws  )
    {
        GW_ASSERT( Original.GetNbrRows()==Original.GetNbrCols() );
        GW_U32 nSize = GW_MIN(Original.GetNbrRows(),Original.GetNbrCols());
        Q_Constr( &M_,  "GW_SparseMatrix", nSize, False, ElOrder, Normal, True );
        RowPosition_ = new GW_U32[nSize];
        memset( RowPosition_, 0, nSize*sizeof(GW_U32) );
        /* copy data */
        this->BuildFromFull(Original);
    }
    virtual ~GW_SparseMatrix()
    {
        Q_Destr( &M_ );
        GW_DELETEARRAY(RowPosition_);
    }
    GW_SparseMatrix& operator =( GW_SparseMatrix& Original )
    {
        GW_ASSERT( this->GetDim()==Original.GetDim() );
        GW_SparseMatrix::Copy( *this, Original.M_ );
        return *this;
    }

    void BuildFromFull( GW_MatrixNxP& Original )
    {
        GW_U32 nSize = Original.GetNbrRows();
        /* copy data */
        for( GW_U32 i=0; i<nSize; ++i )
        {
            /* count number of non-zero elements */
            GW_U32 nNonZero = 0;
            for( GW_U32 j=0; j<nSize; ++j )
                if( Original.GetData(i,j)!=0.0 )
                    nNonZero++;
            this->SetRowSize(i,nNonZero);
            for( GW_U32 j=0; j<nSize; ++j )
                if( Original.GetData(i,j)!=0.0 )
                    this->SetData( i,j, Original.GetData(i,j) );
        }
    }

    GW_U32 GetDim()
    {
        return (GW_U32) Q_GetDim(&M_);
    }

    //-------------------------------------------------------------------------
    /** \name accessors */
    //-------------------------------------------------------------------------
    //@{
    GW_Float GetData(GW_U32 i, GW_U32 j)
    {
        GW_ASSERT( i<this->GetDim() && j<this->GetDim() );
        return Q_GetEl(&M_, i+1,j+1);
    }
    GW_U32 GetRowSize( GW_U32 i )
    {
        return (GW_U32) Q_GetLen( &M_, i+1 );
    }
    void SetRowSize( GW_U32 i, GW_U32 nSize )
    {
        GW_ASSERT( i<this->GetDim() );
        GW_ASSERT( nSize<=this->GetDim() );
        Q_SetLen( &M_, i+1, nSize );
        // restart the counter ...
        RowPosition_[i] = 0;
    }
    void ResetRowSize( GW_U32 i, GW_U32 nSize )
    {
        GW_ASSERT( i<this->GetDim() );
        GW_ASSERT( nSize<=this->GetDim() );
        /* we must save the previous stuff */
        std::list<GW_U32> PosList;
        std::list<GW_Float> ValList;
        for( GW_U32 k=0; k<RowPosition_[i]-1; ++k )
        {
            ValList.push_back( Q_GetVal( &M_, i+1, k ) );
            PosList.push_back( (GW_U32) Q_GetPos( &M_, i+1, k ) );
        }
        /* now reset the length */
        Q_SetLen( &M_, i+1, nSize );
        RowPosition_[i] = 0;
        /* now reset the previous entry */
        std::list<GW_Float>::iterator it_val = ValList.begin();
        for( std::list<GW_U32>::iterator it_pos = PosList.begin(); it_pos!=PosList.end(); ++it_pos )
        {
            GW_ASSERT( it_val!=ValList.end() );
            GW_Float val = *it_val;
            GW_U32 pos = *it_pos;    GW_ASSERT( pos>0 );
            this->SetData( i, pos-1, val );
            ++it_val;
        }
        GW_ASSERT( it_val==ValList.end() );
    }
    void SetData( GW_U32 i, GW_U32 j, GW_Float rVal )
    {
        GW_ASSERT( i<this->GetDim() && j<this->GetDim() );
        GW_ASSERT( this->GetRowSize(i)>0 && RowPosition_[i]<this->GetRowSize(i) );
        RowPosition_[i]++;
        Q_SetEntry(&M_, i+1, RowPosition_[i]-1, j+1, rVal);    // matrix, row, entry, col, val
    }
    //@}

    /** access element by element */
    GW_Float AccessEntry( GW_U32 i, GW_U32 entry, GW_U32& j )
    {
        GW_ASSERT( entry<this->GetRowSize(i) );
        if( entry<this->GetRowSize(i) )
        {
            j = (GW_U32) Q_GetPos(&M_, i+1, entry) - 1;
            return Q_GetVal(&M_, i+1, entry);
        }
        return 0;
    }

    //-------------------------------------------------------------------------
    /** \name multiplication methods (both optimised and non-optimised) */
    //-------------------------------------------------------------------------
    //@{
    GW_VectorND operator*(GW_VectorND& v)
    {
        GW_VectorND r(v.GetDim());
        GW_SparseMatrix::Multiply( *this, v, r );
        return r;
    }
    static void Multiply(GW_SparseMatrix& a, GW_VectorND& v, GW_VectorND& r)
    {
        GW_ASSERT( a.GetDim()==v.GetDim() );
        GW_ASSERT( a.GetDim()==r.GetDim() );
        LSP_Vector v1;
        V_Constr( &v1, "temp", v.GetDim(), Normal, True );
        GW_SparseMatrix::Copy( v1, v );
        Asgn_VV( &v1, Mul_QV( &a.M_, &v1 ) );
        GW_SparseMatrix::Copy( r, v1 );
        V_Destr( &v1 );
    }
    //@}

    GW_SparseMatrix Transpose()
    {
        ElOrderType ElOrder = Q_GetElOrder(&M_);
        if( ElOrder == Rowws )
            ElOrder = Clmws;
        else if( ElOrder == Clmws )
            ElOrder = Rowws;
        GW_SparseMatrix r(this->GetDim(), ElOrder);
        GW_SparseMatrix::Transpose( *this, r );
        return r;
    }
    static void  Transpose(GW_SparseMatrix& a, GW_SparseMatrix& r)
    {
        GW_SparseMatrix::Copy( r, *Transp_Q(&a.M_) );
    }

    void BuildTriDiag( GW_Float val_down, GW_Float val_up, GW_Float val_diag )
    {
        GW_U32 n = this->GetDim();
        this->SetRowSize(0,2);
        this->SetData(0,0, val_diag);
        this->SetData(0,1, val_up);
        for( GW_U32 i=1; i<n-1; ++i )
        {
            this->SetRowSize(i,3);
            this->SetData(i,i-1, val_down);
            this->SetData(i,i, val_diag);
            this->SetData(i,i+1, val_up);
        }
        this->SetRowSize(n-1,2);
        this->SetData(n-1,n-2, val_down);
        this->SetData(n-1,n-1, val_diag);
    }

    static void TestClass(std::ostream &s = cout)
    {
        TestClassHeader("GW_SparseMatrix", s);
        GW_U32 n = 10;

        s << "Creation test." << endl;
        GW_SparseMatrix SM(n);
        SM.BuildTriDiag( 3,3,4 );
        s << SM;

        s << "M1=M2 : Copy test." << endl;
        GW_SparseMatrix SM2(n);
        SM2 = SM;
        s << SM;

        s << "M^T : Transpose test." << endl;
        s << SM.Transpose();


        s << "M*[1] : Matrix/Vector multiplication test." << endl;
        GW_VectorND v(n);
        v.SetValue(1);
        s << SM*v << endl;

        n = 1000;
        GW_Float eps = 1e-10;
        GW_SparseMatrix SM3(n);
        SM3.BuildTriDiag( 3,3,4 );
        s << "Conjugate gradient with SSOR test, size " << n << endl;
        GW_VectorND x(n);
        GW_VectorND b(n);
        b.Randomize();
        GW_Float err = SM3.IterativeSolve( x, b, GW_SparseMatrix::IterativeSolver_CG, GW_SparseMatrix::Preconditioner_SSOR,
                eps, 5*n, 1.2, GW_True );
        s << "Error=" << err << "." << endl;


        GW_SparseMatrix SM4(n);
        SM4.BuildTriDiag( -3,3,4 );
        s << "Bi-conjugate gradient with SSOR test, size " << n << endl;
        b.Randomize();
        err = SM4.IterativeSolve( x, b, GW_SparseMatrix::IterativeSolver_BiCG, GW_SparseMatrix::Preconditioner_SSOR,
                eps, 5*n, 1.2, GW_True );
        s << "Error=" << err << "." << endl;
        TestClassFooter("GW_SparseMatrix", s);
    }

    void Print(std::ostream &s)
    {
        s << "Sparse matrix, size " << this->GetDim() << "x" << this->GetDim() << "." << endl;
        for( GW_U32 i=0; i<this->GetDim(); ++i )
        {
            s << "|";
            for( GW_U32 j=0; j<this->GetDim();++j )
            {
                if( Q_GetElOrder(&M_)==Rowws )
                    s << this->GetData(i,j);
                else
                    s << this->GetData(j,i);
                if( j!=this->GetDim()-1 )
                    s << " ";
            }
            s << "|" << endl;
        }
    }

    GW_Float IterativeSolve( GW_VectorND& x, GW_VectorND& b, T_IterativeSolverType Solver, T_PreconditionerType Preconditioner,
                         GW_Float eps = 1e-10, GW_U32 nMaxIter = 100, GW_Float Omega = 1.2, GW_Bool bGetError = GW_False )
    {
        LSP_Vector xv;
        LSP_Vector bv;
        size_t Dim = this->GetDim();

        /* temportary data */
        V_Constr( &xv, "xv", Dim, Normal, True );
        GW_SparseMatrix::Copy( xv, x );
        V_Constr( &bv, "bv", Dim, Normal, True );
        GW_SparseMatrix::Copy( bv, b );

        /* set the preconditioner */
        PrecondProcType PrecondProc = NULL;
        switch( Preconditioner ) {
        case Preconditioner_SSOR:
            PrecondProc = SSORPrecond;
            break;
        case Preconditioner_Jacobi:
            PrecondProc = JacobiPrecond;
            break;
        case Preconditioner_ILU:
            PrecondProc = ILUPrecond;
            break;
        }

        SetRTCAccuracy( eps );
        V_SetAllCmp( &xv, 0 );    // initial guess
        switch(Solver) {
        case IterativeSolver_Jacobi:
            JacobiIter(&M_, &xv, &bv, nMaxIter, PrecondProc, Omega );
            break;
        case IterativeSolver_SORForw:
            SORForwIter( &M_, &xv, &bv, nMaxIter, PrecondProc, Omega );
            break;
        case IterativeSolver_SORBackw:
            SORBackwIter( &M_, &xv, &bv, nMaxIter, PrecondProc, Omega );
            break;
        case IterativeSolver_SSOR:
            SSORIter( &M_, &xv, &bv, nMaxIter, PrecondProc, Omega );
            break;
        case IterativeSolver_Chebyshev:
            ChebyshevIter( &M_, &xv, &bv, nMaxIter, PrecondProc, Omega );
            break;
        case IterativeSolver_CG:
            CGIter( &M_, &xv, &bv, nMaxIter, PrecondProc, Omega );
            break;
        case IterativeSolver_CGN:
            CGNIter( &M_, &xv, &bv, nMaxIter, PrecondProc, Omega );
            break;
        case IterativeSolver_GMRES:
            GMRESIter( &M_, &xv, &bv, nMaxIter, PrecondProc, Omega );
            break;
        case IterativeSolver_BiCG:
            BiCGIter( &M_, &xv, &bv, nMaxIter, PrecondProc, Omega );
            break;
        case IterativeSolver_QMR:
            QMRIter( &M_, &xv, &bv, nMaxIter, PrecondProc, Omega );
            break;
        case IterativeSolver_CGS:
            CGSIter( &M_, &xv, &bv, nMaxIter, PrecondProc, Omega );
            break;
        case IterativeSolver_BiCGSTAB:
            BiCGSTABIter( &M_, &xv, &bv, nMaxIter, PrecondProc, Omega );
            break;
        default:
            GW_ASSERT( GW_False );
        }

        GW_SparseMatrix::Copy( x, xv );

        if( bGetError )
        {
            /* r = b - A * x */
            return (b-(*this)*x).Norm2();
        }
        else
            return 0;

        /* destroy data */
        V_Destr(&xv);
        V_Destr(&bv);
    }

    /** Internal helper */
    static void Copy( GW_VectorND& v1, LSP_Vector& v2 )
    {
        GW_ASSERT( V_GetDim(&v2)==v1.GetDim() );
        for( GW_U32 i=0; i<v1.GetDim(); ++i )
            v1.SetData( i, V_GetCmp( &v2, i+1 ) );
    }
    static void Copy( LSP_Vector& v1, GW_VectorND& v2 )
    {
        GW_ASSERT( V_GetDim(&v1)==v2.GetDim() );
        for( GW_U32 i=0; i<v2.GetDim(); ++i )
            V_SetCmp( &v1, i+1, v2.GetData(i) );
    }
    static void Copy( GW_SparseMatrix& m1, LSP_Matrix& m2 )
    {
        GW_ASSERT( m1.GetDim() == Q_GetDim(&m2) );
        for( GW_U32 i=1; i<=Q_GetDim(&m2); ++i )
        {
            size_t l = Q_GetLen(&m2, i);
            Q_SetLen( &(m1.M_), i, l );
            for( GW_U32 j=0; j<l; ++j )
            {
                GW_U32 pos = (GW_U32) Q_GetPos( &m2, i, j );
                Q_SetEntry( &m1.M_, i, j, pos, Q_GetVal(&m2, i,j) );
            }
        }
    }

// private:

    LSP_Matrix M_;
    GW_U32* RowPosition_;

};

inline
std::ostream& operator<<(std::ostream &s, GW_SparseMatrix &A)
{
    A.Print(s);

    return s;
}

} // End namespace GW


#endif // _GW_SPARSEMATRIX_H_


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
