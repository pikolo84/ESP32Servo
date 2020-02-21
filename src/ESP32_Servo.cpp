/*
Copyright (c) 2017 John K. Bennett. All right reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

* Notes on the implementation:
* The ESP32 supports 16 hardware LED PWM channels that are intended
* to be used for LED brightness control. The low level ESP32 code
* (esp32-hal-ledc.*) allows us to set the PWM frequency and bit-depth,
* and then manipulate them by setting bits in the relevant control
* registers.
*
* Different servos require different pulse widths to vary servo angle, but the range is
* an approximately 500-2500 microsecond pulse every 20ms (50Hz). In general, hobbyist servos
* sweep 180 degrees, so the lowest number in the published range for a particular servo
* represents an angle of 0 degrees, the middle of the range represents 90 degrees, and the top
* of the range represents 180 degrees. So for example, if the range is 1000us to 2000us,
* 1000us would equal an angle of 0, 1500us would equal 90 degrees, and 2000us would equal 180
* degrees. We vary pulse width (recall that the pulse period is already set to 20ms) as follows:
*
* The ESP32 PWM timers allow us to set the timer width (max 20 bits). Thus
* the timer "tick" length is (pulse_period/2**timer_width), and the equation for pulse_high_width
* (the portion of the 20ms cycle that the signal is high) becomes:
*
*                  pulse_high_width  = count * tick_length
*                                    = count * (pulse_period/2**timer_width)
*
*            and   count = (pulse_high_width / (pulse_period/2**timer_width))
*
* So, for example, if I want a 1500us pulse_high_width, I set pulse_period to 20ms (20000us)
* (this value is set in the ledcSetup call), and count (used in the ledcWrite call) to
* 1500/(20000/65536), or 4924. This is the value we write to the timer in the ledcWrite call.
* If we increase the timer_width, the timer_count values need to be adjusted.
*
* The servo signal pins connect to any available GPIO pins on the ESP32, but not all pins are
* GPIO pins.
*
* The ESP32 is a 32 bit processor that includes FP support; this code reflects that fact.
*/

#include "ESP32_Servo.h"
#include "esp32-hal-ledc.h"
#include "Arduino.h"

Servo::Servo() {}

Servo::Servo(int pin, int channel)
{
    this->servoChannel = channel;
    // initialize this channel with plausible values, except pin # (we set pin # when attached)
    this->ticks = DEFAULT_PULSE_WIDTH_TICKS;   
    this->timer_width = DEFAULT_TIMER_WIDTH;
    this->pinNumber = pin;
    this->min = MIN_PULSE_WIDTH;
    this->max = MAX_PULSE_WIDTH;
    this->timer_width_ticks = pow(2,this->timer_width);
}

void Servo::attach()
{
    if (!isAttacched) {
        ledcSetup(this->servoChannel, REFRESH_CPS, this->timer_width); // channel #, 50 Hz, timer width
        ledcAttachPin(this->pinNumber, this->servoChannel);   // GPIO pin assigned to channel 
        isAttacched = true;
    }
}

void Servo::detach()
{
    if (isAttacched)
    {
        ledcDetachPin(this->pinNumber);
        isAttacched = false;
    }
}

void Servo::writeMicroseconds(int value)
{
    // calculate and store the values for the given channel
    if (isAttacched)   // ensure channel is valid
    {
        if (value < this->min)          // ensure pulse width is valid
            value = this->min;
        else if (value > this->max)
            value = this->max;

        value = usToTicks(value);  // convert to ticks
        this->ticks = value;
        // do the actual write
        ledcWrite(this->servoChannel, this->ticks);
    }
}

int Servo::readMicroseconds()
{
    int pulsewidthUsec;
    if (isAttacched)
    { 
        pulsewidthUsec = ticksToUs(this->ticks);
    }
    else
    {
        pulsewidthUsec = 0;
    }

    return (pulsewidthUsec);
}

bool Servo::attached()
{
    return isAttacched;
}

void Servo::setTimerWidth(int value)
{
    // only allow values between 16 and 20
    if (value < 16)
        value = 16;
    else if (value > 20)
        value = 20;
        
    // Fix the current ticks value after timer width change
    // The user can reset the tick value with a write() or writeUs()
    int widthDifference = this->timer_width - value;
    // if positive multiply by diff; if neg, divide
    if (widthDifference > 0)
    {
        this->ticks << widthDifference;
    }
    else
    {
        this->ticks >> widthDifference;
    }
    
    this->timer_width = value;
    this->timer_width_ticks = pow(2,this->timer_width);
    
    // If this is an attached servo, clean up
    if ((this->servoChannel <= MAX_SERVOS) && (this->attached()))
    {
        // detach, setup and attach again to reflect new timer width
        ledcDetachPin(this->pinNumber);
        ledcSetup(this->servoChannel, REFRESH_CPS, this->timer_width);
        ledcAttachPin(this->pinNumber, this->servoChannel);
    }        
}

int Servo::readTimerWidth()
{
    return (this->timer_width);
}

int Servo::usToTicks(int usec)
{
    return (int)((float)usec / ((float)REFRESH_USEC / (float)this->timer_width_ticks));   
}

int Servo::ticksToUs(int ticks)
{
    return (int)((float)ticks * ((float)REFRESH_USEC / (float)this->timer_width_ticks)); 
}

 