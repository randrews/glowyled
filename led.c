#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>

#include "usbdrv.h"
#include "light_ws2812.h"

#define abs(x) ((x) > 0 ? (x) : (-x))

struct cRGB ledState[1] = { {0, 0, 0} };
unsigned char shouldSend = 0; // Should we update the LED in the next loop?

unsigned char sequence[] = {1,3,2,6,4,5}; // Sequence of shifts. Each one is a bitfield for rgb, r=1,g=2,b=4.
unsigned char sequenceLength = 6;
unsigned char current; // Which element of the sequence we're working *toward*
unsigned char dR, dG, dB; // Deltas to add to R, G, B this shift
unsigned char currentCycles; // Number of times we've updated in this shift

void setDeltas(unsigned char start, unsigned char stop);

void startSequence(){
    currentCycles = 0;
    current = 1; // We start at state 0, working toward 1

    ledState[0].r = (sequence[0] & 1 ? 0xff : 0);
    ledState[0].g = (sequence[0] & 2 ? 0xff : 0);
    ledState[0].b = (sequence[0] & 4 ? 0xff : 0);

    setDeltas(sequence[0], sequence[1]);
}

void setDeltas(unsigned char start, unsigned char stop){
    dR = ((start & 1) && !(stop & 1) ? -1 : // 1 to 0, negative
          !(start & 1) && (stop & 1) ? 1 : // 0 to 1, positive
          0); // Anything else, no change

    dG = ((start & 2) && !(stop & 2) ? -1 : // 1 to 0, negative
          !(start & 2) && (stop & 2) ? 1 : // 0 to 1, positive
          0); // Anything else, no change

    dB = ((start & 4) && !(stop & 4) ? -1 : // 1 to 0, negative
          !(start & 4) && (stop & 4) ? 1 : // 0 to 1, positive
          0); // Anything else, no change
}

void step(){
    if(currentCycles == 0xff) {
        // Time to shift to the next state
        unsigned char start = sequence[current];
        current = (current + 1) % sequenceLength;
        unsigned char stop = sequence[current];
        setDeltas(start, stop);
        currentCycles = 0;
    } else {
        ledState[0].r += dR;
        ledState[0].g += dG;
        ledState[0].b += dB;
        ws2812_setleds(ledState, 1);
        currentCycles++;
    }
}

USB_PUBLIC uchar usbFunctionSetup(uchar data[8]) {
    return 0;
}

USB_PUBLIC uchar usbFunctionWrite(uchar *data, uchar len) {
    return 1;
}

// This was ripped straight from the example code
// Called by V-USB after device reset
void hadUsbReset() {
    int frameLength, targetLength = (unsigned)(1499 * (double)F_CPU / 10.5e6 + 0.5);
    int bestDeviation = 9999;
    uchar trialCal, bestCal, step, region;

    // do a binary search in regions 0-127 and 128-255 to get optimum OSCCAL
    for(region = 0; region <= 1; region++) {
        frameLength = 0;
        trialCal = (region == 0) ? 0 : 128;
        
        for(step = 64; step > 0; step >>= 1) {
            if(frameLength < targetLength) // true for initial iteration
                trialCal += step; // frequency too low
            else
                trialCal -= step; // frequency too high
                
            OSCCAL = trialCal;
            frameLength = usbMeasureFrameLength();
            
            if(abs(frameLength-targetLength) < bestDeviation) {
                bestCal = trialCal; // new optimum found
                bestDeviation = abs(frameLength -targetLength);
            }
        }
    }

    OSCCAL = bestCal;
}

// timer0 overflw
ISR(TIMER1_OVF_vect) {
    // process the timer1 overflow here
    shouldSend++;
}

void setupTimer(){
    TIMSK|=(1<<TOIE1);
    TCNT1 = 0;
    TCCR1 = (1<<CS13);
}

int main() {
    ws2812_setleds(ledState, 1);

    /* wdt_enable(WDTO_1S); // enable 1s watchdog timer */

    usbInit();
	
    usbDeviceDisconnect(); // enforce re-enumeration
    for(uchar i = 0; i<250; i++) { // wait 500 ms
        wdt_reset(); // keep the watchdog happy
        _delay_ms(2);
    }
    usbDeviceConnect();

    setupTimer();
    startSequence();
    sei(); // Enable interrupts after re-enumeration

    while(1) {
        wdt_reset(); // keep the watchdog happy
        usbPoll();
        if(shouldSend){
            shouldSend = 0;
            step();
        }
    }
	
    return 0;
}
