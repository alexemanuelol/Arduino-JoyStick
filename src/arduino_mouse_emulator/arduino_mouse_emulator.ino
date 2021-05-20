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

#define SENSOR_MIN_VALUE            0
#define SENSOR_MAX_VALUE            1024
#define MOUSE_STEP_OFFSET           100

#define SENSOR_READING_TIMEOUT_MS   10
#define KEEP_ALIVE_TIMEOUT_MS       10000

#define START_MESSAGE               "0x1"
#define KEEP_ALIVE_MESSAGE          "0x2"


/**************************
 *  Includes
 *************************/


/**************************
 *  Global variables
 *************************/

int hPin = A7;              /* Analog input pin for horizontal joystick value. */
int vPin = A6;              /* Analog input pin for vertical joystick value. */
int swPin = 9;              /* Switch pin for the joystick. */

int hMidPosValue;           /* The middle or neutral position value of the joystick horizontally. */
int vMidPosValue;           /* The middle or neutral position value of the joystick vertically. */
int hValue;                 /* The joystick horizontal value. */
int vValue;                 /* The joystick vertical value. */

int hStep;                  /* Stores the current horizontal step value. */
int vStep;                  /* Stores the current vertical step value. */
String hStepSign;           /* Stores the current horizontal step value sign. */
String vStepSign;           /* Stores the current vertical step value sign. */

String keepAliveReceived;
uint16_t keepAliveCounter = 0;


/**********************************************************************************************************************
 *  Setup routine
 *********************************************************************************************************************/
void setup()
{
    Serial.begin(9600);                 /* Start serial monitor. */

    /* Set analog pins as inputs (reads from the joystick sensor). */
    pinMode(hPin, INPUT);
    pinMode(vPin, INPUT);

    pinMode(swPin, INPUT);              /* Set the switch pin as input. */
    digitalWrite(swPin, HIGH);          /* Pull switch pin high. */
    delay(500);                         /* Short delay to let outputs settle */

    /* Initial joystick values (Joystick should be in neutral position when reading these). */
    hMidPosValue = analogRead(hPin);
    vMidPosValue = analogRead(vPin);
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
    String received;

    while (true)
    {
        if (Serial.available())
        {
            received = Serial.readString();

            if (find_substring(received, (String) START_MESSAGE))
            {
                /* Start message received, let's start sensor reading and transmittion! */
                break;
            }
        }
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
        keepAliveReceived = Serial.readString();

        if (find_substring(keepAliveReceived, (String) KEEP_ALIVE_MESSAGE))
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
    hValue = analogRead(hPin);
    vValue = analogRead(vPin);
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
