# Rhombic Triacontahedron (Orb)

30 WS2811 LEDs on a Fadecandy board, arranged in a rhombic triacontahedron.

## Prerequisites

```
sudo apt install libusb-1.0-0-dev rapidjson-dev
```

## Build fcserver

```
cd server
make
```

## Run

1. Start the Fadecandy server (requires root for USB access):

```
sudo ./server/fcserver rhombic_triacontahedron/config.json
```

2. In another terminal, start the animation:

```
python3 rhombic_triacontahedron/glome.py
```

The orb will slowly cycle through hues with individual pixels fading on and off. The Fadecandy's hardware interpolation smoothly blends between frames.

## Configuration

`config.json` maps OPC channel 0, pixels 0-29 to the Fadecandy output. Gamma is set to 2.5.

`glome.py` has calm and party mode parameters at the top of the file:

- `SATURATION` -- color saturation (0-1)
- `MIN_BRIGHTNESS` / `MAX_BRIGHTNESS` -- brightness range for lit pixels
- `PERCENT_ON` -- fraction of pixels lit each frame
- `FRAME_RATE` -- frames per second (0.5 = one frame every 2s, hardware interpolation fills in)

## Original Processing sketch

The original `rhombic_triacontahedron.pde` + `OPC.pde` Processing sketch is still included. `glome.py` is a headless Python port that doesn't require a display.
