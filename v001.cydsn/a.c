    else if (
                (lp->v > v + threshold_units && lp->v >= MIN_POSITIVE_PEAK) 
                ||
                (lp->v < v - threshold_units && lp->v < MIN_NEGATIVE_PEAK) 
            )
    {
        // previous peak is final. add a new peak
        t->peak_idx++; // new peak
        save_peak(t, v, delay);                
    }
