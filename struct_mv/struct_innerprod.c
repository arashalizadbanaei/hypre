/*BHEADER**********************************************************************
 * (c) 1997   The Regents of the University of California
 *
 * See the file COPYRIGHT_and_DISCLAIMER for a complete copyright
 * notice, contact person, and disclaimer.
 *
 * $Revision$
 *********************************************************************EHEADER*/
/******************************************************************************
 *
 * Structured inner product routine
 *
 *****************************************************************************/

#include "headers.h"
#ifdef HYPRE_USE_PTHREADS
#include "box_pthreads.h"
#endif

/*--------------------------------------------------------------------------
 * hypre_StructInnerProd
 *--------------------------------------------------------------------------*/

#ifdef HYPRE_USE_PTHREADS
double           global_result[hypre_MAX_THREADS];
#endif

double           final_innerprod_result;


double
hypre_StructInnerProd(  hypre_StructVector *x,
                        hypre_StructVector *y )
{
   double           local_result;
                   
   hypre_Box       *x_data_box;
   hypre_Box       *y_data_box;
                   
   int              xi;
   int              yi;
                   
   double          *xp;
   double          *yp;
                   
   hypre_BoxArray  *boxes;
   hypre_Box       *box;
   hypre_Index      loop_size;
   hypre_IndexRef   start;
   hypre_Index      unit_stride;
                   
   int              i;
   int              loopi, loopj, loopk;
#ifdef HYPRE_USE_PTHREADS
   int              threadid = hypre_GetThreadID();
#endif

   local_result = 0.0;

#ifdef HYPRE_USE_PTHREADS
   global_result[threadid] = 0.0;
#endif

   hypre_SetIndex(unit_stride, 1, 1, 1);

   boxes = hypre_StructGridBoxes(hypre_StructVectorGrid(y));
   hypre_ForBoxI(i, boxes)
      {
         box   = hypre_BoxArrayBox(boxes, i);
         start = hypre_BoxIMin(box);

         x_data_box = hypre_BoxArrayBox(hypre_StructVectorDataSpace(x), i);
         y_data_box = hypre_BoxArrayBox(hypre_StructVectorDataSpace(y), i);

         xp = hypre_StructVectorBoxData(x, i);
         yp = hypre_StructVectorBoxData(y, i);

         hypre_GetBoxSize(box, loop_size);

#ifdef HYPRE_USE_PTHREADS
         hypre_BoxLoop2(loopi, loopj, loopk, loop_size,
                        x_data_box, start, unit_stride, xi,
                        y_data_box, start, unit_stride, yi,
                        {
                           global_result[threadid] += xp[xi] * yp[yi];
                        });
#else
         hypre_BoxLoop2(loopi, loopj, loopk, loop_size,
                        x_data_box, start, unit_stride, xi,
                        y_data_box, start, unit_stride, yi,
                        {
                           local_result += xp[xi] * yp[yi];
                        });
#endif
      }

#ifdef HYPRE_USE_PTHREADS
   if (threadid == 0)
   {
      for (i = 0; i < hypre_NumThreads; i++)
         local_result += global_result[i];
   }
   else if (threadid == hypre_NumThreads)
      local_result = global_result[threadid];
#endif


   MPI_Allreduce(&local_result, &final_innerprod_result, 1,
                 MPI_DOUBLE, MPI_SUM, hypre_StructVectorComm(x));


#ifdef HYPRE_USE_PTHREADS
   if (threadid == 0 || threadid == hypre_NumThreads)
#endif
   hypre_IncFLOPCount(2*hypre_StructVectorGlobalSize(x));

   return final_innerprod_result;
}
