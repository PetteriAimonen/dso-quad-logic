#include <math.h>
#include <stdlib.h>
#include "BIOS.h"
#include "stm32f10x.h"
#include "ds203_io.h"
#include "signal_generator.h"
#include "rms_measurement.h"
#include "mathutils.h"
#include "tinyprintf.h"
#include "Interrupt.h"

/* ---------- Screen scaling ------------ */
#define GRAPH_LEFT 35
#define GRAPH_RIGHT 370
#define GRAPH_BOTTOM 40
#define GRAPH_TOP 220
#define MIN_DB -70
#define MAX_DB 10
#define MIN_FREQ 10
#define MAX_FREQ 200000

// Compensation for DAC delay, in microseconds
#define DAC_DELAY 0.5f

// Output voltage of the DAC. You can calibrate this by checking out the
// reading at the bottom of the screen when input is directly connected to
// the output.
#define VRMS_REFERENCE 0.93f

// Screen colors
#define BORDER_COLOR RGB565RGB(255, 255, 255)
#define MAJOR_GRID RGB565RGB(255, 0, 0)
#define MINOR_GRID RGB565RGB(128, 0, 0)
#define TEXT_COLOR RGB565RGB(255, 255, 255)
#define DB_COLOR RGB565RGB(100, 255, 100)
#define PHASE_COLOR RGB565RGB(255, 255, 100)

// Get the floating-point X coordinate on screen from frequency in Hz
static float getxf(int freq) {
    const float scale = (GRAPH_RIGHT - GRAPH_LEFT) / (log10f(MAX_FREQ) - log10f(MIN_FREQ));
    float rel_x = (log10f(freq) - log10f(MIN_FREQ)) * scale;
    float x = rel_x + GRAPH_LEFT;
    
    if (x > GRAPH_RIGHT) x = GRAPH_RIGHT;
    if (x < GRAPH_LEFT) x = GRAPH_LEFT;
    
    return x;
}

// Get the integer X coordinate on screen
static int getx(int freq) { return (int)(getxf(freq) + 0.5f); }

// Get the floating-point Y coordinate on screen from decibel value
static float getyf(float db) {
    const float scale = (float)(GRAPH_TOP - GRAPH_BOTTOM) / (MAX_DB - MIN_DB);
    float y = (db - MIN_DB) * scale + GRAPH_BOTTOM;
    
    if (y > GRAPH_TOP) y = GRAPH_TOP;
    if (y < GRAPH_BOTTOM) y = GRAPH_BOTTOM;
    
    return y;
}

// Get the integer Y coordinate on screen from decibels
static int gety(float db) { return (int)(getyf(db) + 0.5f); }

// Get the floating-point Y coordinate on screen from phase in degrees
static float getyf_phase(float phase) {
    const float scale = (float)(GRAPH_TOP - GRAPH_BOTTOM) / 360.0f;
    float y = (phase + 180.0f) * scale + GRAPH_BOTTOM;
    
    if (y > GRAPH_TOP) y = GRAPH_TOP;
    if (y < GRAPH_BOTTOM) y = GRAPH_BOTTOM;
    
    return y;
}

// Get the integer Y coordinate on screen from degrees
static int gety_phase(float phase) { return (int)(getyf_phase(phase) + 0.5f); }

/* ------------- Graph grid drawing ------------ */
void draw_graph_background() {
    __Clear_Screen(0);
    
    lcd_printf(80, GRAPH_TOP + 1, TEXT_COLOR, 0, "Frequency response for DSO Quad");
    
    // Draw frequency grid & tick labels
    for (int freq = MIN_FREQ; freq <= MAX_FREQ; freq *= 10)
    {
        // Minor line
        for (int mfreq = freq; mfreq < freq * 10; mfreq += freq)
        {
            vertdots(getx(mfreq), GRAPH_BOTTOM, GRAPH_TOP, MINOR_GRID);
        }
        
        // Major line
        vertdots(getx(freq), GRAPH_BOTTOM - 2, GRAPH_TOP, MAJOR_GRID);
        
        if (freq < 1000)
        {
            lcd_printf(getx(freq) - 12, GRAPH_BOTTOM - 16, TEXT_COLOR, 0,
                       "%2d", freq);
        } else {
            lcd_printf(getx(freq) - 12, GRAPH_BOTTOM - 16, TEXT_COLOR, 0,
                       "%2dk", freq / 1000);
        }
    }
    
    // Draw dB grid & tick labels
    for (int db = MIN_DB + 5; db < MAX_DB; db += 5)
    {
        if (db % 10 == 0)
        {
            horizdots(GRAPH_LEFT, GRAPH_RIGHT, gety(db), MAJOR_GRID);
            lcd_printf(0, gety(db) - 7, DB_COLOR, 0,
                       "%3d", db);
        }
        else
        {
            horizdots(GRAPH_LEFT, GRAPH_RIGHT, gety(db), MINOR_GRID);
        }
    }
    
    // Draw phase tick labels
    for (int phase = -90; phase <= 90; phase += 90)
    {
        lcd_printf(GRAPH_RIGHT + 4, gety_phase(phase) - 7, PHASE_COLOR, 0,
                   "%d", phase);
    }
    
    // Axis labels
    lcd_printf(0, GRAPH_TOP - 7, DB_COLOR, 0, " dB");
    lcd_printf(GRAPH_RIGHT + 4, GRAPH_TOP - 7, PHASE_COLOR, 0, "Deg");
    lcd_printf(GRAPH_RIGHT, GRAPH_BOTTOM - 16, TEXT_COLOR, 0, "Hz");
    
    // Graph borders
    drawline(GRAPH_LEFT, GRAPH_TOP, GRAPH_RIGHT, GRAPH_TOP, BORDER_COLOR);
    drawline(GRAPH_LEFT, GRAPH_BOTTOM, GRAPH_RIGHT, GRAPH_BOTTOM, BORDER_COLOR);
    drawline(GRAPH_LEFT, GRAPH_TOP, GRAPH_LEFT, GRAPH_BOTTOM, BORDER_COLOR);
    drawline(GRAPH_RIGHT, GRAPH_TOP, GRAPH_RIGHT, GRAPH_BOTTOM, BORDER_COLOR);
}

