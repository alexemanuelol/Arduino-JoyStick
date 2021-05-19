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

#define SENSOR_MIN_VALUE    0
#define SENSOR_MAX_VALUE    1024
#define MOUSE_STEP_OFFSET   100


/**************************
 *  Includes
 *************************/


/**************************
 *  Global variables
 *************************/

int horizontalPin = A7;                 /* Analog input pin for horizontal joystick value. */
int verticalPin = A6;                   /* Analog input pin for vertical joystick value. */
int switchPin = 9;                      /* Switch pin for the joystick. */

int horizontalMidPos;                   /* The middle or natural position of the joystick horizontally. */
int verticalMidPos;                     /* The middle or natural position of the joystick vertically. */

int hStep;                              /* Stores the current horizontal step value. */
int vStep;                              /* Stores the current vertical step value. */
String hStepSign;                       /* Stores the current step value sign. */
String vStepSign;                       /* Stores the current step value sign. */


/**********************************************************************************************************************
 *  Setup routine
 *********************************************************************************************************************/
void setup()
{
    Serial.begin(9600);                 /* Start serial monitor. */

    /* Set analog pins as inputs (reads from the joystick sensor). */
    pinMode(horizontalPin, INPUT);
    pinMode(verticalPin, INPUT);

    pinMode(switchPin, INPUT);          /* Set the switch pin as input. */
    digitalWrite(switchPin, HIGH);      /* Pull switch pin high. */
    delay(500);                         /* Short delay to let outputs settle */

    /* Initial joystick values (Joystick should be in neutral position when reading these). */
    horizontalMidPos = analogRead(horizontalPin);
    verticalMidPos = analogRead(verticalPin);
}


/**********************************************************************************************************************
 *  Main program loop
 *********************************************************************************************************************/
void loop()
{
    /* Get mouse step values. */
    hStep = translate_to_steps(analogRead(horizontalPin), horizontalMidPos);
    vStep = translate_to_steps(analogRead(verticalPin), verticalMidPos);

    /* Set the step signs. */
    hStepSign = (hStep < 0) ? "" : "+";
    vStepSign = (vStep < 0) ? "" : "+";

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
