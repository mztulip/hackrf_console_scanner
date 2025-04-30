

#include "hackrf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <inttypes.h>


#if defined(__GNUC__)
	#include <unistd.h>
	#include <sys/time.h>
#endif

#include <termios.h>
#include <signal.h>
#include <math.h>
#include "filter.h"

/*
a już wiem dlaczego na SDRrze  do dupy widać ramki. A na nrf51 i efr prosty skaner na rssi.
 Jakoś lepiej bez żadnych zmian parametrów widzę transmisje. nrf51 mierzy RSSI z dynamiką 127dB(teoretycznie,
  w praktyce widzę że jest niecałe 100) . A SDR ma przetwornik 8 bitów co daje dynamikę 48dB(sporo z tego zabiera mi szum 6.02*8).
   Stąd trzeba kręcić wzmocnieniami  i przy mocnym sygnale słabe znikają, a ja widze więcej na nrf51 i efr32

wogóle w hackrf,  scalak od pośredniej za filtrem pośrednie ma wyprowadozny analogowy sygnał RSSIj i ma dynamkie koło 100dB. 
Jest wyprowadzone na testpoint. Więc dupa trywialnie tego nie uruchomie. To już chyba wolę wydrutować jakiegoś PCB.
zrobiłem jeszcze testy z hackrf, i powiem że użyteczny zakres przy pracy z antenami.(nie wpinam kabla bezpośrednio).
Pracując na surowych danych z przetwornika. Gdzie poziom 0dB to 128. To gdy położe anteny tak że leżą na sobie  to dostaje 11dB.
  A gdy oddale na odległośc 0.5m to już spadam na poziom szumu -22dB(LNA 14dB i pośrednia 14dB). Czyli zakresu dynamiki mam koło 30dB.
   Ten zakres nie zależy od ustawień wzmocnienia LNA, dopiero wamacniacz napośredniej zawęża(przy 50dB już widoczne jest podnoszenie się szumu, 
   a przy 60 to szum mam na poziomie -10dB). Ale jak już mam nawet 40dB na pośredniej to mam lepsze zasięgi niż nrf51 i efr32.
    Podsumowując na sdrze mam około 30dB dynamiki.
dodatkowo zrobiłem wersję konsolową skanera z hackrf, aby mi się łatwo porównywało do tego co zrobiłem wczesniej. Szerokość kanału mam 2MHz. 
Co jest niby więcej bo na tamtych miałem 1MHz. Ale  i tak selektywnośc kanałowa jest lepsza bo tutaj filtr na pośredniej jest lepszy.
A moc obliczam z próbek IQ,  a sampluje to 20MHZ. Więc tutaj jeszcze jakiegoś IIRA można wrzucić na IQ i zmniejszyć to do 1MHZ ostro.
*/

bool one_shot = false;
bool finite_mode = false;
const int DEFAULT_SAMPLE_RATE_HZ = (20000000);
const int FREQ_ONE_MHZ = (1000000ull);

int max_value, min_value;
int num_ranges = 0;
int TUNE_STEP_MHZ ;
int OFFSET = 0;
unsigned int lna_gain;
unsigned int vga_gain;
int baseband_filter; 
bool fir_enabled = false;

int start_frequency;
int stop_frequency;
bool amp;

static bool do_exit = false;
static hackrf_device* device = NULL;

int iqSize = 1000;

volatile uint32_t byte_count = 0;
volatile uint64_t sweep_count = 0;
volatile bool sweep_started = false;
uint32_t num_sweeps = 2;

const int FREQ_MAX_MHZ = (7250);

const int BLOCKS_PER_TRANSFER = 16;

uint16_t frequencies[MAX_SWEEP_RANGES * 2];

int32_t marker_pos = 0;
int32_t marker_range = 200;

double min_rssi = 0;
double max_rssi = -255;

void sigint_callback_handler(int signum)
{
	fprintf(stderr, "Caught signal %d\n", signum);
	do_exit = true;
}

