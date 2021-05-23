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

#define JOYSTICK_HORIZONTAL_PIN     A7          /* Analog input pin for horizontal joystick value. */
#define JOYSTICK_VERTICAL_PIN       A6          /* Analog input pin for vertical joystick value. */
#define JOYSTICK_SWITCH_PIN         9           /* Joystick switch pin. */
#define LED_GREEN_PIN               6           /* Green LED pin. */
#define LED_RED_PIN                 5           /* Red LED pin. */

#define SENSOR_MIN_VALUE            0
#define SENSOR_MAX_VALUE            1024
#define MOUSE_STEP_OFFSET           100

#define SENSOR_READING_TIMEOUT_MS   10
#define KEEP_ALIVE_TIMEOUT_MS       10000

#define HELLO_MESSAGE               '0'
#define START_MESSAGE               '1'
#define KEEP_ALIVE_MESSAGE          '2'


/**************************
 *  Includes
 *************************/


/**************************
 *  Global variables
 *************************/

int hMidPosValue;           /* The middle or neutral position value of the joystick horizontally. */
int vMidPosValue;           /* The middle or neutral position value of the joystick vertically. */
int hValue;                 /* The joystick horizontal value. */
int vValue;                 /* The joystick vertical value. */

int hStep;                  /* Stores the current horizontal step value. */
int vStep;                  /* Stores the current vertical step value. */
String hStepSign;           /* Stores the current horizontal step value sign. */
String vStepSign;           /* Stores the current vertical step value sign. */

char keepAliveReceived;
uint16_t keepAliveCounter = 0;


/**********************************************************************************************************************
 *  Setup routine
 *********************************************************************************************************************/
void setup()
{
    Serial.begin(9600);                 /* Start serial monitor. */

    /* Set analog pins as inputs (reads from the joystick sensor). */
    pinMode(JOYSTICK_HORIZONTAL_PIN, INPUT);
    pinMode(JOYSTICK_VERTICAL_PIN, INPUT);

    /* LEDs */
    pinMode(LED_GREEN_PIN, OUTPUT);
    pinMode(LED_RED_PIN, OUTPUT);

    pinMode(JOYSTICK_SWITCH_PIN, INPUT);              /* Set the switch pin as input. */
    digitalWrite(JOYSTICK_SWITCH_PIN, HIGH);          /* Pull switch pin high. */

    delay(500);                         /* Short delay to let outputs settle */

    /* Initial joystick values (Joystick should be in neutral position when reading these). */
    hMidPosValue = analogRead(JOYSTICK_HORIZONTAL_PIN);
    vMidPosValue = analogRead(JOYSTICK_VERTICAL_PIN);
}


/**********************************************************************************************************************
 *  Main program loop
 *********************************************************************************************************************/
void loop()
{
    /* Wait for a start message from the script. */
    wait_for_start();

    while (true)
    {
        if (!check_keep_alive())
        {
            break;
        }

        read_joystick_sensor();
        calculate_steps();
        set_step_signs();
        tx_data_package();
        delay(SENSOR_READING_TIMEOUT_MS);
    }
}


/**********************************************************************************************************************
 *  Wait for a start message from the script.
 *********************************************************************************************************************/
void wait_for_start()
{
    char received;

    set_joystick_active_leds(false);

    while (true)
    {
        if (Serial.available())
        {
            received = Serial.read();

            if (received == HELLO_MESSAGE)
            {
                Serial.print("OK");
                continue;
            }
            else if (received == START_MESSAGE)
            {
                /* Start message received, let's start sensor reading and transmittion! */
                break;
            }
        }
    }

    set_joystick_active_leds(true);
}


/**********************************************************************************************************************
 *  Alternate the green and red LEDs. True turns on green and turns off red, false turns off green and turns on red.
 *********************************************************************************************************************/
