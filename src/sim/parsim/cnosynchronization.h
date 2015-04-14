//=========================================================================
//  CNOSYNCHRONIZATION.H - part of
//
//                  OMNeT++/OMNEST
//           Discrete System Simulation in C++
//
//  Author: Andras Varga, 2003
//          Dept. of Electrical and Computer Systems Engineering,
//          Monash University, Melbourne, Australia
//
//=========================================================================

/*--------------------------------------------------------------*
  Copyright (C) 2003-2015 Andras Varga
  Copyright (C) 2006-2015 OpenSim Ltd.

  This file is distributed WITHOUT ANY WARRANTY. See the file
  `license' for details on this and other legal matters.
*--------------------------------------------------------------*/

#ifndef __OMNETPP_NOSYNCHRONIZATION_H
#define __OMNETPP_NOSYNCHRONIZATION_H

#include "cparsimprotocolbase.h"

NAMESPACE_BEGIN

class cCommBuffer;

/**
 * "Dummy" implementation -- just pass messages between partitions,
 * without any synchronization. Of course incausalities may occur
 * which terminate the simulation with error, so this class is
 * only useful as a base "template" for implementing "real" synchronization
 * protocols.
 *
 * @ingroup Parsim
 */
class SIM_API cNoSynchronization : public cParsimProtocolBase
{
  public:
    /**
     * Constructor.
     */
    cNoSynchronization();

    /**
     * Destructor.
     */
    virtual ~cNoSynchronization();

    /**
     * Called at the beginning of a simulation run.
     */
    virtual void startRun();

    /**
     * Called at the end of a simulation run.
     */
    virtual void endRun();

    /**
     * Scheduler function. It processes whatever comes from other partitions,
     * then returns msgQueue.peekFirst(). Performs no synchronization
     * with other partitions, so incausalities may occur which stop the
     * simulation with an error (see also class comment).
     */
    virtual cEvent *takeNextEvent();

    /**
     * Undo takeNextEvent() -- it comes from the cScheduler interface.
     */
    virtual void putBackEvent(cEvent *event);
};

NAMESPACE_END


#endif

