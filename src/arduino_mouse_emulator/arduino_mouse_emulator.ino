/*
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**************************
 *  Defines
 *************************/

#define JOYSTICK_HORIZONTAL_PIN     A6          /* Analog input pin for horizontal joystick value. */
#define JOYSTICK_VERTICAL_PIN       A7          /* Analog input pin for vertical joystick value. */
#define JOYSTICK_SWITCH_PIN         9           /* Joystick switch pin. */
#define LED_GREEN_PIN               6           /* Green LED pin. */
#define LED_RED_PIN                 5           /* Red LED pin. */

#define SENSOR_MIN_VALUE            0           /* The minimum value of the joystick sensor. */
#define SENSOR_MAX_VALUE            1024        /* The maximum value of the joystick sensor. */
#define STEP_OFFSET                 20          /* Based on joystick sensor value interval, how much for one step? */

#define HEARTBEAT_TIMEOUT_MS        10          /* The heartbeat timeout of the arduino. */
#define KEEP_ALIVE_TIMEOUT_MS       10000       /* Timeout before looking for the start event again. */

#define HELLO_EVENT                 '0'
#define START_EVENT                 '1'
#define KEEP_ALIVE_EVENT            '2'


/**************************
 *  Global variables
 *************************/

int joystickHorizontalNeutralPosValue;          /* The neutral position value of the joystick horizontally. */
int joystickVerticalNeutralPosValue;            /* The neutral position value of the joystick vertically. */
int joystickHorizontalValue;                    /* The joystick horizontal value. */
int joystickVerticalValue;                      /* The joystick vertical value. */

int horizontalPixelSteps;                       /* Stores the current horizontal pixel steps value. */
int verticalPixelSteps;                         /* Stores the current vertical pixel steps value. */
String horizontalPixelStepsSign;                /* Stores the current horizontal pixel steps value sign (+/-). */
String verticalPixelStepsSign;                  /* Stores the current vertical pixel steps value sign (+/-). */

int joystickSwitchState;                        /* The state of the joystick switch. */

char receivedSerial;
uint16_t keepAliveCounter = 0;


/**********************************************************************************************************************
 *  Setup routine
 *********************************************************************************************************************/
void setup()
{/*{{{*/
    /* Start serial communication. */
    Serial.begin(9600);

    /* Joystick sensors. */
    pinMode(JOYSTICK_HORIZONTAL_PIN, INPUT);        /* Analog input for horizontal sensor. */
    pinMode(JOYSTICK_VERTICAL_PIN, INPUT);          /* Analog input for vertical sensor. */

    /* Switches. */
    pinMode(JOYSTICK_SWITCH_PIN, INPUT_PULLUP);     /* Simulates scroll wheel. */

    /* LEDs */
    pinMode(LED_GREEN_PIN, OUTPUT);                 /* The green LED. */
    pinMode(LED_RED_PIN, OUTPUT);                   /* The red LED. */

    /* Short delay to let outputs settle. */
    delay(100);

    /* Initial joystick values (Joystick should be in neutral position when reading these). */
    joystickHorizontalNeutralPosValue = analogRead(JOYSTICK_HORIZONTAL_PIN);
    joystickVerticalNeutralPosValue = analogRead(JOYSTICK_VERTICAL_PIN);
}/*}}}*/


/**********************************************************************************************************************
 *  Main program loop
 *********************************************************************************************************************/
void loop()
{/*{{{*/
    /* Wait for the start event via serial communication. */
    wait_for_start_event();

    while (true)
    {
        if (!check_keep_alive())
        {
            break;
        }

        read_joystick_sensors();
        read_switches();
        convert_to_pixel_steps();
        set_pixel_steps_signs();
        transmit_data_package_frame();

        delay(HEARTBEAT_TIMEOUT_MS);
    }
}/*}}}*/


/**********************************************************************************************************************
 *  Wait for the start event via serial communication.
 *********************************************************************************************************************/
void wait_for_start_event()
{/*{{{*/
    set_leds_active_state(false);       /* Green LED on. */

    while (true)
    {
        if (Serial.available())
        {
            receivedSerial = Serial.read();

            if (receivedSerial == HELLO_EVENT)
            {
                Serial.print("HELLO");
                continue;
            }
            else if (receivedSerial == START_EVENT)
            {
                /* Start message received, let's start sensor reading and transmission! */
                break;
            }
        }
    }

    set_leds_active_state(true);        /* Red LED on. */
}/*}}}*/


