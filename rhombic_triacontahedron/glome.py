#!/usr/bin/env python3
"""
Orb glome animation - port of rhombic_triacontahedron.pde
Slow hue cycle, calm mode. Relies on Fadecandy hardware interpolation.
"""

import socket
import time
import random
import colorsys
import struct

HOST = '127.0.0.1'
PORT = 7890
NUM_PIXELS = 30

# calm mode
SATURATION = 70 / 100.0
MIN_BRIGHTNESS = 30 / 100.0
MAX_BRIGHTNESS = 100 / 100.0
PERCENT_ON = 60 / 100.0
FRAME_RATE = 0.5  # fps — original sketch rate; hardware interpolation smooths between frames


def make_opc_packet(pixels):
    """Build an OPC Set Pixel Colors packet for channel 0."""
    data = b''.join(
        bytes([min(255, max(0, int(r * 255))),
               min(255, max(0, int(g * 255))),
               min(255, max(0, int(b * 255)))])
        for r, g, b in pixels
    )
    header = struct.pack('>BBH', 0, 0, len(data))
    return header + data


def make_firmware_config_packet(config_byte):
    """Fadecandy firmware config sysex."""
    return bytes([0x00, 0xFF, 0x00, 0x05, 0x00, 0x01, 0x00, 0x02, config_byte])


def connect():
    while True:
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect((HOST, PORT))
            s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            # All defaults: dithering ON, interpolation ON
            s.sendall(make_firmware_config_packet(0x00))
            time.sleep(0.5)
            print("Connected to OPC server")
            return s
        except Exception as e:
            print(f"Waiting for OPC server... ({e})")
            time.sleep(1)


def main():
    sock = connect()
    start = time.time()
    frame_interval = 1.0 / FRAME_RATE

    while True:
        frame_start = time.time()
        t = frame_start - start

        # All pixels share one hue (original sketch: i * 2/30 = 0 in Java integer division)
        hue = (t * 1.0) % 100 / 100.0

        pixels = []
        for i in range(NUM_PIXELS):
            on = random.random() < PERCENT_ON
            brightness = random.uniform(MIN_BRIGHTNESS, MAX_BRIGHTNESS) if on else 0.0
            r, g, b = colorsys.hsv_to_rgb(hue, SATURATION, brightness)
            pixels.append((r, g, b))

        pixels[0] = (0, 0, 0)  # pixel 0 not attached to a panel

        packet = make_opc_packet(pixels)
        try:
            sock.sendall(packet)
        except Exception as e:
            print(f"Disconnected: {e}")
            sock = connect()

        elapsed = time.time() - frame_start
        time.sleep(max(0, frame_interval - elapsed))


if __name__ == '__main__':
    main()
