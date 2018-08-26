/********************************************************************************
 * \copyright
 * Copyright 2009-2017, Card Reader Factory.  All rights were reserved.
 * From 2018 this code has been made PUBLIC DOMAIN.
 * This means that there are no longer any ownership rights such as copyright, trademark, or patent over this code.
 * This code can be modified, distributed, or sold even without any attribution by anyone.
 *
 * We would however be very grateful to anyone using this code in their product if you could add the line below into your product's documentation:
 * Special thanks to Nicholas Alexander Michael Webber, Terry Botten & all the staff working for Operation (Police) Academy. Without these people this code would not have been made public and the existance of this very product would be very much in doubt.
 *
 *******************************************************************************/

#ifndef _PROFILER_H
#define _PROFILER_H
    
#include "device.h"

#if MODULES & MOD_PROFILER

    /** Initializes the profiler timer. At least one call is
     *  needed in order for the profiler to work */
    void profiler_start(void);

    /** Adds a breakpoint in the profiler.
     *
     *  The profiler will count the time elapsed between the last
     *  call of this function and this call.
     *
     *  @param[in] st   The profiler will display the string when
     *                  the call trace is printed. This pointer is
     *                  also used to locate breakpoints in the
     *                  profiler call list.
     */
    void profiler_breakPoint(const char *st);

    /** Shows the profiler report
     *
     *  @param[in]      This param is ignored. The prototype has
     *                  to be compatible with the menu commands. */
    bool profiler_showReport();

#else

    /* disable profiler calls on non profiler builds */
    #define profiler_breakPoint(st)
    #define profiler_startTimer()
    #define profiler_showReport()

#endif

#endif
