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

#include <stdlib.h>
#include "project.h"
#include "common.h"
#include "debug.h"
#include "xprintf.h"
#include "device.h"
#include "profiler.h"

/* ADC SAR sequencer component header to access Vref value */
#include <ADC_SAR.h>

//#define PEAK_TEST_SAME_LINE  // shows the peak results on the same line
    
#define THRESHOLD_MV 		(30)
#define MAX_PEAKS 			(705)
#define TRACK_TIMEOUT 		(3000)
#define MIN_POSITIVE_PEAK 	(100)
#define MIN_NEGATIVE_PEAK 	(-100)
#define MV_PEAK_MIN 		(100)
#define MV_PEAK_MAX 		(200)    

#define VAR_WIDTH (sizeof(uint32_t)*8)
/* Get actual Vref. value from ADC SAR sequencer */
#define ADC_VREF_VALUE_V    ((float)ADC_SAR_DEFAULT_VREF_MV_VALUE/1000.0)


enum
{
    Ch_0 = 0,
    Ch_Temperature
};

typedef struct
{
    bool     positive:1;
    uint8_t  ts:6;
    int16_t  v:9;
} peak_t;

typedef struct
{
    int16_t peak_idx;
    uint32_t lp_tu;
    peak_t peaks[MAX_PEAKS];
} track_t;
    


volatile uint32 dataReady = 0;
uint32_t        dietemp_ts                          = 0;
uint32_t        adcStartTs                          = 0;
uint32_t        cnt_injections                      = 0;
uint32_t        cnt_free                            = 0;
uint32_t        comp_ts                             = 0;
int16_t         threshold_units                     = 0;
volatile bool   comparatorDataReady                 = false;
volatile int16  result[ADC_SAR_TOTAL_CHANNELS_NUM];
track_t         trk;
uint32_t profiling_sample_ts = 0;
uint32_t profiling_sample_count = 0;

int print_index = 0;
int max_index = 0;


CY_ISR(LPComp_Interrupt) 
{     
    /* Interrupt does not clear automatically.      
    * It is user responsibility to do that.      */     
    LPComp_ClearInterrupt(LPComp_INTR);          
    
    /*     
    * Add user interrupt code to manage interrupt.     
    */ 
    comparatorDataReady = true;
//    comparatorData = result[Ch_0];
    comp_ts = GetTick() + 3000;
    
} 

void start_track(track_t *t)
{

    memset(t, 0, sizeof(track_t));
    t->peak_idx = -1;
    t->lp_tu = timerUnits();   
}

