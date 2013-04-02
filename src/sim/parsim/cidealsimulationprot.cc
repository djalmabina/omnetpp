//=========================================================================
//  CIDEALSIMULATIONPROT.CC - part of
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
  Copyright (C) 2003-2008 Andras Varga
  Copyright (C) 2006-2008 OpenSim Ltd.

  This file is distributed WITHOUT ANY WARRANTY. See the file
  `license' for details on this and other legal matters.
*--------------------------------------------------------------*/


#include "cmessage.h"
#include "cmodule.h"
#include "cgate.h"
#include "cenvir.h"
#include "cconfiguration.h"
#include "cidealsimulationprot.h"
#include "cplaceholdermod.h"
#include "cparsimpartition.h"
#include "cparsimcomm.h"
#include "ccommbuffer.h"
#include "messagetags.h"
#include "globals.h"
#include "cconfigoption.h"
#include "regmacros.h"

USING_NAMESPACE


Register_Class(cIdealSimulationProtocol);

// load 100,000 values from eventlog at once (~800K allocated memory)
#define TABLESIZE   "100000"

Register_GlobalConfigOption(CFGID_PARSIM_IDEALSIMULATIONPROTOCOL_TABLESIZE, "parsim-idealsimulationprotocol-tablesize", CFG_INT, TABLESIZE, "When cIdealSimulationProtocol is selected as parsim synchronization class: specifies the memory buffer size for reading the ISP event trace file.");
extern cConfigOption *CFGID_PARSIM_DEBUG; // registered in cparsimpartition.cc


cIdealSimulationProtocol::cIdealSimulationProtocol() : cParsimProtocolBase()
{
    fin = NULL;
    debug = ev.getConfig()->getAsBool(CFGID_PARSIM_DEBUG);

    tableSize = ev.getConfig()->getAsInt(CFGID_PARSIM_IDEALSIMULATIONPROTOCOL_TABLESIZE);
    table = new ExternalEvent[tableSize];
    numItems = 0;
    nextPos = 0;
}

cIdealSimulationProtocol::~cIdealSimulationProtocol()
{
    delete [] table;
}

void cIdealSimulationProtocol::startRun()
{
    char fname[200];
    sprintf(fname, "ispeventlog-%d.dat", comm->getProcId());
    fin = fopen(fname,"rb");
    if (!fin)
        throw cRuntimeError("cIdealSimulationProtocol error: cannot open file `%s' for read", fname);

    readNextRecordedEvent();
}

void cIdealSimulationProtocol::endRun()
{
    fclose(fin);
}

void cIdealSimulationProtocol::processReceivedMessage(cMessage *msg, int destModuleId, int destGateId, int sourceProcId)
{
    msg->setSchedulingPriority(sourceProcId);
    cParsimProtocolBase::processReceivedMessage(msg, destModuleId, destGateId, sourceProcId);
}

cEvent *cIdealSimulationProtocol::takeNextEvent()
{
    // if no more local events, wait for something to come from other partitions
    while (sim->msgQueue.isEmpty())
        if (!receiveBlocking())
            return NULL;

    cEvent *event = sim->msgQueue.peekFirst();
    simtime_t eventTime = event->getArrivalTime();

    // if we aren't at the next external even yet --> nothing special to do
    if (eventTime < nextExternalEvent.t)
    {
        ASSERT(event->getSrcProcId()==-1); // must be local message
        return event;
    }

    // if we reached the next external event in the log file, do it
    if (event->getSrcProcId()==nextExternalEvent.srcProcId && eventTime==nextExternalEvent.t)
    {
        if (debug) EV << "expected external event (srcProcId=" << event->getSrcProcId()
                      << " t=" << nextExternalEvent.t << ") has already arrived, good!\n";
        readNextRecordedEvent();
        event->setSchedulingPriority(0);

        // remove event from FES and return it
        cEvent *tmp = sim->msgQueue.removeFirst();
        ASSERT(tmp==event);
        return event;
    }

    // next external event not yet here, we have to wait until we've received it
    ASSERT(eventTime > nextExternalEvent.t);
    if (debug)
    {
        EV << "next local event at " << eventTime << " is PAST the next external event "
              "expected for t=" << nextExternalEvent.t << " -- waiting...\n";
    }

    do
    {
        if (!receiveBlocking())
            return NULL;
        event = sim->msgQueue.peekFirst();
        eventTime = event->getArrivalTime();
    }
    while (event->getSrcProcId()!=nextExternalEvent.srcProcId || eventTime > nextExternalEvent.t);

    if (eventTime < nextExternalEvent.t)
    {
        throw cRuntimeError("cIdealSimulationProtocol: event trace does not match actual events: "
                            "expected event with timestamp %s from proc=%d, and got one with timestamp %s",
                            SIMTIME_STR(nextExternalEvent.t), nextExternalEvent.srcProcId, SIMTIME_STR(eventTime));
    }

    // we have the next external event we wanted, return it
    ASSERT(eventTime==nextExternalEvent.t);
    readNextRecordedEvent();
    event->setSchedulingPriority(0);

    // remove event from FES and return it
    cEvent *tmp = sim->msgQueue.removeFirst();
    ASSERT(tmp==event);
    return event;
}

void cIdealSimulationProtocol::readNextRecordedEvent()
{
    // if table is empty, load new values into it
    if (nextPos==numItems)
    {
        nextPos = 0;
        numItems = fread(table, sizeof(ExternalEvent), tableSize, fin);
        if (numItems==0)
            throw cTerminationException("cIdealSimulationProtocol: end of event trace file");
    }

    // get next entry from table
    nextExternalEvent = table[nextPos++];

    if (debug) EV << "next expected external event: srcProcId=" << nextExternalEvent.srcProcId
                  << " t=" << nextExternalEvent.t << "\n";
}

void cIdealSimulationProtocol::putBackEvent(cEvent *event)
{
    throw cRuntimeError("cIdealSimulationProtocol: \"Run Until Event/Module\" functionality "
                        "cannot be used with this scheduler (putBackEvent() not implemented)");
}

