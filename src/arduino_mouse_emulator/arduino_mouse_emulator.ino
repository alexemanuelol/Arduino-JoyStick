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

int horizontalValue;                    /* Stores the current horizontal value. */
int verticalValue;                      /* Stores the current vertical value. */


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

    /* Initial joystick values (Hoystick should be in neutral position when reading these). */
    horizontalMidPos = analogRead(horizontalPin);
    verticalMidPos = analogRead(verticalPin);
}


/**********************************************************************************************************************
 *  Main program loop
 *********************************************************************************************************************/
void loop()
{
    /* Get joystick values. */
    horizontalValue = analogRead(horizontalPin);    /* Read the horizontal joystick value. */
    verticalValue = analogRead(verticalPin);        /* Read the vertical joystick value. */

    Serial.print("x: ");
    Serial.print(horizontalValue);
    Serial.print("  y: ");
    Serial.println(verticalValue);
}
