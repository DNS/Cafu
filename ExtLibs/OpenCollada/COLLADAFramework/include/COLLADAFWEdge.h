/*
    Copyright (c) 2008-2009 NetAllied Systems GmbH

    This file is part of COLLADAFramework.

    Licensed under the MIT Open Source License, 
    for details please see LICENSE file or the website
    http://www.opensource.org/licenses/mit-license.php
*/

#ifndef __COLLADAFW_EDGE_H__
#define __COLLADAFW_EDGE_H__

#include "COLLADAFWPrerequisites.h"
#include "COLLADABUPlatform.h"
#include "COLLADABUhash_map.h"

#if !defined (COLLADABU_HAVE_TR1_UNORDERED_MAP) && !defined (COLLADABU_HAVE_STD_UNORDERED_MAP)
#ifdef COLLADABU_OS_LINUX
#	include <ext/hash_fun.h>
#elif (defined COLLADABU_OS_MAC)
#	include <ext/hash_fun.h>
#endif
#endif
#include <assert.h>
#include <climits>

namespace COLLADAFW
{

    /*
    * An edge contains two int values, which contains the vertex indices of the points, 
    * which descripe the edge.
    */
    class Edge
    {
    private:

        /** The vertex indices of the points, which descripe the edge. */
        unsigned int mVertexIndices[2];

        /** Flag, if the edge is stored in reverse direction. */
        bool mReverse;

    public:

        /** Constructor. */
        Edge () : mReverse (false)
        {
            mVertexIndices[0] = UINT_MAX;
            mVertexIndices[1] = UINT_MAX;
        }

        /** Constructor. */
        Edge ( int index1, int index2 ) : mReverse (false)
        {
            if ( index1 < index2 )
            {
                mVertexIndices[0] = index1;
                mVertexIndices[1] = index2;
            }
            else
            {
                mVertexIndices[0] = index2;
                mVertexIndices[1] = index1;
                mReverse = true;
            }
        }

        /** Destructor. */
        ~Edge () {}

        /** Set the edge vertex indices sorted. */
        void setVertexIndices ( int index1, int index2 ) 
        {
            if ( index1 < index2 )
            {
                mVertexIndices[0] = index1;
                mVertexIndices[1] = index2;
                mReverse = false;
            }
            else
            {
                mVertexIndices[0] = index2;
                mVertexIndices[1] = index1;
                mReverse = true;
            }
        }

        /** Flag, if the edge is stored in reverse direction. */
        const bool& isReverse () const { return mReverse; }

        unsigned int operator[](size_t i) const
        {
            assert( i < 2 );
            return mVertexIndices [i];
        }

        operator size_t () const
        {
            size_t retVal = 2166136261U;
            retVal = 16777619U * retVal ^ (size_t)mVertexIndices[0];
            retVal = 16777619U * retVal ^ (size_t)mVertexIndices[1]; 
            return retVal;
        }

        bool operator==(const Edge& e2) const
        {
            if ( mVertexIndices[0] != e2.mVertexIndices[0] ) return false;
            if ( mVertexIndices[1] != e2.mVertexIndices[1] ) return false;

            return true;
        }

        bool operator<(const Edge& e2) const
        {
            if ( mVertexIndices[0] > e2.mVertexIndices[0] ) return false;
            if ( mVertexIndices[0] == e2.mVertexIndices[0] ) 
            {
                if ( mVertexIndices[1] >= e2.mVertexIndices[1] ) return false;
            }
            return true;
        }

    };

} // namespace COLLADAFW

#if defined (COLLADABU_HAVE_STD_UNORDERED_MAP) || defined (COLLADABU_HAVE_TR1_UNORDERED_MAP) || defined(COLLADABU_OS_LINUX) || defined(COLLADABU_OS_MAC)
#if defined (COLLADABU_HAVE_STD_UNORDERED_MAP) || defined (COLLADABU_HAVE_TR1_UNORDERED_MAP)
namespace std
#if defined (COLLADABU_HAVE_TR1_UNORDERED_MAP)
{ namespace tr1
#endif
#else
namespace __gnu_cxx
#endif
{
    template<>
    struct hash<COLLADAFW::Edge>
    {
        size_t operator()(const COLLADAFW::Edge& edge) const { return edge; }
    };
#if defined (COLLADABU_HAVE_TR1_UNORDERED_MAP)
   }
#endif
} // namespace __gnu_cxx
#endif

#endif // __COLLADAFW_EDGE_H__
