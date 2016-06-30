#ifndef spMV_HPP
#define spMV_HPP

#include "SellCSigma.hpp"
#include "CSRMatrix.hpp"

#ifdef USE_LIKWID
extern "C"
{
#include <likwid.h>
}
#endif


/*****CSR_MATRIX**************************************************************/
/**
 * sparse Matrix-Vector multiplication
 * y=A*x
 * using the CSR Format
 * y and x musst be allocated and valid
 */
template<bool PLUSy=false>
void spMV( CSR_Matrix const & A,
           double const *x,
           double *y
         )
{
    double const *val  = A.getValues();
    int const *colInd  = A.getColInd();
    int const *rowPtr  = A.getRowPtr();
    int const numRows  = A.getRows();
    int const nonZeros = A.getNonZeros();

#ifdef USE_LIKWID
    LIKWID_MARKER_THREADINIT;
    LIKWID_MARKER_START("SpMV_CSR");
#endif

    // loop over all rows
#ifdef _OPENMP
    #pragma omp parallel for schedule(runtime)
#endif
    for (int rowID=0; rowID<numRows; ++rowID)
    {
        double tmp = 0.;

        // loop over all elements in row
        for (int rowEntry=rowPtr[rowID]; rowEntry<rowPtr[rowID+1]; ++rowEntry)
        {
            tmp += val[rowEntry] * x[ colInd[rowEntry] ];
        }

        y[rowID] = tmp;
    }

#ifdef USE_LIKWID
    LIKWID_MARKER_STOP("SpMV_CSR");
#endif
}



/*****SELL-C-SIGMA************************************************************/
/**
 * sparse Matrix-Vector multiplication
 * y=A*x
 * using the Sell-C-Sigma Format
 * y and x musst be allocated and valid
 *
 * x must be permutaed!
 * y will be permutaed!
 */
template< int C>
void spMV( SellCSigma_Matrix<C> const & A,
           double const * x,
           double * y
         )
{
    double const * val       = A.getValues();
    int const * chunkPtr     = A.getChankPtr();
    int const * chunkLength  = A.getChankLength();
    int const * colInd       = A.getColInd();
    int const numRows        = A.getRows();
    int const nonZeros       = A.getNonZeros();
    int const numberOfChunks = A.getNumberOfChunks();
    int const chunkSize      = C;

#ifdef USE_LIKWID
    LIKWID_MARKER_THREADINIT;
    LIKWID_MARKER_START("SpMV_Sell-C-sigma");
#endif

#ifdef _OPENMP
    #pragma omp parallel for schedule(runtime)
#endif
    // loop over all chunks
    for (int chunk=0; chunk < numberOfChunks; ++chunk)
    {
        int chunkOffset = chunkPtr[chunk];
        double tmp[chunkSize] {};

        // loop over all row elements in chunk
        for (int rowEntry=0; rowEntry<chunkLength[chunk]; ++rowEntry)
        {
            // (auto) vectorised loop over all rows in chunk
            #pragma simd
            for (int cRow=0; cRow<chunkSize; ++cRow)
            {
                tmp[cRow] += val      [chunkOffset + rowEntry*chunkSize + cRow]
                           * x[ colInd[chunkOffset + rowEntry*chunkSize + cRow] ];
            }
        }
        
        // write back result of y = alpha Ax + beta y
        // TODO zweite abbruch bedingung verhindert vectoresierung
        //      entwerder muss y groß genug sein ode rirgend eine coole andere idee
        for (int cRow=0,           rowID=chunk*chunkSize;
                 cRow<chunkSize && rowID<numRows;
               ++cRow,           ++rowID
            )
        {
            y[rowID] = tmp[cRow];
        }
    }

#ifdef USE_LIKWID
        LIKWID_MARKER_STOP("SpMV_Sell-C-sigma");
#endif
}


#endif
