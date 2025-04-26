#include <math.h>
#include <string.h>
#include <ctype.h>

#include "plConfig.h"
#include "plplot.h"
#include "filter.h"

IIRFilter filter;

int
main( int argc, char *argv[] )
{
    IIRFilter_init(&filter);
    PLFLT x[1000], y1_in[1000],y1_out[1000];
    PLFLT y2_in[1000], y2_out[1000];
    PLFLT xmin = 0., xmax = 1., ymin = -255., ymax = 255.;

    float sampling = 20000000;
    float sampling_period = 1/sampling;
    int samples_count = 1000;
    float freq1 = 900000;
    float freq2 = 1100000;

    printf("\n\rIIRFilter_TAP_NUM: %d", IIRFilter_TAP_NUM);
    fflush(stdout);
    xmax = samples_count;

    // Prepare data to be plotted.
    for ( int sample = 0; sample < samples_count; sample++ )
    {
        float time_point = ( sample*sampling_period );
        x[sample] = (PLFLT) sample;
        y1_in[sample] = 127*sin(2*M_PI*freq1*time_point);
        y2_in[sample] = 127*sin(2*M_PI*freq2*time_point);
        IIRFilter_put(&filter, y1_in[sample], y2_in[sample]);
        // y2[sample] = 10*sin(2*M_PI*freq*x[sample]);
        float tmp;
        float filter_output;
        float filter_output2;
        IIRFilter_get(&filter,&filter_output,&filter_output2);
        y1_out[sample] = filter_output;
        y2_out[sample] = filter_output2;
    }

    // Parse and process command line arguments
    plparseopts( &argc, argv, PL_PARSE_FULL );

    // plsdev (	"xwin" );
    // Initialize plplot
    // plinit();
     plstar( 1, 2 );

    // Create a labelled box to hold the plot.
    //plenv dodaje osie oraz skale
    //dodaje to jako obraz na nowej substronie
    //dodaje zakresy zmiennych
    //rysuje takźe boxa
    plenv( xmin, xmax, ymin, ymax, 0, 0 );
    //labeling axes
    pllab( "x", "Filtered", "Pass band signal filtering" );

    // Plot the data that was prepared above.
    plline( samples_count, x, y1_in );
    plcol0( 2 );
    plwidth( 2 );
    plline( samples_count, x, y1_out );

    
    plenv( xmin, xmax, ymin, ymax, 0, 1 );
    
    plcol0( 4 );
    pllab( "x", "Filtered", "Stop band signal filtering" );
    plline( samples_count, x, y2_in);
    plcol0( 3 );
    plline( samples_count, x, y2_out);
    

    // Close PLplot library
    plend();

    exit( 0 );
}