void draw_frequency_scale(void)
{
  	printf("\033[2;1H");//move cursor to row2 column1
 	printf("\033[2K"); //erase line
  	for(int i = 0; i <= marker_range; i++)
  	{
  		uint8_t backgorund_color = 0;
  		if(i == marker_pos)
  		{
  			backgorund_color = 1;
  		}
  		if(i%10 == 0)
  		{
  			printf("\e[48;5;%dm|\e[0m", backgorund_color);
  		}
  		else
  		{
  			printf("\e[48;5;%dm \e[0m", backgorund_color);;
  		}
  	}
}

void draw_header(int start_freq, int stop_freq, int step_freq)
{
    char str_buffer[100];

	// https://github.com/0x5c/VT100-Examples/blob/master/vt_seq.md
	// Helpful document for VT100
	printf("\033[!p");
	printf("\033[2J");//Clear 
	//start scroll from line 4
  	printf("\033[4r");
	printf("\033[0;1H");//move cursor to row0 column1
  	printf("\033[2K"); //erase line

    int counter = 0;
    for(int i = start_freq; i <= stop_freq; i+=step_freq)
    {
        if(counter%10 == 0)
        {
            sprintf(str_buffer,"%dMHZ", i);
            printf(str_buffer);
            int chars = 10 - strlen(str_buffer);
            for (int c = 0; c < chars; c++)
            {
                printf(" ");
            }
        }
        counter++;
    }
	printf("\033[3;1H");//move cursor to row3 column1
	printf("\033[2K"); //erase line
	printf("\033[3;41H");//move cursor to row3 column20
	printf("| Use left or right arrow to move marker");
  	draw_frequency_scale();
}

uint8_t colors_vt100[] =  {16,17,18,19,20,21,93,92,91,90,89,88,160};

void print_colors_bar(int16_t min, int16_t max)
{
	int16_t colors_count = sizeof(colors_vt100);
    int16_t values_per_color = (max-min)/colors_count;
	for(int value = min; value <= max; value++)
	{
		int16_t color_index = (value-min)/values_per_color;
		printf("\033[48;5;%dm%d|", colors_vt100[color_index], value);
	}
}

void print_colored(int16_t value, int16_t min, int16_t max)
{
    int16_t colors_count = sizeof(colors_vt100);
    int16_t values_per_color = (max-min)/colors_count;
    int16_t color_index = (value-min)/values_per_color;
	if (color_index < 0) color_index = 0;
    if(color_index >= colors_count)
    {
        color_index = colors_count-1;
    }
    printf("\033[48;5;%dm ", colors_vt100[color_index]);
}

void update_frequency_marker(uint8_t dir)
{
	if(dir)
	{
		marker_pos++;
		if(marker_pos > marker_range)
		{
			marker_pos = 0;
		}
	}
	else
	{
		marker_pos--;
		if(marker_pos < 0)
		{
			marker_pos = marker_range;
		}
	}
}

void draw_frequency_marker(void)
{
	draw_frequency_scale();
  	printf("\033[3;1H");//move cursor to row3 column1
 	uint32_t frequency = start_frequency+marker_pos*TUNE_STEP_MHZ;
  	printf("Marker pos: %luMHz",frequency);
}

// http://t-filter.engineerjs.com/