void save_peak(track_t *t, uint16_t ts, int16_t v, bool positive)
{
    assert(t->peak_idx >= 0);
    
    if (t->peak_idx < MAX_PEAKS)
    {
        t->lp_tu = timerUnits();
        peak_t *p = &t->peaks[t->peak_idx];
        p->ts = ts;
        p->v = v;        
        p->positive = positive;        
    }
}
/*
#define save_peak(_track, _peak, _ts, _value, _positive)    \
{                                                           \
    (_track)->lp_tu = timerUnits();                         \
    (_peak)->ts = _ts;                                      \
    (_peak)->v = _value;                                    \
    (_peak)->positive = _positive;                          \
}
*/
#if MODULES & MOD_PEAK_DETECT_UNIT_TEST == 0
void process_sample(track_t *t, int16_t v)
{
#else
void process_sample(track_t *t, uint16_t delay, int16_t v)
{
    if (delay == 0xffff)
#endif
    {
        int32_t diff = timerUnits() - t->lp_tu;
        if (diff > 0xffff)
            delay = 0xffff;
        else
            delay = diff;
    }
    
    peak_t *lp;

    if (t->peak_idx < 0)
    {
        lp = &t->peaks[0];
        if (v >= MIN_POSITIVE_PEAK)
        {                
            t->peak_idx++; // new peak
//            save_peak(t, lp, delay, v, true);
            save_peak(t, delay, v, true);
        }
        else if (v < MIN_NEGATIVE_PEAK)
        {
            t->peak_idx++; // new peak
            save_peak(t, delay, v, false);
//            save_peak(t, lp, delay, v, false);
        }
        return;
    }
            
    assert(t->peak_idx >= 0);
// 	if (t->peak_idx >= MAX_PEAKS)
//		return;
    
    lp = &t->peaks[t->peak_idx];
    
    if (v > lp->v) 
    {
        if (lp->positive) // increasing
        {
            delay += lp->ts; // as we are rewriting, we have to add the previous timestamp difference to the current one.
            save_peak(t, delay, v, true);// rewrite last peak with this one (don't increase t->peak_idx)
//            save_peak(t, lp, delay, v, true);// rewrite last peak with this one (don't increase t->peak_idx)
        }
        else if (v - threshold_units > lp->v) // just increased over a negative peak
        {
            t->peak_idx++; // last peak (negative) is ok            
//			if (t->peak_idx < MAX_PEAKS)
			{
            	save_peak(t, delay, v, true); // start working on a new peak (new peak is positive)
//				lp++;
//            	save_peak(t, lp, delay, v, true); // start working on a new peak (new peak is positive)
			}
        }
        else
        {
            //value dropped
        }
    }
    else 
    {
        if (!lp->positive) // decreasing
        {
            delay += lp->ts; // as we are rewriting, we have to add the previous timestamp difference to the current one.
            save_peak(t, delay, v, false);// rewrite last peak with this one (don't increase t->peak_idx)
//            save_peak(t, lp, delay, v, false);// rewrite last peak with this one (don't increase t->peak_idx)
        }
        else if (v + threshold_units < lp->v) // just decreased over a positive peak
        {
            t->peak_idx++; // last peak (positive) is ok            
//			if (t->peak_idx < MAX_PEAKS)
			{
				save_peak(t, delay, v, false); // start working on a new peak (new peak is negative)
//				lp++;
//				save_peak(t, lp, delay, v, false); // start working on a new peak (new peak is negative)
			}
        }
        else
        {
            //value dropped            
        }
    }
    
    
    
//    if ( !lp->positive && v < lp->v) // increasing
//    {
//        t->peak_idx++; // new peak
//        // don't increase t->peak_idx; rewrite last peak with this one
//        save_peak(t, delay, v, false);
//    }    
//    else if ( v < lp->v && lp->v < MIN_NEGATIVE_PEAK) // decreasing
//    {
//        save_peak(t, delay, v, false);
//    }
//    else if (
//                (v < lp->v - threshold_units && lp->v >= MIN_POSITIVE_PEAK) 
//                ||
//                (v > lp->v + threshold_units && lp->v < MIN_NEGATIVE_PEAK) 
//            )			
//    {
//        // previous peak is final. add a new peak
//        t->peak_idx++; // new peak
//        save_peak(t, delay, v);
//    }
//    else
//    {
//        // value ignored...
//    }  
}

CY_ISR(ADC_ISR_LOC)
{
    uint32 intr_status;
    uint32 range_status;

    /* Read interrupt status registers */
    intr_status = ADC_SAR_SAR_INTR_MASKED_REG;
    /* Check for End of Scan interrupt */
    if((intr_status & ADC_SAR_EOS_MASK) != 0u)
    {
        /* Read range detect status */
        range_status = ADC_SAR_SAR_RANGE_INTR_MASKED_REG;
        /* Verify that the conversion result met the condition Low_Limit <= Result < High_Limit  */
//        ADC_SAR_CountsTo_mVolts(Ch_Temperature, ADCCountsCorrected)
        int16_t r = ADC_SAR_GetResult16(Ch_0);
        process_sample(&trk, r, 0xffff);
        
        
//        if((range_status & (uint32)(1ul << Ch_0)) != 0u) 
//        {
//            
//            /* Read conversion result */
//            result[Ch_0] = ADC_SAR_GetResult16(Ch_0);
            // 1024 ... 128
            // mv ... ?
            
//        }    

        /* Clear range detect status */
        ADC_SAR_SAR_RANGE_INTR_REG = range_status;
        dataReady |= ADC_SAR_EOS_MASK;
        cnt_free++;
    }    

    /* Check for Injection End of Conversion */
    if((intr_status & ADC_SAR_INJ_EOC_MASK) != 0u)
    {
        result[Ch_Temperature] = ADC_SAR_GetResult16(Ch_Temperature);
        dataReady |= ADC_SAR_INJ_EOC_MASK;
        cnt_injections++;
    }    

    /* Clear handled interrupt */
    ADC_SAR_SAR_INTR_REG = intr_status;
}


void adc_start(void)
{
    /* Init and start sequencing SAR ADC */
    ADC_SAR_Start();
    /* Enable interrupt and set interrupt handler to local routine */
    ADC_SAR_IRQ_StartEx(ADC_ISR_LOC);    
    adcStartTs = GetTick();
    
    LPComp_ISR_StartEx(LPComp_Interrupt);     
    LPComp_Start(); 
    
    // 1000 units .... ADC_SAR_CountsTo_mVolts(Ch_0, 1000)
    // x          .... THRESHOLD_MV
    
    threshold_units = THRESHOLD_MV * 1000 / ADC_SAR_CountsTo_mVolts(Ch_0, 1000);
    vLog(xDebug, "threshold_units = %lu\n", threshold_units);
    vLog(xDebug, "sizeof(peak_t) = %u\n", sizeof(peak_t));
    vLog(xDebug, "1000 counts = %imV\n", ADC_SAR_CountsTo_mVolts(Ch_0, 1000));
    
    
}

void adc_info(void)
{
    uint32_t free = cnt_free;
    uint32_t elapsed = GetTick() - adcStartTs;
    uint32_t inj = cnt_injections;
    vLog(xInfo, "injections: %i/sec, free running: %i/sec\n", (inj+1)*1024/elapsed, free*1024/elapsed);    
}

void adc_main(void)
{
    
//    int16 res = 0;
    int16 ADCCountsCorrected;

    
    if (TimerExpired(dietemp_ts))
    {
        dietemp_ts = GetTick() + 1024;
        ADC_SAR_EnableInjection();
//        uint32_t elapsed = GetTick() - adcStartTs;
//        vLog(xDebug, "ADC_EnableInjection injections: %i/sec, free running: %i\n", cnt_injections*1024/elapsed, cnt_free*1024/elapsed);
    }
    
    /* When conversion of sequencing channels has completed */
    if((dataReady & ADC_SAR_EOS_MASK) != 0)
    {
        /* Get voltage, measured by ADC */
        dataReady &= ~ADC_SAR_EOS_MASK;
//        res = ADC_SAR_CountsTo_mVolts(Ch_0, result[Ch_0]);
        
//        vLog(xDebug, "conversion of sequencing channels has been completed. %i mV\n", res);
    }    
    
    /* When conversion of the injection channel has completed */
    if((dataReady & ADC_SAR_INJ_EOC_MASK) != 0)
    {
        dataReady &= ~ADC_SAR_INJ_EOC_MASK;

        /******************************************************************************
        * Adjust data from ADC with respect to Vref value.
        * This adjustment is to be done if Vref is set to any other than
        * internal 1.024V.
        * For more detailed description see Functional Description section
        * of DieTemp P4 datasheet.
        *******************************************************************************/
        ADCCountsCorrected = result[Ch_Temperature]*((int16)((float)ADC_VREF_VALUE_V/1.024));

        #if(ADC_SAR_DEFAULT_AVG_MODE == ADC_SAR__ACCUMULATE)
            stats.dieTemperature = DieTemp_CountsTo_Celsius(ADCCountsCorrected/ADC_SAR_DEFAULT_AVG_SAMPLES_DIV);
        #else                
            stats.dieTemperature = DieTemp_CountsTo_Celsius(ADCCountsCorrected);
        #endif /* ADC_SAR_DEFAULT_AVG_MODE == ADC_SAR__ACCUMULATE */
        

        /* Print temperature value to UART */
        vLog(xDebug, "Temperature value: %dC, adc voltage: %dmV adc: %08x r[]: %08x\n", stats.dieTemperature, ADC_SAR_CountsTo_mVolts(Ch_Temperature, ADCCountsCorrected), ADCCountsCorrected, result[Ch_Temperature]);
    }
    
//    static int c = 0;
//    static uint32_t cts = 0;
#if 0    
    if (comparatorDataReady)
    {
        comparatorDataReady = false;
//        LPComp_ClearInterrupt(LPComp_INTR);          
        LPComp_Stop(); 
        start_track(&trk);
        vLog(xDebug, "adc started, cnt_free: %i\n", cnt_free);
        ADC_SAR_StartConvert();
        cnt_free = 0;
        adcStartTs = GetTick();
    }
//        ADC_SAR_StartConvert();
//
//        cts = GetTick() + 250;
//        c++;
//        xprintf("%3i ", ADC_SAR_CountsTo_mVolts(Ch_0, comparatorData));
//        if (c%8 == 0)
//            xputc('\n');
//            
//        comparatorDataReady = false;
//    }
//    if (cts != 0 && TimerExpired(cts))
//    {
//        xputc('\n');
//        cts = 0;        
//    }
    
    if (trk.first_ts != 0 &&    
         (
            (trk.peak_idx < 1 && TimerExpired(trk.first_ts + TRACK_TIMEOUT)) ||
            (trk.peak_idx >= 1 && TimerExpired(trk.peaks[trk.peak_idx].ts + trk.first_ts + (trk.peaks[trk.peak_idx].ts - trk.peaks[trk.peak_idx-1].ts)*3))
         )
       )
    {
        ADC_SAR_StopConvert();
        LPComp_Start(); 
        comp_ts = 0;
#ifdef WORKING        
        adc_info();
        vLog(xDebug, "adc stopped, cnt_free: %i\n", cnt_free);
        for(int i = 0; i < trk.peak_idx; i++)
        {
            xprintf("@%3i: %4i ", trk.peaks[i].ts, ADC_SAR_CountsTo_mVolts(Ch_0, trk.peaks[i].value));
            if ((i+1) % 16 == 0)
                xputc('\n');
        }
#endif        
        start_track(&trk);
//        for(int i = 255; i >= 0; i--)
//        {
//            if (i == 127)
//            {
//                for(int j = 0; j < 112; j++)
//                    xputc('-');
//               xputc('\n');
//            }
//            xprintf("%5i ", adcstats[i]);
//            if (i % 16 == 0)
//                xputc('\n');            
//        }
        cnt_free = 0;
    }
#endif    
}

#if MODULES & MOD_PEAK_DETECT_UNIT_TEST

void print_separator(uint32_t flags, uint8_t count)
{
    if ((vLogFlags & (1<<flags)) == 0)
        return;
    
    for (int i = 0; i < count; i++)
        xputc('-');
        
    xputc('\n');
}

void print_binary(uint32_t *data, int len)
{
        
    int cnt0 = 0;
    int cnt1 = 0;
    for (uint16_t i = 0; i < len*VAR_WIDTH; i++)
    {
        int byte = i / VAR_WIDTH;
        int bit = i % VAR_WIDTH;
        
        if (data != NULL && (data[byte] & (1<<bit)) == 0)
        {
            xputc('0');
            cnt0++;
        }
        else
        {
            xputc('1');
            cnt1++;
        }
        xputc(' ');
    }
    if (data != NULL)
        xputc(' ');
    xprintf("%i ones, %i zeroes\n", cnt1, cnt0);
}
#ifdef TEST_DELAY    
    uint32_t ts_peak_delay = GetTick();
#endif    

void send_test_sample(uint16_t delay, int16_t value, char c)
{
    if ((vLogFlags & (1<<xPeakVrbs)) || ((vLogFlags & (1<<xPeakValue)) && ((print_index < 8) || (print_index >= max_index - 8))))
	{
	    if (print_index % 8 == 0)
	    {
	        if (print_index != 0)
	            xputc('\n');
	        xprintf("   %4i | ", print_index);
	    }
//            xprintf("%5i %3ims| ", values[i], delays[i]);
	    xprintf("%3ims%c%5i| ", delay, c, value);
	}    
    print_index++;        
       
#ifdef TEST_DELAY    
	while (!TimerExpired(ts_peak_delay+delay)) wdtReset();
	ts_peak_delay = GetTick();        
	process_sample(&trk, 0xffff, value);  
#else    
    uint32_t ts =  timerUnits();
	process_sample(&trk, delay, value);        
    profiling_sample_ts +=  timerUnits() - ts;
    profiling_sample_count++;
#endif        
}

int rn(int minimum, int maximum)
{
    int v;
    int m;
    
    if (minimum < 0)
        m = 0;
    else
        m = minimum;
    
    if (m < maximum)
        v = (m + rand()%(maximum-m));
    else
        v = 0;
    
//    vLog(xDebug, "min: %i, max: %i, random number:%i\n", minimum, maximum, v);

    return v;
}

void test_waveform(uint32_t *data, int8_t len, int16_t noise)
{
    
    int v;
    int b;
    int delay          = 50;
    int sign           = 1;
    int cnt1           = 0;
    int cnt0           = 0;
	int previous_peak  = MIN_POSITIVE_PEAK;
    int previous_value = 0;
	

    if (len == 0)
    {
        vLog(xErr, "Nothing to do\n");
        return;
    }
    if (len * 2* VAR_WIDTH >= MAX_PEAKS)
    {
        vLog(xErr, "insuficient memory for detecting %i peaks; %i is our limit\n", len*2*VAR_WIDTH, MAX_PEAKS);
        return;
    }
//    vLog(xDebug, "max peaks=%i\n", len * 2* VAR_WIDTH);
    
    for (uint16_t i = 0; i < len; i++)
        data[i] = rand();
        
    if (vLogFlags & (1<<xPeakBits))   
    {
        vLog(xPeakBits, "data      : "); print_binary(data, len);
        vLog(xPeakBits, "clocks    :  "); print_binary(NULL, len);    
        vLog(xPeakBits, "encoded   : ");
        for (uint16_t i = 0; i < len*VAR_WIDTH; i++)
        {
            int byte = i / VAR_WIDTH;
            int bit = i % VAR_WIDTH;
            if((data[byte] & (1<<bit)) == 0)
            {
                cnt0++;
            	xputc('0');
            }
            else 
            {
                cnt1++;
            	xputc('1');
            }
            
            cnt1++;
            xputc('1');        
        }
    
        xprintf(" %i ones, %i zeroes\n", cnt1, cnt0);
    }
    

//    if (vLogFlags & (1<<xPeakValue))
//    {
//        int bits = 0;
//        vLog(xPeakValue, "data: ");    
//        for(uint8_t i = 0; i < len; i++)
//        {
//            xprintf("%08x (%s) ", data[i], maptobin(data[i]));        
//            bits += count_bits(data[i]);
//        }
//        xprintf(" (%i ones, %i clocks, %i peaks)\n", bits, sizeof(data)*8, bits+sizeof(data)*8);
//    }
    
    cnt1           = 0;
    cnt0           = 0;    
    print_index    = 0;
#ifdef TEST_DELAY    
    ts_peak_delay = GetTick();
#endif    
    start_track(&trk);
    max_index = len*VAR_WIDTH*2*(noise+1);
    if (vLogFlags & (1<<xPeakValue|1<<xPeakVrbs))   
        xprintf("generated (max_index: %i):\n", max_index);
    int m = previous_value;
    bool chm = true;
    for (uint16_t i = 0; i < len*VAR_WIDTH; i++)    
    {        
        wdtReset();
		for (int k = 0; k < 2; k++)
		{
	
		    /* noise */
	        v = (rand() % 20) - 10;
	        delay += delay * v / 100;
		
            if (previous_value < 0)
                previous_value = -previous_value;
//            vLog(xDebug, "set previous_value to %i\n", previous_value);
            if (previous_peak < 0)
                previous_peak = -previous_peak;
            if (chm)
                m = previous_value;
            //xputc('\n');
	        for(int j = 0; j < noise; j++ )
	        {              
                v = rn(previous_peak-threshold_units, min(min(m,previous_value) + threshold_units, previous_peak));
//                vLog(xDebug, "+ v: %i, m:%i, previous_peak: %i, previous_value: %i, sign: %i\n", v, m, previous_peak, previous_value, sign);
                m = min(m, v);                    
                                        
//			    vLog(xDebug, "Sending:");
                send_test_sample(delay, sign*v, ' ');
                previous_value = v;
//                xputc('\n');
//                vLog(xDebug, "set previous_value to %i\n", previous_value);
//                xputc('\n');
//                vLog(xDebug, "> v: %i, m:%i, previous_peak: %i, previous_value: %i, sign: %i\n", v, m, previous_peak, previous_value, sign);
	        }
		
			/* data or clock */
			if (k==0)
			{
			    /* data */
//		        vLog(xDebug, "byte: %i, bit: %i\n", byte, bit);            

		        int byte = i / VAR_WIDTH;
		        int bit = i % VAR_WIDTH;
				
		        if((data[byte] & (1<<bit)) == 0)
					b = 0;
				else 
					b = 1;
			}
			else
			{
				/* clock */
				b = 1;
			}
			
			
			if (b == 0)
			{
                v = rn(previous_peak-threshold_units, min(min(m,previous_value) + threshold_units, previous_peak));
//	                v = rand() % MV_PEAK_MIN;
	            cnt0++;
			}
			else
			{
	            if (sign == 1)
	                sign = -1;
	            else
	                sign = 1;
                
	            v = MV_PEAK_MIN + 1 + rand() % (MV_PEAK_MAX - MV_PEAK_MIN);
	            cnt1++;		
			}
            previous_value = v * sign;
//            vLog(xDebug, "Sending:");
  			if (b == 1)
            {
                previous_peak = previous_value;
    		    send_test_sample(delay, previous_value, '*');
                chm = true;
            }
            else
            {
    		    send_test_sample(delay, previous_value, ' ');
                m = min(m, max(0, previous_value));
                chm = false;
            }
//            xputc('\n');
//            vLog(xDebug, "set previous value to %i\n", previous_value);
                
//          vLog(xDebug, "v[%i]: %i\n", i, previous_peak);
		}
    }
    if (vLogFlags & (1<<xPeakValue|1<<xPeakVrbs))   
    {
        xprintf("\b\b  \n");
        xprintf("detected: \n");
        for (uint16 i = 0; i <= trk.peak_idx; i++)
        {
            if ((vLogFlags & (1<<xPeakVrbs)) || ((vLogFlags & (1<<xPeakValue)) && ((i < 8) || (i/8 > (trk.peak_idx - 8)/8))))
            {
                if (i%8 == 0)
                {
                    if (i != 0)
                        xputc('\n');
                    xprintf("   %4i | ", i);
                }
                xprintf("%3ims %5i| ", trk.peaks[i].ts, trk.peaks[i].v);
            }
        }
        xprintf("\b\b  \n");
    }
        
    int cnt = 0;
    for (uint16_t i = 0; i < len; i++)
    {
        cnt += 32 + count_bits(data[i]);
    }
    if ((vLogFlags & (1<<xPeakValue|1<<xPeakBits|1<<xPeakVrbs)) != 0)
        xprintf("sent %i peaks, detected %i peaks => %s\n", cnt, trk.peak_idx+1, trk.peak_idx+1 == cnt ? "ok" : "FAIL");
    if (cnt1 != cnt)
        vLog(xBUG, "problem with ones: cnt = %i, cnt1= %i\n", cnt, cnt1); 
    if (cnt0 != len * 64 - cnt)
        vLog(xBUG, "problem with zeroes: ARRAY_SIZE(data) * 64 - cnt = %i, cnt0= %i\n", len * 64 - cnt, cnt0); 
        
      assert(trk.peak_idx+1 == cnt);
//    assert(cnt1 == cnt);    
//    assert(cnt0 == len * 64 - cnt);
}

void peak_test(void)
{
    uint32_t data[12];
    uint32_t ts = 0;
    int      xl = 0;
    
    profiling_sample_ts = 0;
    profiling_sample_count = 0;
    
    int mx;
    if ((vLogFlags & (1<<xPeakValue|1<<xPeakBits|1<<xPeakVrbs)) == 0)
    {
        mx = 999999;
        vLog(xInfo, "Running %i tests... ", mx);
#ifndef PEAK_TEST_SAME_LINE 
        if (vLogFlags & (1<<xInfo))
            xputc('\n');
#endif    
    }
    else
    {
        mx = 5;
    }
    
    ts += GetTick() + 64;
    xl = xl;

    for (int i = 0; i < mx; i++)
    {
        int len = 1+(i%(ARRAY_SIZE(data)-1));
        
        if (UART_SpiUartGetRxBufferSize() != 0)
        {
            UART_UartGetChar();
            break;
        }
#ifdef PEAK_TEST_SAME_LINE        
        if (TimerExpired(ts))
#endif    
        {
            xl = xprintf(" %4i. noise: %3i, max_peaks: %3i (%7i samples/sec)", i, (i+10)%200, len*64, profiling_sample_count*1024/tu2ms(profiling_sample_ts));
            if ((vLogFlags & (1<<xPeakValue|1<<xPeakBits|1<<xPeakVrbs)) != 0)
            {
                xputc('\n');
            }
            else
            {
#ifdef PEAK_TEST_SAME_LINE        
                for(int i = 0; i < xl; i++)
                    xputc('\b');
                ts = GetTick() + 64;
#endif
            }
        }
#ifndef PEAK_TEST_SAME_LINE        
        int tsb = GetTick();
#endif        
        test_waveform(data, len, (i+10)%200);
#ifndef PEAK_TEST_SAME_LINE        
        if ((vLogFlags & (1<<xPeakValue|1<<xPeakBits|1<<xPeakVrbs)) == 0)
            xprintf(" %ims\n", GetTick() - tsb);
#endif            
    }
    if ((vLogFlags & (1<<xPeakValue|1<<xPeakBits|1<<xPeakVrbs)) == 0)
    {
        xprintf("done\n");
    }
}

#endif