/* ------------ Main program --------------- */

// Structure for storing the results in RAM
typedef struct {
    int frequency;
    float dB;
    float phase;
} result_t;

#define RESULTS_SIZE 400
static int results_count;
static result_t results[RESULTS_SIZE];

// Write the results from RAM to CSV file
bool save_csv(const char *filename)
{
    _fopen_wr(filename);
    _fprintf("# Freq   dB    Phase(deg)\r\n");
        
    for (int i = 0; i < results_count; i++)
    {
        _fprintf("%10d %3d.%03d ",
            results[i].frequency,
            (int)results[i].dB, abs((int)(results[i].dB * 1000)) % 1000);
        
        if (!isnan(results[i].phase))
        {
            // Phase is NaN if the amplitude was too small to be detected
            _fprintf("%3d.%02d\r\n",
            (int)results[i].phase,  abs((int)(results[i].phase * 100)) % 100);
        }
        else
        {
            _fprintf("\r\n");
        }
    }
    
    return _fclose();
}

int main(void)
{   
    int choice;
    draw_graph_background();
    
    __Set(ADC_CTRL, EN);       
    __Set(ADC_MODE, SEPARATE);               

    __Set(CH_A_COUPLE, AC);
    __Set(CH_A_RANGE, ADC_1V);
    DelayMs(1000); // Wait for ADC to settle
    
    int points_per_decade = 10;
    while (1) {
        clearline(0);
        debugf("Measuring... hold key to abort...   ");
        
        results_count = 0;
        float prev_x = 0, prev_y = 0, prev_y_phase = 0;
        const float freq_mul = powf(10, 1.0f/points_per_decade);
        for (float freq = MIN_FREQ; freq <= MAX_FREQ; freq *= freq_mul)
        {
            // Set the output frequency and wait for the circuit to react
            int real_freq = set_sine_frequency(freq);
            DelayMs(10);
            
            // Measure phase and amplitude on channel A
            float phase;
            float vrms = measure_vrms(real_freq, &phase);
            
            // Calculate dB by comparing to the output value from signal
            // generator.
            float vref = VRMS_REFERENCE;
            float dB = 20 * log10f(vrms / vref);
            
            // Compensate the phase measurement for DAC delay
            // (affects frequencies > 20kHz)
            phase += DAC_DELAY / 1000000.0f * real_freq * 360.0f;
            if (phase > 180.0f) phase -= 360.0f;
            
            float x = getxf(real_freq);
            float y = getyf(dB);
            float y_phase = getyf_phase(phase);
            
            // Draw a line between the previous location and the new one.
            if (prev_x != 0)
            {
                drawline(prev_x, prev_y, x, y, DB_COLOR);
                
                if (abs(prev_y_phase - y_phase) < 100 && !isnan(phase))
                {
                    // Don't draw the phase line if it would cross the screen
                    drawline(prev_x, prev_y_phase, x, y_phase, PHASE_COLOR);
                }
            }
            
            prev_x = x;
            prev_y = y;
            prev_y_phase = y_phase;
            
            // Store results to RAM for later saving to file
            if (results_count < RESULTS_SIZE)
            {
                results[results_count].frequency = real_freq;
                results[results_count].dB = dB;
                results[results_count].phase = phase;
                results_count++;
            }
            
            debugf("%d Hz: Vrms %d mV  dB %d.%02d  phase %d    ",
                   (int)real_freq, (int)(vrms * 1000),
                   (int)dB, abs((int)(dB * 100)) % 100, (int)phase);
            
            if (__Get(KEY_STATUS) ^ ALL_KEYS)
            {
                clearline(0);
                debugf("Aborted!");
                DelayMs(2000);
                break;
            }
        }
        
        // Measurement done, now let the user save the results
        while(1)
        {
            choice = show_menu(0, 0, 0xFFFF,
                    "Menu:", "Fast", "Slow", "Draw over", "Save BMP", "Save CSV", NULL);
            if (choice == 0) // Start a new fast scan
            {
                points_per_decade = 10;
                draw_graph_background();
                break;
            }
            else if (choice == 1) // Start a slow scan
            {
                points_per_decade = 50;
                draw_graph_background();
                break;
            }
            else if (choice == 2) // Draw a new line over the previous ones
            {
                break;
            }
            else if (choice == 3) // Save bitmap image and return to menu
            {
                char *filename = select_filename("FREQ%03d.BMP");
                clearline(0);
                debugf("Writing %s...", filename);
                if (write_bitmap(filename))
                    debugf("%s written!    ", filename);
                else
                    debugf("Error writing file.  ");
                DelayMs(1000);
            }
            else if (choice == 4) // Save CSV and return to menu
            {
                char *filename = select_filename("FREQ%03d.CSV");
                clearline(0);
                debugf("Writing %s...", filename);
                if (save_csv(filename))
                    debugf("%s written!    ", filename);
                else
                    debugf("Error writing file.  ");
                DelayMs(1000);
            }
        }
    }
    
    return 0;
}