/**********************************************************************************************************************
 *  Alternate the green and red LEDs. True turns on green and turns off red, false turns off green and turns on red.
 *********************************************************************************************************************/
void set_leds_active_state(bool active)
{/*{{{*/
    digitalWrite(LED_GREEN_PIN, active);
    digitalWrite(LED_RED_PIN, !active);
}/*}}}*/


/**********************************************************************************************************************
 *  A continuous check to see if a connection is still present via serial communication.
 *********************************************************************************************************************/
bool check_keep_alive()
{/*{{{*/
    if (keepAliveCounter >= KEEP_ALIVE_TIMEOUT_MS)
    {
        /* No keep alive message have been received within the timeframe, go back to 'wait_for_start_event'. */
        keepAliveCounter = 0;
        return false;
    }

    if (Serial.available())
    {
        receivedSerial = Serial.read();

        if (receivedSerial == KEEP_ALIVE_EVENT)
        {
            keepAliveCounter = 0;
            return true;
        }
    }

    keepAliveCounter += HEARTBEAT_TIMEOUT_MS;

    return true;
}/*}}}*/


/**********************************************************************************************************************
 *  Reads the joystick sensors (horizontal and vertical data).
 *********************************************************************************************************************/
void read_joystick_sensors()
{/*{{{*/
    joystickHorizontalValue = analogRead(JOYSTICK_HORIZONTAL_PIN);
    joystickVerticalValue = analogRead(JOYSTICK_VERTICAL_PIN);
}/*}}}*/


/**********************************************************************************************************************
 *  Reads the switches.
 *********************************************************************************************************************/
void read_switches()
{/*{{{*/
    joystickSwitchState = (digitalRead(JOYSTICK_SWITCH_PIN) == 1) ? 0 : 1;
}/*}}}*/


/**********************************************************************************************************************
 *  Converts the joystick sensor values to pixel steps.
 *********************************************************************************************************************/
void convert_to_pixel_steps()
{/*{{{*/
    /* Get mouse step values. */
    horizontalPixelSteps = translate_value_to_pixel_steps(joystickHorizontalValue, joystickHorizontalNeutralPosValue);
    verticalPixelSteps = -(translate_value_to_pixel_steps(joystickVerticalValue, joystickVerticalNeutralPosValue));
}/*}}}*/


/**********************************************************************************************************************
 *  Sets the pixel steps signs (+ or -).
 *********************************************************************************************************************/
void set_pixel_steps_signs()
{/*{{{*/
    /* Set the step signs. */
    horizontalPixelStepsSign = (horizontalPixelSteps < 0) ? "" : "+";
    verticalPixelStepsSign = (verticalPixelSteps < 0) ? "" : "+";
}/*}}}*/


/**********************************************************************************************************************
 *  Transmits the steps data and switch states package via serial.
 *********************************************************************************************************************/
void transmit_data_package_frame()
{/*{{{*/
    /* 'S' indicate start, ':' seperate the values, 'E' indicate end. */
    /* Order of values:
        Horizontal pixel steps
        Veritcal pixel steps
        joystick switch state (1 = pressed) */
    String frame = "S" +
                   horizontalPixelStepsSign + ((String) horizontalPixelSteps) +
                   ":" +
                   verticalPixelStepsSign + ((String) verticalPixelSteps) +
                   ":" +
                   joystickSwitchState +
                   "E";

    /* Send package frame via serial interface. */
    Serial.print(frame);
}/*}}}*/


/**********************************************************************************************************************
 *  Translates joystick sensor value to pixel steps.
 *********************************************************************************************************************/
int translate_value_to_pixel_steps(int value, int midPos)
{/*{{{*/
    int steps = 0;
    int lowerEdge = midPos - STEP_OFFSET;
    int higherEdge = midPos + STEP_OFFSET;

    /* If not inside joystick sensor value range */
    if (value < SENSOR_MIN_VALUE || value > SENSOR_MAX_VALUE)
    {
        return 0;
    }

    for (steps = 0; steps <= (midPos / STEP_OFFSET); steps++)
    {
        if (value >= lowerEdge && value <= midPos)
        {
            return (-1 * steps);
        }
        else if (value <= higherEdge && value >= midPos)
        {
            return steps;
        }
        lowerEdge -= STEP_OFFSET;
        higherEdge += STEP_OFFSET;
    }

    return 0;
}/*}}}*/
