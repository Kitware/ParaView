
/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_MatrixNxP.h
 *  \brief  Definition of class \c GW_MatrixNxP
 *  \author Gabriel Peyré
 *  \date   5-31-2003
 */
/*------------------------------------------------------------------------------*/
#error matrixnp
#ifndef _GW_MATRIXNXP_H_
#define _GW_MATRIXNXP_H_

#include "GW_MathsConfig.h"
#include "GW_VectorND.h"
#include "GW_MatrixStatic.h"


#include "tnt/tnt.h"
#include "tnt/jama_eig.h"
#include "tnt/jama_qr.h"
#include "tnt/jama_cholesky.h"
#include "tnt/jama_svd.h"
#include "tnt/jama_lu.h"



namespace GW {

typedef TNT::Array2D<GW_Float> GW_TNTArray2D;

/*------------------------------------------------------------------------------*/
/**
 *  \class  GW_MatrixNxP
 *  \brief  A matrix of arbitrary size.
 *  \author Gabriel Peyré
 *  \date   5-31-2003
 *
 *  Use \b TNT library.
 */
/*------------------------------------------------------------------------------*/

class GW_MatrixNxP:        public GW_TNTArray2D
{

public:

    GW_MatrixNxP()                                        :GW_TNTArray2D(){}
    GW_MatrixNxP(GW_U32 m, GW_U32 n)                        :GW_TNTArray2D((int) m,(int) n){}
    GW_MatrixNxP(GW_U32 m, GW_U32 n,  GW_Float *a)            :GW_TNTArray2D((int) m,(int) n,a){}
    GW_MatrixNxP(GW_U32 m, GW_U32 n, const GW_Float &a)    :GW_TNTArray2D((int) m,(int) n,a){}
    GW_MatrixNxP(const GW_MatrixNxP &A)                    :GW_TNTArray2D(A){}

    void Reset( GW_U32 i, GW_U32 j )
    {
        ((GW_TNTArray2D&) (*this)) = GW_TNTArray2D((int) i, (int) j);
    }

