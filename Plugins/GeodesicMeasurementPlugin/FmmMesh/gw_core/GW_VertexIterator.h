
/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_VertexIterator.h
 *  \brief  Definition of class \c GW_VertexIterator
 *  \author Gabriel Peyré
 *  \date   4-2-2003
 */
/*------------------------------------------------------------------------------*/

#ifndef _GW_VERTEXITERATOR_H_
#define _GW_VERTEXITERATOR_H_

#include "GW_Config.h"

namespace GW {

class GW_Face;
class GW_Vertex;

/*------------------------------------------------------------------------------*/
/**
 *  \class  GW_VertexIterator
 *  \brief  An iterator on the vertex around a given vertex.
 *  \author Gabriel Peyré
 *  \date   4-2-2003
 *
 *  To iterate on vertex.
 */
/*------------------------------------------------------------------------------*/

class FMMMESH_EXPORT GW_VertexIterator
{

public:

    GW_VertexIterator(  GW_Face* pFace, GW_Vertex* pOrigin, GW_Vertex* pDirection, GW_Face* pPrevFace, GW_U32 nNbrIncrement = 0 );

    /* assignment */
    //GW_VertexIterator& operator=( const GW_VertexIterator& it);

    /* evaluation */
    GW_Bool operator==( const GW_VertexIterator& it);
    GW_Bool operator!=( const GW_VertexIterator& it);

    /* indirection */
    GW_Vertex* operator*(  );

    /* progression */
    void operator++();

    GW_Face* GetLeftFace();
    GW_Face* GetRightFace();

    GW_Vertex* GetLeftVertex();
    GW_Vertex* GetRightVertex();

private:

    GW_Face* pFace_;
    GW_Vertex* pOrigin_;
    GW_Vertex* pDirection_;
    /* this is needed only for saving time on border edge */
    GW_Face* pPrevFace_;

    /** just for debug purpose */
    GW_U32 nNbrIncrement_;

};



/* assignment */
/*
inline GW_VertexIterator& GW_VertexIterator::operator=( const GW_VertexIterator& it)
{
    this->pFace_        = it.pFace_;
    this->pOrigin_        = it.pOrigin_;
    this->pDirection_    = it.pDirection_;
    this->pPrevFace_    = it.pPrevFace_;
    this->nNbrIncrement_= it.nNbrIncrement_;
    return *this;
}*/


/* egality */
inline GW_Bool GW_VertexIterator::operator==( const GW_VertexIterator& it)
{
    return (pFace_==it.pFace_)
        && (pOrigin_==it.pOrigin_)
        && (pDirection_==it.pDirection_)
        && (pPrevFace_==it.pPrevFace_);
}

/* egality */
inline GW_Bool GW_VertexIterator::operator!=( const GW_VertexIterator& it)
{
    return (pFace_!=it.pFace_)
        || (pOrigin_!=it.pOrigin_)
        || (pDirection_!=it.pDirection_)
        || (pPrevFace_!=it.pPrevFace_);
}




/* egality */
inline GW_Vertex* GW_VertexIterator::operator*(  )
{
    return pDirection_;
}



} // End namespace GW


#endif // _GW_VERTEXITERATOR_H_


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