void set_joystick_active_leds(bool active)
{
    if (active)
    {
        digitalWrite(LED_GREEN_PIN, HIGH);
        digitalWrite(LED_RED_PIN, LOW);
    }
    else
    {
        digitalWrite(LED_GREEN_PIN, LOW);
        digitalWrite(LED_RED_PIN, HIGH);
    }
}


/**********************************************************************************************************************
 *  A continuous check to see if a connection is still present between arduino and script.
 *********************************************************************************************************************/
bool check_keep_alive()
{
    if (keepAliveCounter >= KEEP_ALIVE_TIMEOUT_MS)
    {
        keepAliveCounter = 0;
        /* No keep alive message have been received within the timeframe, go back to 'wait_for_start'. */
        return false;
    }

    if (Serial.available())
    {
        keepAliveReceived = Serial.read();

        if (keepAliveReceived == KEEP_ALIVE_MESSAGE)
        {
            keepAliveCounter = 0;
            return true;
        }
    }

    keepAliveCounter += SENSOR_READING_TIMEOUT_MS;

    return true;
}


/**********************************************************************************************************************
 *  Reads the joystick sensor (horizontal and vertical data).
 *********************************************************************************************************************/
void read_joystick_sensor()
{
    hValue = analogRead(JOYSTICK_HORIZONTAL_PIN);
    vValue = analogRead(JOYSTICK_VERTICAL_PIN);
}


/**********************************************************************************************************************
 *  Calculates the steps based on the joystick sensor values.
 *********************************************************************************************************************/
void calculate_steps()
{
    /* Get mouse step values. */
    hStep = translate_to_steps(hValue, hMidPosValue);
    vStep = translate_to_steps(vValue, vMidPosValue);
}


/**********************************************************************************************************************
 *  Sets the steps signs (+ or -).
 *********************************************************************************************************************/
void set_step_signs()
{
    /* Set the step signs. */
    hStepSign = (hStep < 0) ? "" : "+";
    vStepSign = (vStep < 0) ? "" : "+";
}


/**********************************************************************************************************************
 *  Transmits the steps data package via serial.
 *********************************************************************************************************************/
void tx_data_package()
{
    /* 'S' indicate start, ':' seperate the step values, 'E' indicate end. */
    String str = "S" + hStepSign + ((String) hStep) + ":" + vStepSign + ((String) vStep) + "E";

    /* Send mouse steps via serial interface. */
    Serial.print(str);
}


/**********************************************************************************************************************
 *  Translates joystick sensor values to mouse steps
 *********************************************************************************************************************/
int translate_to_steps(int value, int midPos)
{
    int steps = 0;
    int lowerEdge = midPos - MOUSE_STEP_OFFSET;
    int higherEdge = midPos + MOUSE_STEP_OFFSET;

    /* If not inside joystick sensor value range */
    if (value < SENSOR_MIN_VALUE || value > SENSOR_MAX_VALUE)
    {
        return 0;
    }

    for (steps = 0; steps <= (midPos / MOUSE_STEP_OFFSET); steps++)
    {
        if (value >= lowerEdge && value <= midPos)
        {
            return (-1 * steps);
        }
        else if (value <= higherEdge && value >= midPos)
        {
            return steps;
        }
        lowerEdge -= MOUSE_STEP_OFFSET;
        higherEdge += MOUSE_STEP_OFFSET;
    }

    return 0;
}


/**********************************************************************************************************************
 *  Find a substring withing a string.
 *********************************************************************************************************************/
bool find_substring(String str, String substr)
{
    int strLength = str.length();
    int substrLength = substr.length();

    if (substrLength > strLength)
    {
        return false;
    }

    int i, j;
    for (i = 0, j = 0; i != strLength; i++)
    {
        if (str[i] == substr[j])
        {
            if ((j + 1) == substrLength)
            {
                return true;
            }
            j++;
        }
        else
        {
            j = 0;
        }
    }

    if ((j + 1) == substrLength)
    {
        return true;
    }
    else
    {
        return false;
    }
}
