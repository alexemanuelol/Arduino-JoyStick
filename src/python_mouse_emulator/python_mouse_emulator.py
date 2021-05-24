#!/usr/bin/env python3

import glob
import pyautogui
import re
import serial
import sys
import threading
import time

class MouseEmulator:
    """ Mouse emulator class. """

    def __init__(self):
        """ Initialize mouse emulator class. """
        # General variables
        self.connected = False
        self.baudrate = 9600
        self.regex = r"S[+-][0-9]+:[+-][0-9]+:\d:\d:\dE"

        # button states
        self.prevJoystickButtonState =  0
        self.prevLeftButtonState = 0
        self.prevRightButtonState = 0

        # pyautogui init
        pyautogui.PAUSE = 0
        pyautogui.FAILSAFE = False

        # Serial communication setup
        self.__sp = serial.Serial(
            port                = None,
            baudrate            = self.baudrate,
            bytesize            = serial.EIGHTBITS,
            parity              = serial.PARITY_NONE,
            stopbits            = serial.STOPBITS_ONE,
            timeout             = 2,
            xonxoff             = False,
            rtscts              = False,
            write_timeout       = None,
            dsrdtr              = False,
            inter_byte_timeout  = None
        )


    def start(self):
        """ Start the application. """
        while True:# {{{
            ports = self.get_ports()

            self.__sp.port = None
            for port in ports:
                if self.test_port(port):
                    self.__sp.port = port
                    break

            if self.__sp.port != None:
                self.connect_to_joystick()
                self.main_thread()
            else:
                continue

            print("Arduino joystick lost.")
            self.__sp.close()# }}}


    def get_ports(self):
        """ Returns a list of all available ports. """
        if sys.platform.startswith('win'):                                          # Windows{{{
            ports = ['COM%s' % (i + 1) for i in range(256)]
        elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'): # Linux
            ports = glob.glob('/dev/ttyUSB*')
        elif sys.platform.startswith('darwin'):                                     # Mac OS
            ports = glob.glob('/dev/tty.*')
        else:
            raise EnvironmentError('Unsupported platform')

        result = list()
        for port in ports:
            try:
                tmp = serial.Serial(port)
                tmp.close()
                result.append(port)
            except (OSError, serial.SerialException):
                if port == self.__sp.port:
                    result.append(port)

        return result# }}}


    def test_port(self, port):
        """ Test the port to see if it is the joystick. """
        try:# {{{
            self.__sp.port = port
            time.sleep(.5)

            received = ""
            for i in range(5):      # Five times required before checking, trash on the line
                self.__sp.write("0".encode())
                time.sleep(.2)
                received += self.__sp.read(self.__sp.in_waiting).decode()

            if ("HELLO" in received):
                print(f"Arduino joystick found on port: {port}")
                return True
            else:
                self.__sp.close()
                return False

        except (OSError, serial.SerialException):
            self.__sp.close()
            return False
        except (UnicodeDecodeError):
            self.__sp.close()
            return False# }}}


    def connect_to_joystick(self):
        """ Opens the serial connection to the joystick. """
        try:# {{{
            if not self.__sp.isOpen():
                self.__sp.open()
        except:
            raise Exception("Could not open port.")
        self.connected = True# }}}


    def main_thread(self):
        """ Main thread that initialize read_serial_thread and sends keep alive message to the joystick. """
        self.__sp.flush()# {{{

        # Send '1' five times to make sure that the joystick receives it (Not a nice solution).
        # '1' tells the joystick to begin sending joystick data packages.
        for i in range(5):
            self.__sp.write("1".encode())
            time.sleep(.2)
            self.__sp.read(self.__sp.in_waiting).decode()

        event = threading.Event()
        thread = threading.Thread(target=self.read_serial_thread, args=(event,))
        thread.start()

        # Every ~3 seconds, send 'keep alive' message (2)
        while self.connected:
            try:
                self.__sp.write("2".encode())
                time.sleep(3)
            except:
                self.connected = False# }}}


    def read_serial_thread(self, event):
        """ Serial communication reading thread. """
        buffer = ""# {{{
        data = ""

        try:
            while self.connected:
                data = self.__sp.read(self.__sp.in_waiting).decode()

                if data != "":
                    # Append to the buffer variable
                    buffer += data

                    # Find all package frames in the buffer
                    frames = re.findall(self.regex, buffer)

                    # Remove the found matches from the buffer
                    buffer = re.sub(self.regex, "", buffer)

                    if len(frames) != 0:
                        joystickActions = self.get_values_from_frames(frames)
                        self.execute_joystick_actions(joystickActions)

        except (OSError, serial.serialutil.SerialException) as e:
            print("Problem with the serial connection...")
        except (UnicodeDecodeError) as e:
            print("Problem with decoding the byte...")# }}}


    def get_values_from_frames(self, frames):
        """ Returns a list of value dictionaries. """
        if not isinstance(frames, list):# {{{
            raise Exception("frames needs to be of list type.")

        valueList = list()
        for frame in frames:
            valueList.append(self.get_frame_values(frame))
        return valueList# }}}


    def get_frame_values(self, frame):
        """ Returns a dictionary of the extracted values from the frame. """
        if not isinstance(frame, str):# {{{
            raise Exception("frame needs to be of str type.")

        frame = (frame[1:][:-1]).split(":")

        if len(frame) != 5:
            raise Exception("the frame was invalid.")

        frameValues = dict()
        frameValues["joyX"] = int(frame[0])
        frameValues["joyY"] = int(frame[1])
        frameValues["joySw"] = int(frame[2])
        frameValues["leftSw"] = int(frame[3])
        frameValues["rightSw"] = int(frame[4])
        return frameValues# }}}


    def execute_joystick_actions(self, joystickActions):
        """ Executes joystick actions defined in the joystickActions list. """
        for actions in joystickActions:# {{{
            self.execute_joystick_movements(actions["joyX"], actions["joyY"])
            self.execute_joystick_button_actions(actions["joySw"], actions["leftSw"], actions["rightSw"])# }}}


    def execute_joystick_movements(self, x, y):
        """ Executes the joystick movements. """
        pyautogui.move(x, y)# {{{}}}


    def execute_joystick_button_actions(self, joystickButtonState, leftButtonState, rightButtonState):
        """ Executes the actions from the button states. """
        # Joystick button
        if joystickButtonState != self.prevJoystickButtonState:
            if joystickButtonState == 1:
                pyautogui.mouseDown(button="left")
            else:
                pyautogui.mouseUp(button="left")

        # Left 'mouse' button
        if leftButtonState != self.prevLeftButtonState:
            if leftButtonState == 1:
                pyautogui.mouseDown(button="left")
            else:
                pyautogui.mouseUp(button="left")

        # Right 'mouse' button
        if rightButtonState != self.prevRightButtonState:
            if rightButtonState == 1:
                pyautogui.mouseDown(button="right")
            else:
                pyautogui.mouseUp(button="right")

        # Set the previous state
        self.prevJoystickButtonState = joystickButtonState
        self.prevLeftButtonState = leftButtonState
        self.prevRightButtonState = rightButtonState



if __name__ == "__main__":
    me = MouseEmulator()
    me.start()