IIRFilter filter;
int rx_callback(hackrf_transfer* transfer)
{
	int i,j;
	int8_t* buf;
	uint8_t* ubuf; //uzywany do wskazywania na poczatek bloku w transferz danych
	byte_count += transfer->valid_length;
	uint64_t frequency; /* in Hz */
	buf = (int8_t*) transfer->buffer;

	if (do_exit) {return 0;}

	for (j = 0; j < BLOCKS_PER_TRANSFER; j++)
	{
		ubuf = (uint8_t*) buf;
		if (ubuf[0] == 0x7F && ubuf[1] == 0x7F) 
		{
			frequency = ((uint64_t) (ubuf[9]) << 56) |
				((uint64_t) (ubuf[8]) << 48) |
				((uint64_t) (ubuf[7]) << 40) |
				((uint64_t) (ubuf[6]) << 32) |
				((uint64_t) (ubuf[5]) << 24) |
				((uint64_t) (ubuf[4]) << 16) |
				((uint64_t) (ubuf[3]) << 8) | ubuf[2];
			// printf("\n\rFrequency: %lu MHz", frequency / FREQ_ONE_MHZ);
		}
		else 
		{
			buf += BYTES_PER_BLOCK;
			continue;
		}

		//Tutaj ladujemy gdy jest to blok poczatkowy z informacja o czestotliwosci

		//Początkowa czesottliwosc skanu to startujemy sweep
		uint64_t start_freq = (uint64_t) (FREQ_ONE_MHZ * (uint64_t)frequencies[0]);
		// printf("\n\rComp: %llu vs %llu",frequency, start_freq);
		
		if (frequency == start_freq) 
		{
			printf("\e7"); //save cursor position
			draw_frequency_marker();
			printf("\e8"); //restore cursor position
			printf("\033[0m");
			printf("Max: %d Min: %d\n\r", (int)max_rssi, (int)min_rssi);
		
			min_rssi = 0;
        	max_rssi = -255;
			if (sweep_started) 
			{
				sweep_count++;
				if (one_shot) 
				{
					do_exit = true;
				}
				else if (finite_mode && sweep_count == num_sweeps) 
				{
					do_exit = true;
				}
			}
			sweep_started = true;
		}
		if (do_exit) {return 0;}


		//sweep nie jest started to przesuwamy wskaznik bufora o jeden blok danych
		if (!sweep_started) 
		{
			//Nie rozpoczeto sweepa to dalej analiza danych 
			buf += BYTES_PER_BLOCK;
			continue;
		}

		//sweep jest started, ale otrzymano block z informacją o czestotliwosci
		//jesli czestotliwosc bloku w odebranym transferze jest wieksza od maksymalnej
		// to przesun bufor o blok, czyli ignorujemy dane
		if ((FREQ_MAX_MHZ * FREQ_ONE_MHZ) < frequency) 
		{
			buf += BYTES_PER_BLOCK;
			continue;
		}

		//wreszcie wszystko ok, jest blad poczatkowy
		//ktorego czestotliwosc jest wewnatrz skanowanego pasma
		//i nie wykracza poza maksymalna mozliwą do uzyskania przez SDR
		float block_power = 0.0;
		float squares_sum = 0;

		// printf("\n\rSamples to use: %d", BYTES_PER_BLOCK-10);
		buf += BYTES_PER_BLOCK - (iqSize * 2);
		
		IIRFilter_init(&filter);
		for (i = 0; i < iqSize; i++) 
		{
			float i_sample= buf[i * 2]/255.0;
			float q_sample = buf[i * 2 + 1]/255.0;

			if( fir_enabled )
			{
				IIRFilter_put(&filter, i_sample, q_sample);
				float filtered_i, filtered_q;
				IIRFilter_get(&filter,&filtered_i,&filtered_q);
				//Ignore first filter length outputs samples, due to initial conditions
				if (i > IIRFilter_TAP_NUM)
				{
					float v_peak = sqrt(filtered_i*filtered_i+filtered_q*filtered_q);
					squares_sum += v_peak*v_peak;
				}
			}
			else
			{
				float v_peak = sqrt(i_sample*i_sample+q_sample*q_sample);
				squares_sum += v_peak*v_peak;
			}

		}
		buf += iqSize * 2;
		float v_rms = sqrt(squares_sum/iqSize);
		//Logarithm refered to ADC LSB 10*log(adc_rms_raw/adc_LSB)
		float raw_log = 10*log(v_rms/1);
		if(frequency%(TUNE_STEP_MHZ*1000000) == 0)
		{
			// printf("\n\rFreq:%llu Adc log:%f", frequency, raw_log);
			print_colored((int16_t)(raw_log), min_value, max_value);
			if(raw_log > max_rssi) max_rssi = raw_log;
            if(raw_log < min_rssi) min_rssi = raw_log;
		}
	}
	return 0;
}