    GW_U32 GetNbrRows() const    { return this->dim1(); }
    GW_U32 GetNbrCols() const    { return this->dim2(); }

    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixNxP::GetData
    *
    *  \param  i row number
    *  \param  j col number
    *  \return value of the (i,j) data.
    *  \author Gabriel Peyré 2001-09-19
    */
    /*------------------------------------------------------------------------------*/
    GW_Float GW_MatrixNxP::GetData(GW_U32 i, GW_U32 j) const
    {
        GW_ASSERT( i<this->GetNbrRows() && j<this->GetNbrCols() );
        return (*this)[i][j];
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixNxP::SetData
    *
    *  \param  i row number
    *  \param  j col number
    *  \param  rVal value
    *  \author Gabriel Peyré 2001-09-19
    */
    /*------------------------------------------------------------------------------*/
    void GW_MatrixNxP::SetData(GW_U32 i, GW_U32 j, GW_Float rVal)
    {
        GW_ASSERT( i<this->GetNbrRows() && j<this->GetNbrCols() );
        (*this)[i][j] = rVal;
    }



    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixNxP::operator *
    *
    *  \param  m right hand side
    *  \return multiplication this*m
    *  \author Gabriel Peyré 2001-09-19
    */
    /*------------------------------------------------------------------------------*/
    GW_MatrixNxP GW_MatrixNxP::operator*(const GW_MatrixNxP& m)
    {
        GW_ASSERT( this->GetNbrCols() == m.GetNbrRows() );
        GW_MatrixNxP Res( this->GetNbrRows(), m.GetNbrCols() );
        GW_MatrixNxP::Multiply( *this, m, Res );
        return Res;
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixNxP::Multiply
    *
    *  \param  a right side
    *  \param  b left side
    *  \param  r result
    *  \author Gabriel Peyré 2001-09-19
    */
    /*------------------------------------------------------------------------------*/
    static void GW_MatrixNxP::Multiply(const GW_MatrixNxP& a, const GW_MatrixNxP& b, GW_MatrixNxP& r)
    {
        GW_ASSERT( a.GetNbrCols() == b.GetNbrRows() );
        GW_ASSERT( r.GetNbrRows() == a.GetNbrRows() );
        GW_ASSERT( r.GetNbrCols() == b.GetNbrCols() );

        GW_Float rVal;

        for( GW_U32 i=0; i<r.GetNbrRows(); ++i )
            for( GW_U32 j=0; j<r.GetNbrCols(); ++j )
            {
                rVal = 0;
                for( GW_U32 k=0; k<a.GetNbrCols(); ++k )
                    rVal += a.GetData(i,k) * b.GetData(k,j);
                r.SetData( i,j, rVal );
            }
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixNxP::*=
    *
    *  \param  m right side
    *  \author Gabriel Peyré 2001-09-19
    */
    /*------------------------------------------------------------------------------*/
    void  GW_MatrixNxP::operator*=(const GW_MatrixNxP & m)
    {
        GW_MatrixNxP Tmp( this->GetNbrRows(), this->GetNbrCols() );
        GW_MatrixNxP::Multiply( *this, m, Tmp );
        (*this) = Tmp;
    }

    /*------------------------------------------------------------------------------*/
    // Name : GW_MatrixNxP::GW_MatrixNxP operator*
    /**
    *  \param  s [GW_Float] scalar.
    *  \author Gabriel Peyré
    *  \date   6-1-2003
    *
    *  Matrix times scalar operator.
    */
    /*------------------------------------------------------------------------------*/
    GW_MatrixNxP GW_MatrixNxP::operator*(GW_Float s)
    {
        GW_MatrixNxP m( this->GetNbrRows(), this->GetNbrCols() );
        GW_MatrixNxP::Multiply(*this, s, m);
        return m;
    }
    /*------------------------------------------------------------------------------*/
    // Name : GW_MatrixNxP::Multiply
    /**
    *  \param  a [GW_MatrixNxP&] The matrix.
    *  \param  s [GW_Float] The scalar.
    *  \param  r [GW_MatrixNxP&] The result.
    *  \author Gabriel Peyré
    *  \date   6-1-2003
    *
    *  Matrix times scalar.
    */
    /*------------------------------------------------------------------------------*/
    static void GW_MatrixNxP::Multiply(const GW_MatrixNxP& a, const GW_Float s, GW_MatrixNxP& r)
    {
        GW_ASSERT( a.GetNbrCols()==r.GetNbrCols() &&  r.GetNbrRows()==r.GetNbrRows() );

        for( GW_U32 i=0; i<a.GetNbrRows(); ++i )
        for( GW_U32 j=0; j<a.GetNbrCols(); ++j )
            r.SetData(i,j, a.GetData(i,j)*s );
    }
    /*------------------------------------------------------------------------------*/
    // Name : GW_MatrixNxP::
    /**
    *  \param  s [GW_Float] scalar.
    *  \return [void operator *] DESCRIPTION
    *  \author Gabriel Peyré
    *  \date   6-1-2003
    *
    *  Auto multiply.
    */
    /*------------------------------------------------------------------------------*/
    void GW_MatrixNxP::operator *= (GW_Float s)
    {
        GW_MatrixNxP::Multiply(*this, s, *this);
    }


    /*------------------------------------------------------------------------------*/
    // Name : GW_MatrixNxP::GW_MatrixNxP operator/
    /**
    *  \param  s [GW_Float] scalar.
    *  \author Gabriel Peyré
    *  \date   6-1-2003
    *
    *  Matrix divided by scalar operator.
    */
    /*------------------------------------------------------------------------------*/
    GW_MatrixNxP GW_MatrixNxP::operator/(GW_Float s)
    {
        GW_MatrixNxP m( this->GetNbrRows(), this->GetNbrCols() );
        GW_MatrixNxP::Divide(*this, s, m);
        return m;
    }
    /*------------------------------------------------------------------------------*/
    // Name : GW_MatrixNxP::Divide
    /**
    *  \param  a [GW_MatrixNxP&] The matrix.
    *  \param  s [GW_Float] The scalar.
    *  \param  r [GW_MatrixNxP&] The result.
    *  \author Gabriel Peyré
    *  \date   6-1-2003
    *
    *  Matrix divided by scalar.
    */
    /*------------------------------------------------------------------------------*/
    static void GW_MatrixNxP::Divide(const GW_MatrixNxP& a, const GW_Float s, GW_MatrixNxP& r)
    {
        if( s==0 )
            return;
        GW_ASSERT( a.GetNbrCols()==r.GetNbrCols() &&  r.GetNbrRows()==r.GetNbrRows() );

        for( GW_U32 i=0; i<a.GetNbrRows(); ++i )
        for( GW_U32 j=0; j<a.GetNbrCols(); ++j )
            r.SetData(i,j, a.GetData(i,j)/s );
    }
    /*------------------------------------------------------------------------------*/
    // Name : GW_MatrixNxP::/=
    /**
    *  \param  s [GW_Float] scalar.
    *  \return [void operator *] DESCRIPTION
    *  \author Gabriel Peyré
    *  \date   6-1-2003
    *
    *  Auto divide.
    */
    /*------------------------------------------------------------------------------*/
    void GW_MatrixNxP::operator /= (GW_Float s)
    {
        GW_MatrixNxP::Divide(*this, s, *this);
    }

    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixNxP::operator
    *
    *  \param  v right hand statement.
    *  \return the vector multiplied by the matrix.
    *  \author Gabriel Peyré 2001-09-30
    */
    /*------------------------------------------------------------------------------*/
    GW_VectorND GW_MatrixNxP::operator*(const GW_VectorND& v)
    {
        GW_VectorND Res( this->GetNbrRows() );
        GW_MatrixNxP::Multiply( *this, v, Res );
        return Res;
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixNxP::Multiply
    *
    *  \param  a left hand statement.
    *  \param  v Right hand statement.
    *  \param  r Result.
    *  \author Gabriel Peyré 2001-09-30
    *
    *    Multiply the vector by the matrix.
    */
    /*------------------------------------------------------------------------------*/
    static void GW_MatrixNxP::Multiply(const GW_MatrixNxP& a, const GW_VectorND& v, GW_VectorND& r)
    {
        GW_ASSERT( a.GetNbrRows() == r.GetDim() );
        GW_ASSERT( a.GetNbrCols() == v.GetDim() );

        GW_Float rVal;

        for( GW_U32 i=0; i<r.GetDim(); ++i )
        {
            rVal = 0;
            for( GW_U32 j=0; j<v.GetDim(); ++j )
                rVal += a.GetData(i,j) * v.GetData(j);

            r.SetData( i, rVal );
        }
    }


    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixNxP::Transpose
    *
    *  \return Transposed matrix.
    *  \author Gabriel Peyré 2001-09-19
    */
    /*------------------------------------------------------------------------------*/
    GW_MatrixNxP GW_MatrixNxP::Transpose()
    {
        GW_MatrixNxP Res( this->GetNbrCols(), this->GetNbrRows() );
        GW_MatrixNxP::Transpose( *this, Res );

        return Res;
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixNxP::Transpose
    *
    *  \param  a Right side
    *  \param  r Result
    *  \return Transposed matrix.
    *  \author Gabriel Peyré 2001-09-19
    */
    /*------------------------------------------------------------------------------*/
    void GW_MatrixNxP::Transpose(const GW_MatrixNxP& a, GW_MatrixNxP& r)
    {
        GW_ASSERT( a.GetNbrCols()==r.GetNbrRows() &&  r.GetNbrCols()==r.GetNbrRows() );

        for( GW_U32 i=0; i<a.GetNbrRows(); ++i )
        for( GW_U32 j=0; j<a.GetNbrCols(); ++j )
        {
            r.SetData(i,j, a.GetData(j,i) );
        }
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixNxP::operator +
    *
    *  \param  m Right side
    *  \return this+a
    *  \author Gabriel Peyré 2001-09-19
    */
    /*------------------------------------------------------------------------------*/
    GW_MatrixNxP  GW_MatrixNxP::operator+(const GW_MatrixNxP & m)
    {
        GW_MatrixNxP Res( this->GetNbrRows(), this->GetNbrCols() );
        GW_MatrixNxP::Add( *this, m, Res );
        return Res;
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixNxP::Add
    *
    *  \param  a left hand side
    *  \param  b right hand side
    *  \param  r result = a+b
    *  \author Gabriel Peyré 2001-09-19
    */
    /*------------------------------------------------------------------------------*/
    void GW_MatrixNxP::Add(const GW_MatrixNxP& a, const GW_MatrixNxP& b, GW_MatrixNxP& r)
    {
        GW_ASSERT( a.GetNbrCols()==b.GetNbrCols() &&  a.GetNbrRows()==b.GetNbrRows() );
        GW_ASSERT( a.GetNbrCols()==r.GetNbrCols() &&  a.GetNbrRows()==r.GetNbrRows() );

        for( GW_U32 i=0; i<a.GetNbrRows(); ++i )
            for( GW_U32 j=0; j<a.GetNbrCols(); ++j )
                r.SetData(i,j, a.GetData(i,j)+b.GetData(i,j) );
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixNxP::operator -
    *
    *  \param  m Right side
    *  \return this-a
    *  \author Gabriel Peyré 2001-09-19
    */
    /*------------------------------------------------------------------------------*/
    GW_MatrixNxP  GW_MatrixNxP::operator-(const GW_MatrixNxP & m)
    {
        GW_MatrixNxP Res( this->GetNbrRows(), this->GetNbrCols() );
        GW_MatrixNxP::Minus( *this, m, Res );
        return Res;
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixNxP::Minus
    *
    *  \param  a left hand side
    *  \param  b right hand side
    *  \param  r result = a-b
    *  \author Gabriel Peyré 2001-09-19
    */
    /*------------------------------------------------------------------------------*/
    void GW_MatrixNxP::Minus(const GW_MatrixNxP& a, const GW_MatrixNxP& b, GW_MatrixNxP& r)
    {
        GW_ASSERT( a.GetNbrCols()==b.GetNbrCols() &&  a.GetNbrRows()==b.GetNbrRows() );
        GW_ASSERT( a.GetNbrCols()==r.GetNbrCols() &&  a.GetNbrRows()==r.GetNbrRows() );

        for( GW_U32 i=0; i<a.GetNbrRows(); ++i )
            for( GW_U32 j=0; j<a.GetNbrCols(); ++j )
                r.SetData(i,j, a.GetData(i,j)-b.GetData(i,j) );
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixNxP::operator -
    *
    *  \return -this
    *  \author Gabriel Peyré 2001-09-19
    */
    /*------------------------------------------------------------------------------*/
    GW_MatrixNxP  GW_MatrixNxP::operator-()
    {
        GW_MatrixNxP Res( this->GetNbrRows(), this->GetNbrCols() );
        GW_MatrixNxP::UMinus( *this, Res );
        return Res;
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixNxP::UMinus
    *
    *  \param  a right side
    *  \param  r result
    *  \author Gabriel Peyré 2001-09-19
    *
    *    unary minus.
    */
    /*------------------------------------------------------------------------------*/
    void GW_MatrixNxP::UMinus(const GW_MatrixNxP& a, GW_MatrixNxP& r)
    {
        GW_ASSERT( a.GetNbrCols()==r.GetNbrCols() &&  a.GetNbrRows()==r.GetNbrRows() );

        for( GW_U32 i=0; i<a.GetNbrRows(); ++i )
            for( GW_U32 j=0; j<a.GetNbrCols(); ++j )
                r.SetData(i,j, -a.GetData(i,j) );
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixNxP::+=
    *
    *  \param  m right side
    *  \author Gabriel Peyré 2001-09-19
    *
    *    unary minus.
    */
    /*------------------------------------------------------------------------------*/
    void  GW_MatrixNxP::operator+=(const GW_MatrixNxP & m)
    {
        GW_MatrixNxP Tmp( this->GetNbrRows(), this->GetNbrCols() );
        GW_MatrixNxP::Add( *this, m, Tmp );
        (*this) = Tmp;
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixNxP::-=(const
    *
    *  \param  m right side
    *  \author Gabriel Peyré 2001-09-19
    */
    /*------------------------------------------------------------------------------*/
    void  GW_MatrixNxP::operator-=(const GW_MatrixNxP & m)
    {
        GW_MatrixNxP Tmp( this->GetNbrRows(), this->GetNbrCols() );
        GW_MatrixNxP::Minus( *this, m, Tmp );
        (*this) = Tmp;
    }


    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixNxP::SetZero
    *
    *  \return set all component of the matrix to zero.
    *  \author Gabriel Peyré 2001-09-19
    */
    /*------------------------------------------------------------------------------*/
    void GW_MatrixNxP::SetZero()
    {
        this->SetValue(0);
    }


    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixNxP::SetValue
    *
    *  \param  rVal value to set.
    *  \author Gabriel Peyré 2001-09-19
    */
    /*------------------------------------------------------------------------------*/
    void GW_MatrixNxP::SetValue(GW_Float rVal)
    {
        GW_ASSERT( this->GetNbrCols()>0 && this-GetNbrRows()>0 );
        for( GW_U32 i=0; i<this->GetNbrRows(); ++i )
        for( GW_U32 j=0; j<this->GetNbrCols(); ++j )
            this->SetData( i, j, rVal );
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixNxP::Randomize
    *
    *  \param  rMin minimum value
    *  \param  rMax maximum value
    *  \author Gabriel Peyré 2001-09-19
    */
    /*------------------------------------------------------------------------------*/
    void GW_MatrixNxP::Randomize(GW_Float rMin = 0, GW_Float rMax = 1)
    {
        GW_ASSERT( this->GetNbrRows()>0 && this->GetNbrCols()>0 );
        for( GW_U32 i=0; i<this->GetNbrRows(); ++i )
            for( GW_U32 j=0; j<this->GetNbrCols(); ++j )
                this->SetData( i, j, rMin + GW_RAND*(rMax-rMin) );
    }



    /*------------------------------------------------------------------------------*/
    // Name : GW_MatrixNxP::LU
    /**
    *  \param  L [GW_MatrixNxP&] The \c L matrix.
    *  \param  U [GW_MatrixNxP&] The \c U matrix.
    *  \param  P [GW_VectorND&] Permutations of the columns.
    *  \return [GW_Float] Determinant.
    *  \author Gabriel Peyré
    *  \date   5-31-2003
    *
    *  Perform an LU decomposition of the matrix.
    */
    /*------------------------------------------------------------------------------*/
    void GW_MatrixNxP::LU( GW_MatrixNxP& L, GW_MatrixNxP& U, GW_VectorND& P )
    {
        JAMA::LU<GW_Float> lu( *this );
        U = (GW_MatrixNxP&) lu.getU();
        L = (GW_MatrixNxP&) lu.getL();
        P = (GW_VectorND&) lu.getPivot();
    }
    /*------------------------------------------------------------------------------*/
    // Name : GW_MatrixNxP::LUSolve
    /**
    *  \param  x [GW_VectorND&] Solution.
    *  \param  b [GW_VectorND&] RHS
    *  \return [GW_Bool] Is the system inversible ?
    *  \author Gabriel Peyré
    *  \date   5-31-2003
    *
    *  Solve a linear system Ax=b with LU decomposition.
    */
    /*------------------------------------------------------------------------------*/
    GW_Bool GW_MatrixNxP::LUSolve( GW_VectorND& x, const GW_VectorND& b )
    {
        JAMA::LU<GW_Float> lu( *this );
        if( lu.isNonsingular() )
        {
            x = (GW_VectorND&)  lu.solve(b);
            return x.dim()!=0;
        }
        else
            return GW_False;
    }

    /*------------------------------------------------------------------------------*/
    // Name : GW_MatrixNxP::Cholesky
    /**
    *  \param  L [GW_MatrixNxP&] The result, lower triangular matrix.
    *  \return [GW_Bool] \c true if the decomposition was a success, \c false if the matrix is not
    *    symmetric definite positive.
    *  \author Gabriel Peyré
    *  \date   5-31-2003
    *
    *  Perform the Cholesky decomposition of the matrix M =AA^*
    */
    /*------------------------------------------------------------------------------*/
    GW_Bool GW_MatrixNxP::Cholesky( GW_MatrixNxP& L )
    {
        JAMA::Cholesky<GW_Float> chol( *this );

        L = (GW_MatrixNxP&) chol.getL();

        return chol.is_spd()==1;
    }
    /*------------------------------------------------------------------------------*/
    // Name : GW_MatrixNxP::CholeskySolve
    /**
    *  \param  x [GW_VectorND&] Solution.
    *  \param  b [GW_VectorND&] RHS.
    *  \return [GW_Bool] Was the inversion a success ?
    *  \author Gabriel Peyré
    *  \date   5-31-2003
    *
    *  Solve using Cholesky.
    */
    /*------------------------------------------------------------------------------*/
    GW_Bool GW_MatrixNxP::CholeskySolve( GW_VectorND& x, const GW_VectorND& b )
    {
        JAMA::Cholesky<GW_Float> chol( *this );
        if( chol.is_spd() )
        {
            x = (GW_VectorND&) chol.solve( b );
            return x.dim()!=0;
        }
        else
            return GW_False;
    }

    /*------------------------------------------------------------------------------*/
    // Name : GW_MatrixNxP::QR
    /**
    *  \param  Q [GW_MatrixNxP&] The Q matrix.
    *  \param  R [GW_MatrixNxP&] The R matrix.
    *  \author Gabriel Peyré
    *  \date   5-31-2003
    *
    *  Perform QR decomposition.
    */
    /*------------------------------------------------------------------------------*/
    void GW_MatrixNxP::QR( GW_MatrixNxP& Q, GW_MatrixNxP& R )
    {
        JAMA::QR<GW_Float> qr( *this );
        Q = (GW_MatrixNxP&) qr.getQ();
        R = (GW_MatrixNxP&) qr.getR();
    }
    /*------------------------------------------------------------------------------*/
    // Name : GW_MatrixNxP::QRSolve
    /**
    *  \param  x [GW_VectorND&] solution.
    *  \param  b [GW_VectorND&] RHS.
    *  \return [GW_Bool] Was the process successful ?
    *  \author Gabriel Peyré
    *  \date   5-31-2003
    *
    *  Solve a linear system using QR decomposition.
    */
    /*------------------------------------------------------------------------------*/
    GW_Bool GW_MatrixNxP::QRSolve( GW_VectorND& x, const GW_VectorND& b )
    {
        JAMA::QR<GW_Float> qr( *this );
        if( !qr.isFullRank() )
            return GW_False;
        else
        {
            x = (GW_VectorND&) qr.solve(b);
            return x.dim()!=0;
        }
    }

    /*------------------------------------------------------------------------------*/
    // Name : GW_MatrixNxP::Eigenvalue
    /**
    *  \param  V [GW_MatrixNxP&] The change of basis matrix.
    *  \param  pD [GW_MatrixNxP*] A block diagonal matrix (diagonal if eigenvalues are real).
    *  \param  pRealEig [GW_VectorND*] The real part of the eigenvalues.
    *  \param  pImagEig [GW_VectorND*] The imaginary part of the eigenvalues.
    *  \author Gabriel Peyré
    *  \date   5-31-2003
    *
    *  Perform eigen-decomposition A = V D V^*.
    */
    /*------------------------------------------------------------------------------*/
    void GW_MatrixNxP::Eigenvalue( GW_MatrixNxP& V, GW_MatrixNxP* pD, GW_VectorND* pRealEig, GW_VectorND* pImagEig )
    {
        JAMA::Eigenvalue<GW_Float> eig( *this );
        eig.getV(V);
        if( pD!=NULL )
            eig.getD(*pD);
        if( pRealEig!=NULL )
            eig.getRealEigenvalues(*pRealEig);
        if( pImagEig!=NULL )
            eig.getImagEigenvalues(*pImagEig);
    }
    /*------------------------------------------------------------------------------*/
    // Name : GW_MatrixNxP::SVD
    /**
    *  \param  U [GW_MatrixNxP&] U orthogonal matrix
    *  \param  V [GW_MatrixNxP&] the right singular vectors.
    *  \param  pSingV [GW_VectorND*] The singular values.
    *  \param  pS [GW_MatrixNxP*] Diagonal matrix of singular values.
    *  \author Gabriel Peyré
    *  \date   5-31-2003
    *
    *  Compute SVD decomposition, ie A
    */
    /*------------------------------------------------------------------------------*/
    void GW_MatrixNxP::SVD( GW_MatrixNxP& U, GW_MatrixNxP& V, GW_VectorND* pSingV, GW_MatrixNxP* pS )
    {
        JAMA::SVD<GW_Float> svd( *this );
        svd.getU( U );
        svd.getV( V );
        if( pSingV!=NULL )
            svd.getSingularValues( *pSingV );
        if( pS!=NULL )
            svd.getS( *pS );
    }


    /*------------------------------------------------------------------------------*/
    // Name : GW_MatrixNxP::TestClass
    /**
    *  \author Gabriel Peyré
    *  \date   6-1-2003
    *
    *  Test the class.
    */
    /*------------------------------------------------------------------------------*/
    static void GW_MatrixNxP::TestClass(std::ostream &s = cout)
    {
        TestClassHeader("GW_MatrixNxP", s);
        const GW_U32 n = 50;

        GW_MatrixNxP m(n,n);
        m.Randomize();

        GW_VectorND v(n), x(n), b(n);
        v.Randomize();
        b.Randomize();

        m.LUSolve( x, b );
        GW_Float err = ( m*x-b ).Norm2();
        GW_ASSERT( err<GW_EPSILON );

        /* turn m into a symmetric matrix */
        m = m*m.Transpose();
        GW_MatrixNxP Vmat(n,n), Dmat(n,n);
        GW_VectorND RealEig, ImagEig;
        m.Eigenvalue( Vmat,&Dmat, &RealEig, &ImagEig );
        GW_Float dotp = GW_VectorND(n,Vmat[0])*GW_VectorND(n,Vmat[1]);
        GW_ASSERT( GW_ABS(dotp)<GW_EPSILON );
        dotp = GW_VectorND(n,Vmat[0])*GW_VectorND(n,Vmat[1]);
        GW_ASSERT( GW_ABS(dotp)<GW_EPSILON );
        dotp = GW_VectorND(n,Vmat[1])*GW_VectorND(n,Vmat[2]);
        GW_ASSERT( GW_ABS(dotp)<GW_EPSILON );
        dotp = GW_VectorND(n,Vmat[2])*GW_VectorND(n,Vmat[3]);
        GW_ASSERT( GW_ABS(dotp)<GW_EPSILON );
        TestClassFooter("GW_MatrixNxP", s);
    }


private:

};


inline
std::ostream& operator<<(std::ostream &s, GW_MatrixNxP& m)
{
    s << "GW_MatrixNxP : " << endl;
    for( GW_U32 i=0; i<m.GetNbrRows(); ++i )
    {
        cout << "|";
        for( GW_U32 j=0; j<m.GetNbrCols(); ++j )
        {
            s << m.GetData(i,j);
            if( j!=m.GetNbrCols()-1 )
                s << " ";
        }
        s << "|";
        if( i!=m.GetNbrRows()-1 )
            s << endl;
    }
    return s;
}



} // End namespace GW


#endif // _GW_MATRIXNXP_H_


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
