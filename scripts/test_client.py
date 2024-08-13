'''
This file is part of SimMeR, an educational mechatronics robotics simulator.
Initial development funded by the University of Toronto MIE Department.
Copyright (C) 2023  Ian G. Bennett

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
'''

# Basic echo client, for testing purposes
# Code modified from examples on https://realpython.com/python-sockets/
# and https://www.geeksforgeeks.org/python-display-text-to-pygame-window/

import socket
import struct
import time
from datetime import datetime
import serial

# Wrapper functions
def transmit(data):
    '''Selects whether to use serial or tcp for transmitting.'''
    if SIMULATE:
        transmit_tcp(data)
    else:
        transmit_serial(data)
    time.sleep(TRANSMIT_PAUSE)

def receive():
    '''Selects whether to use serial or tcp for receiving.'''
    if SIMULATE:
        return receive_tcp()
    else:
        return receive_serial()

# TCP communication functions
def transmit_tcp(data):
    '''Send a command over the TCP connection.'''
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        try:
            s.connect((HOST, PORT_TX))
            s.send(data.encode('utf-8'))
        except (ConnectionRefusedError, ConnectionResetError):
            print('Tx Connection was refused or reset.')
        except TimeoutError:
            print('Tx socket timed out.')
        except EOFError:
            print('\nKeyboardInterrupt triggered. Closing...')

def receive_tcp():
    '''Receive a reply over the TCP connection.'''
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s2:
        try:
            s2.connect((HOST, PORT_RX))
            response_raw = s2.recv(1024)
            if response_raw:
                # return the data received as well as the current time
                return [depacketize(response_raw), datetime.now().strftime("%H:%M:%S")]
            else:
                return [[False], None]
        except (ConnectionRefusedError, ConnectionResetError):
            print('Rx connection was refused or reset.')
        except TimeoutError:
            print('Response not received from robot.')

# Serial communication functions
def transmit_serial(data):
    '''Transmit a command over a serial connection.'''
    SER.write(data.encode('ascii'))

def receive_serial():
    '''Receive a reply over a serial connection.'''
    # If responses are ascii characters, use this
    response_raw = (SER.readline().strip().decode('ascii'),)

    # If responses are a series of 4-byte floats, use this
    available_bytes = SER.in_waiting

    # If response received, return it
    if response_raw[0]:
        return [depacketize(response_raw), datetime.now().strftime("%H:%M:%S")]
    else:
        return [[False], datetime.now().strftime("%H:%M:%S")]

def clear_serial(delay_time):
    '''Wait some time (delay_time) and then clear the serial buffer.'''
    time.sleep(delay_time)
    SER.read(SER.in_waiting)

def depacketize(data_raw: str):
    '''
    Take a raw string received and verify that it's a complete packet, returning just the data messages in a list.
    '''

    start = data_raw.find('\x02')
    end = data_raw.find('\x03')

    # Check that the start and end framing characters are present
    if (start >= 0 and end >= start):
        data = []
        # Find all instances of ',' in the string
        # If there are any, circle through it with a for loop to break out each data segment and append it to data
        # If there are none, return the whole message
        return data_raw[start+1:end]
    else:
        return False

def packetize(data: str):
    '''
    Take a message that is to be sent to the command script and packetize it with start and end framing.
    '''

    # Check to make sure that a packet doesn't include any forbidden characters (0x01, 0x02, 0x03, 0x04)
    forbidden = ['\x02', '\x03']
    check_fail = any(char in data for char in forbidden)

    if not check_fail:
        return '\x02' + data + '\x03'

    return False

# Set whether to use TCP (SimMeR) or serial (Arduino)
SIMULATE = True

# Pause time
TRANSMIT_PAUSE = 0.25
if SIMULATE:
    TRANSMIT_PAUSE = 0.1

### Network Setup ###
HOST = '127.0.0.1'      # The server's hostname or IP address
PORT_TX = 61200         # The port used by the *CLIENT* to receive
PORT_RX = 61201         # The port used by the *CLIENT* to send data

### Serial Setup ###
BAUDRATE = 9600         # Baudrate in bps
PORT_SERIAL = 'COM4'    # COM port identification
try:
    SER = serial.Serial(PORT_SERIAL, BAUDRATE, timeout=0)
except serial.SerialException:
    pass

# Source
SOURCE = 'serial device ' + PORT_SERIAL
if SIMULATE:
    SOURCE = 'SimMeR'

# Main loop
RUNNING = True
while RUNNING:
    cmd = input('Type in a string to send: ')
    transmit(packetize(cmd))
    [responses, time_rx] = receive()
    responses = [depacketize(item) for item in responses]
    print(f"At time '{time_rx}' received '{round(responses[0], 3)}' from {SOURCE}\n")