char getch(void)
{
    char buf = 0;
    struct termios old = {0};
    fflush(stdout);
    if(tcgetattr(0, &old) < 0)
        perror("tcsetattr()");
    old.c_lflag &= ~ICANON;
    old.c_lflag &= ~ECHO;
    old.c_cc[VMIN] = 0;
    old.c_cc[VTIME] = 1; //1s timeout
    if(tcsetattr(0, TCSANOW, &old) < 0)
        perror("tcsetattr ICANON");
	int result = read(0, &buf, 1);
    if(result < 0)
        perror("read()");
	if (result == 0) return 0;
    // old.c_lflag |= ICANON;
    // old.c_lflag |= ECHO;
    // if(tcsetattr(0, TCSADRAIN, &old) < 0)
    //     perror("tcsetattr ~ICANON");
    return buf;
}



int main(int argc, char** argv)
{
	signal(SIGINT, &sigint_callback_handler);
	// signal(SIGILL, &sigint_callback_handler);
	// signal(SIGFPE, &sigint_callback_handler);
	// signal(SIGSEGV, &sigint_callback_handler);
	// signal(SIGTERM, &sigint_callback_handler);
	// signal(SIGABRT, &sigint_callback_handler);

	if(argc == 2 && argv[1][0] == '5')
	{
		start_frequency = 5100;
		stop_frequency = 6000;
		TUNE_STEP_MHZ = 5; 
		OFFSET = 0;
		amp = true; //first LNA 11dB
		//Przy 16 jest przesterowanie
		//Pojawia sie jakiś sygnał na okolicah 5170MHZ i na lewo od tego podniesiony poziom
		//Przy 0 wygląda lepiej choć ten prażek się  pojawia dalej. 
		lna_gain = 8; 

		vga_gain = 50;
		baseband_filter = (2500000);
		max_value = -20;
		min_value = -38;
		fir_enabled = false;
	}
	else
	{
		start_frequency = 2400;
		stop_frequency = 2600;
		TUNE_STEP_MHZ = 1; 
		OFFSET = 0;
		amp = false; //first LNA 11dB(choć na wyswietlaczu pokazuje 14)
		lna_gain = 0; 
		vga_gain = 40;
		//przy 60 juz mam mocno zawęzone. Przy 40dB jest bardzo ładnie.
		//Przy 50dB poziom szumu zaczyna się podnosić.
		//Przy 60dB juz jest 10dB wyżej.
		baseband_filter = (1000000);
		max_value = 0;
		min_value = -55;
		fir_enabled = true;
	}

	print_colors_bar(min_value, max_value);

	int	result = hackrf_init();
	if (result != HACKRF_SUCCESS) 
	{
		fprintf(stderr,
			"hackrf_init() failed: %s (%d)\n",
			hackrf_error_name(result),
			result);
		return EXIT_FAILURE;
	}

	result = hackrf_open_by_serial(NULL, &device);
	if (result != HACKRF_SUCCESS) 
	{
		fprintf(stderr,
			"hackrf_open() failed: %s (%d)\n",
			hackrf_error_name(result),
			result);
		return EXIT_FAILURE;
	}

	fprintf(stderr,
			"call hackrf_sample_rate_set(%.03f MHz)\n",
			((float) DEFAULT_SAMPLE_RATE_HZ / (float) FREQ_ONE_MHZ));
	result = hackrf_set_sample_rate_manual(device, DEFAULT_SAMPLE_RATE_HZ, 1);
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr,
			"hackrf_sample_rate_set() failed: %s (%d)\n",
			hackrf_error_name(result),
			result);
		return EXIT_FAILURE;
	}

	frequencies[2 * num_ranges] = (uint16_t) start_frequency;
	frequencies[2 * num_ranges + 1] = (uint16_t) stop_frequency;
	num_ranges = 1;

	fprintf(stderr,
			"call hackrf_baseband_filter_bandwidth_set(%.03f MHz)\n",
			((float) baseband_filter / (float) FREQ_ONE_MHZ)
			);
	result = hackrf_set_baseband_filter_bandwidth(
				device,
				baseband_filter
				);

	if (result != HACKRF_SUCCESS) 
	{
		fprintf(stderr,
				"hackrf_baseband_filter_bandwidth_set() failed: %s (%d)\n",
				hackrf_error_name(result),
				result
				);
		return EXIT_FAILURE;
	}

	if (lna_gain % 8)
	{
		fprintf(stderr, "warning: lna_gain (-l) must be a multiple of 8\n");
		return 1;
	}

	if (vga_gain % 2)
	{
		fprintf(stderr, "warning: vga_gain (-g) must be a multiple of 2\n");
		return 1;
	}
	result = hackrf_set_vga_gain(device, vga_gain);
	result |= hackrf_set_lna_gain(device, lna_gain);

	printf("\n\rTUNE_STEP_MHZ: %dMHz",TUNE_STEP_MHZ);
	for (int i = 0; i < num_ranges; i++) 
	{
		fprintf(stderr,
			"Sweeping from %u MHz to %u MHz\n",
			frequencies[2 * i],
			frequencies[2 * i + 1]);
	}
	result = hackrf_init_sweep(
		device,
		frequencies,
		num_ranges,
		BYTES_PER_BLOCK,
		TUNE_STEP_MHZ * FREQ_ONE_MHZ,
		OFFSET,
		INTERLEAVED);
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr,
			"hackrf_init_sweep() failed: %s (%d)\n",
			hackrf_error_name(result),
			result);
		return EXIT_FAILURE;
	}

	printf("\n\rBYTES_PER_BLOCK:%d", BYTES_PER_BLOCK);

	result |= hackrf_start_rx_sweep(device, rx_callback, NULL);
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr,
			"hackrf_start_rx_sweep() failed: %s (%d)\n",
			hackrf_error_name(result),
			result);
		return EXIT_FAILURE;
	}

	if (amp) {
		fprintf(stderr, "call hackrf_set_amp_enable(%u)\n", 1);
		result = hackrf_set_amp_enable(device, (uint8_t) 1);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr,
				"hackrf_set_amp_enable() failed: %s (%d)\n",
				hackrf_error_name(result),
				result);
			return EXIT_FAILURE;
		}
	}

	draw_header(start_frequency, stop_frequency, TUNE_STEP_MHZ);
	while((hackrf_is_streaming(device) == HACKRF_TRUE) && (do_exit == false))
	{
		char c = getch();
		if (c != 0)
		{
			// printf("\n\rChar: %c\n", c);
			if(c == '[')
			{
				char c2 = getch();
				if(c2 == 'D') //Left key
				{
					update_frequency_marker(0);
				}
				else if(c2 == 'C') //Right key
				{
					update_frequency_marker(1);
				}
			}
			else if(c == 'r')
			{
				draw_header(start_frequency, stop_frequency, TUNE_STEP_MHZ);
			}
		}
	}

	result = hackrf_is_streaming(device);
	if (do_exit) {
		fprintf(stderr, "\nExiting...\n");
	} else {
		fprintf(stderr,
			"\nExiting... hackrf_is_streaming() result: %s (%d)\n",
			hackrf_error_name(result),
			result);
	}

	if (device != NULL) 
	{
		result = hackrf_close(device);
		if (result != HACKRF_SUCCESS) 
		{
			fprintf(stderr,
				"hackrf_close() failed: %s (%d)\n",
				hackrf_error_name(result),
				result);
		} 
		else 
		{
			fprintf(stderr, "hackrf_close() done\n");
		}

		hackrf_exit();
		fprintf(stderr, "hackrf_exit() done\n");
	}
	return 0;
}
