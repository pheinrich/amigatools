 /*************************************************************************
 *
 *  DxfToSdf
 *  Copyright (c) 1993,2009  Peter Heinrich
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Boston, MA  02110-1301, USA.
 *
 *************************************************************************
 *  This module has routines which deal with polygon edges.  They'll be
 *  called when we try to combine polygons, split polygons, etc.
 *************************************************************************/



#include <stdlib.h>
#include <string.h>

#include <exec/types.h>

#include "dxftosdf.h"
#include "edge.h"
#include "poly.h"



/*
**  Global variables
*/

static Edge *oldEdges = NULL, *edgeList = NULL;



/*
**  Functions
*/

static Edge *new_edge( Edge **prev, WORD polyNum, WORD start, WORD end )
{
   Edge *edge;

   edge = calloc( 1, sizeof( Edge ) );
   if( edge )
   {
      edge->poly  = polyNum;
      edge->start = start;
      edge->end   = end;

      edge->next = *prev;
      *prev = edge;
   }
   else
      error( "Out of memory" );

   return( edge );
}



static Edge *add_edges( Edge **prev, Edge **list, Edge *start )
{
   Edge *node, *next;

   if( start )
      node = start->next ? start->next : *list;

   while( node )
   {
      next = node->next;
      node->next = *prev;
      *prev = node;

      if( node == start )
         break;
      node = next ? next : *list;
   }

   *list = NULL;
   return( next );
}



static Edge *delete_edge( Edge **list, Edge *edge )
{
   Edge *node = *list, **prev = list;

   while( node )
   {
      if( node == edge )
      {
         *prev = edge->next;
         free( edge );
         break;
      }

      prev = &node->next;
      node = *prev;
   }

   return( *prev );
}



static Edge *transfer_edge( Edge **fromList, Edge **toList, Edge *edge )
{
   Edge *node = *fromList, **prev = fromList;

   while( node )
   {
      if( node == edge )
      {
         *prev = edge->next;
         edge->next = *toList;
         *toList = edge;
         break;
      }

      prev = &node->next;
      node = *prev;
   }

   return( NULL );
}



static void free_edges( void )
{
   Edge *edge;

   edge = oldEdges;
   while( edge )
   {
      oldEdges = edge->next;
      free( edge );

      edge = oldEdges;
   }

   edge = edgeList;
   while( edge )
   {
      edgeList = edge->next;
      free( edge );

      edge = edgeList;
   }
}



static void reorient( Edge *edge, const WORD *orientation )
{
   while( edge )
   {
      if( orientation[ edge->poly ] )
      {
         edge->start ^= edge->end;
         edge->end ^= edge->start;
         edge->start ^= edge->end;
      }

      edge = edge->next;
   }
}



void combine_edges( Polygon *dest, const Layer *layer, const WORD *index, UWORD num )
{
   Edge *edges = NULL, *edge, *path[ MAX_PATHS ];
   UWORD i, numPath, which;
   WORD *orientation;
   BOOL match;

   free_edges();
   orientation = calloc( setup.maxPolys, sizeof( WORD ) );
   if( NOT orientation )
      error( "Out of memory" );

   for( i = 0; i < num; i++ )
   {
      const Polygon *poly;
      UWORD j;

      poly = layer->polygons + index[ i ];
      for( j = 0; j < poly->numPoints - 1; j++ )
         new_edge( &edges, i, poly->index[ j ], poly->index[ j + 1 ] );
      new_edge( &edges, i, poly->index[ j ], *poly->index );
   }

   edge = edges;
   while( edge )
   {
      Edge *edge2 = edge->next;

      match = FALSE;
      while( edge2 )
      {
         if( edge->start == edge2->start && edge->end == edge2->end )
         {
            match = TRUE;
            orientation[ edge2->poly ] = NOT orientation[ edge->poly ];
         }
         else if( edge->start == edge2->end && edge->end == edge2->start )
         {
            match = TRUE;
            orientation[ edge2->poly ] = orientation[ edge->poly ];
         }

         edge2 = match ? transfer_edge( &edges, &oldEdges, edge2 ) : edge2->next;
      }

      edge = match ? delete_edge( &edges, edge ) : edge->next;
   }

   reorient( edges, orientation );
   reorient( oldEdges, orientation );
   free( orientation );

   for( numPath = MAX_PATHS; numPath > 0; numPath-- )
      path[ numPath - 1 ] = NULL;

   while( edges )
   {
      edge = path[ numPath ];
      if( edge )
      {
         Edge *next = edges, *edge2 = NULL;

         while( next )
         {
            if( edge->end == next->start )
            {
               if( edge2 )
                  error( "Non-isolated hole encountered" );

               edge2 = next;
            }

            next = next->next;
         }

         if( NOT edge2 )
         {
            if( MAX_PATHS <= ++numPath )
               error( "Maximum of %hu holes exceeded", MAX_PATHS );
         }
         else
            transfer_edge( &edges, &path[ numPath ], edge2 );
      }
      else
         transfer_edge( &edges, &path[ numPath ], edges );
   }

   add_edges( &edgeList, &path[ 0 ], path[ 0 ] );
   for( i = 0, which = 1; i < numPath; )
   {
      Edge *last = path[ which ];

      while( last )
      {
         Edge *edge2 = oldEdges;

         while( edge2 )
         {
            if( last->start == edge2->end )
            {
               Edge *end = edgeList;

               while( end )
               {
                  if( edge2->start == end->end )
                  {
                     Edge *dest;

                     dest = new_edge( &end->next, 0, end->end, edge2->end );
                     dest = add_edges( &dest->next, &path[ which ], last );
                     new_edge( &dest->next, 0, edge2->end, end->end );

                     i++;
                     break;
                  }

                  end = end->next;
               }

               if( end )
                  break;
            }

            edge2 = edge2->next;
         }

         if( edge2 )
            break;
         else
            last = last->next;
      }

      if( numPath == which++ )
         which = 1;
   }

   dest->numPoints = 0;
   edge = edgeList;
   while( edge )
   {
      if( MAX_POLY_POINTS < dest->numPoints )
         error( "Too many points in polygon" );

      dest->index[ dest->numPoints++ ] = edge->start;
      edge = edge->next;
   }
}
