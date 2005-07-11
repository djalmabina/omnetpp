//==========================================================================
//  DATASORTER.H - part of
//
//                     OMNeT++/OMNEST
//            Discrete System Simulation in C++
//
//==========================================================================

/*--------------------------------------------------------------*
  Copyright (C) 1992-2005 Andras Varga

  This file is distributed WITHOUT ANY WARRANTY. See the file
  `license' for details on this and other legal matters.
*--------------------------------------------------------------*/

#ifndef _DATASORTER_H_
#define _DATASORTER_H_


#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "scalarmanager.h"
#include "filtering.h"
#include "util.h"


typedef std::vector<int> IntVector;
typedef std::vector<IntVector> IntVectorVector;



/**
 * Helps organize the data in a ScalarManager into bar charts,
 * scatter plots, etc.
 */
class DataSorter
{
  private:
    ScalarManager *scalarMgr;
    static ScalarManager *tmpScalarMgr;

    typedef bool (*GroupingFunc)(const ScalarManager::Datum&, const ScalarManager::Datum&);
    typedef bool (*CompareFunc)(int id1, int id2);

  private:
    // comparison functions for doGrouping() and sortAndAlign()
    static bool sameGroupFileRunScalar(const ScalarManager::Datum& d1, const ScalarManager::Datum& d2);
    static bool sameGroupModuleScalar(const ScalarManager::Datum& d1, const ScalarManager::Datum& d2);
    static bool sameGroupFileRunModule(const ScalarManager::Datum& d1, const ScalarManager::Datum& d2);
    static bool lessByModuleRef(int id1, int id2);
    static bool equalByModuleRef(int id1, int id2);
    static bool lessByFileAndRun(int id1, int id2);
    static bool equalByFileAndRun(int id1, int id2);
    static bool lessByScalarNameRef(int id1, int id2);
    static bool equalByScalarNameRef(int id1, int id2);
    static bool lessByValue(int id1, int id2);

    /**
     * Form groups (IntVectors) by the grouping function passed
     * (sameGroupFileRunScalar, sameGroupModuleScalar or sameGroupFileRunModule).
     */
    IntVectorVector doGrouping(const IntVector& idlist, GroupingFunc sameGroup);

    /**
     * Sort every group (IntVectors) in place by the sorting function given
     * (lessByModuleRef,lessByFileAndRun, lessByScalarNameRef, lessByValue),
     * and aligns.
     *
     * Then inserts "null" elements (id=-1) so that every group is of same length,
     * and same indices are "equal", by the eqaual function passed
     * (equalByModuleRef,equalByFileAndRun, equalByScalarNameRef).
     */
    void sortAndAlign(IntVectorVector& vv, CompareFunc less, CompareFunc equal);

  public:
    /**
     * Constructor.
     */
    DataSorter(ScalarManager *sm) {scalarMgr = sm;}

    /**
     * Form groups (IntVectors) by runRef+scalarName.
     * Then order each group by module name, and insert "null" elements (id=-1)
     * so that every group is of same length, and the same indices contain
     * the same moduleNameRefs.
     */
    IntVectorVector groupByRunAndName(const IntVector& idlist);

    /**
     * Form groups (IntVectors) by moduleName+scalarName.
     * Order each group by runRef, and insert "null" elements (id=-1) so that
     * every group is of same length, and the same indices contain the same runRef.
     */
    IntVectorVector groupByModuleAndName(const IntVector& idlist);

    /**
     *
     */
    IntVectorVector prepareScatterPlot(const IntVector& idlist, const char *moduleName, const char *scalarName);

    /**
     * Looks at the data given by their Id, and returns a subset of them
     * where each (moduleName, scalarName) pair occurs only once.
     *
     * Purpose: these pairs should offered as selection of X axis for a
     * scatter plot.
     */
    IntVector getModuleAndNamePairs(const IntVector& idList, int maxcount);

    /**
     *
     */
    IntVectorVector prepareCopyToClipboard(const IntVector& idlist);
};

#endif


