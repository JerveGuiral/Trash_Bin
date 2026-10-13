# CABWMS – Smart Automated Waste Separator

**Cabadbaran Waste Monitoring System.** A smart trash bin that uses an ESP32-CAM and an
Edge Impulse AI model to tell **plastic** from **biodegradable** waste and tilts its lid to drop
each item into the right compartment. An ESP32 DevKit controls the bin and serves a live web
dashboard built from our Figma design.

## How it works

```
 trash on lid ──► IR sensor (GPIO 27) ──► ESP32 DevKit ──'C'──► ESP32-CAM
                                              ▲                   │ takes photo (flash on)
                                              │                   │ runs AI model (96x96 RGB)
                                              └─ PLASTIC / BIO ◄──┘ EMPTY / UNKNOWN
                                              │
                         servo tilts lid ◄────┤ (45° plastic, 135° bio)
                                              │
                   web dashboard (Wi-Fi) ◄────┘ http://192.168.137.60/
```

1. The **IR sensor** detects an item on the lid.
2. The **DevKit** sends `C` to the **ESP32-CAM** over a serial link.
3. The camera turns on its flash, takes a photo and runs the **Edge Impulse model**.
   It answers `PLASTIC`, `BIO`, `EMPTY` or `UNKNOWN` (below 70% confidence).
4. The DevKit **tilts the servo lid** toward the matching side, then returns it to center.
5. Every event is shown live on the **CABWMS dashboard**.

## Hardware

| Component | Role | Connection |
|---|---|---|
| ESP32 DevKit (38-pin, CP2102) | Main controller, web dashboard | USB 5V |
| ESP32-CAM AI Thinker (OV3660, 4MB PSRAM) | Photo + AI classification | 5V rail, serial link to DevKit |
| ESP32-CAM-MB | USB programmer for the camera | Only used for uploading |
| IR obstacle sensor | Detects trash on the lid | OUT → GPIO 27, VCC → 3V3 |
| Servo (MG996R / MG90S) | Tilts the lid | Signal → GPIO 13, 5V rail |

### Wiring

| DevKit pin | Goes to |
|---|---|
| GPIO 13 | Servo signal (orange) |
| GPIO 27 | IR sensor OUT |
| 3V3 | IR sensor VCC |
| GPIO 16 (RX2) | ESP32-CAM IO14 (camera TX) |
| GPIO 17 (TX2) | ESP32-CAM IO15 (camera RX) |
| 5V / VIN | 5V rail → servo red + ESP32-CAM 5V |
| GND | GND rail → servo brown + ESP32-CAM GND + IR GND |

Notes from testing:
- Do **not** put the DevKit on a mini breadboard with both pin rows inserted. The rows end up
  shorted together, including the flash pins (SD2/SD3/CMD), and the board gets stuck in a boot loop.
- Use the GND pin next to GPIO 23, away from the flash pins.
- The MG996R needs its own **5V ≥2A supply**. On laptop USB power it pulls the voltage down and
  the DevKit resets. The firmware moves the servo one degree at a time and releases it when idle
  to keep the current low.

## Repository layout

| Folder | What it is |
|---|---|
| [`esp32-devkit-sorter/`](esp32-devkit-sorter) | **Main controller.** IR → camera → servo logic + CABWMS web dashboard |
| [`esp32cam-sorter/`](esp32cam-sorter) | **Camera AI firmware.** Runs the Edge Impulse model (float32) on request |
| [`esp32cam-combined/`](esp32cam-combined) | Camera **data collection** firmware: live stream, labelled capture page, Record (1 photo/s) |
| [`esp32-devkit-wiring-test/`](esp32-devkit-wiring-test) | Hardware test: servo sweep, IR readings, camera link check |
| [`esp32cam-test/`](esp32cam-test), [`esp32cam-edgeimpulse/`](esp32cam-edgeimpulse) | Early camera tests (web stream, first capture page) |
| [`tools/edge-impulse/`](tools/edge-impulse) | Scripts that upload labelled images to Edge Impulse |

## AI model (Edge Impulse)

- **Labels:** `plastic`, `biodegradable`, `background` (empty lid)
- **Data:** 850 plastic and 850 biodegradable images from the Kaggle
  [Garbage Classification (12 classes)](https://www.kaggle.com/datasets/mostafaabla/garbage-classification)
  dataset (biodegradable = biological + paper + cardboard), plus 251 background photos taken with
  our own ESP32-CAM. Split 80% training / 20% testing.
- **Impulse:** image 96×96 RGB (fit shortest axis) → Transfer Learning, MobileNetV2 96×96 0.35,
  60 training cycles, learning rate 0.0005, data augmentation on, auto-weight classes on.

| Model version | Validation accuracy | Background | Biodegradable | Plastic |
|---|---|---|---|---|
| Quantized (int8) | 61.7% | 0% | 98.7% | 45.3% |
| **Unoptimized (float32)** | **94.0%** | **100%** | **92.6%** | **93.5%** |

The int8 version collapsed to predicting "biodegradable" for almost everything, so the camera runs
the **float32** model. It takes about **10 seconds** per photo on the ESP32-CAM (the model's tensor
arena lives in PSRAM). The firmware raises `EI_MAX_OVERFLOW_BUFFER_COUNT` to 256 so the float32
model fits.

## Web dashboard

The DevKit joins the laptop's Wi-Fi hotspot and serves the dashboard at
**http://192.168.137.60/** (open it on the laptop or on a phone connected to the same hotspot).
It implements the two screens from our Figma design:

- **Main dashboard:** full bins, total bins, active bins, recent alerts, live system alerts, plus a
  **Live smart bin** panel: last item and AI result, compartment fill levels, sorted counts,
  IR/camera/servo/Wi-Fi status, and demo buttons (classify now, tilt, mark as emptied).
- **Interactive map view:** OpenStreetMap of Cabadbaran City with color-coded bin pins and a
  "Needs attention" list.

Only **BIN-CBD-001 (City Plaza)** is our real prototype. The other 11 bins are **sample data**
(marked SAMPLE) so the screens match the wireframe.

API used by the page: `GET /api/status` (live JSON), `POST /api/action?do=classify|plastic|bio|reset`.

## Build and upload

1. Install [PlatformIO](https://platformio.org/) (VS Code extension).
2. In each project, copy `src/secrets.example.h` to `src/secrets.h` and enter your Wi-Fi name and
   password (the hotspot must be **2.4 GHz**; the ESP32 cannot use 5 GHz).
3. Upload:
   - **ESP32-CAM:** put it on the ESP32-CAM-MB board, then `pio run -d esp32cam-sorter -t upload`.
   - **DevKit:** install the [CP210x driver](https://www.silabs.com/developer-tools/usb-to-uart-bridge-vcp-drivers)
     if no COM port appears, then `pio run -d esp32-devkit-sorter -t upload`.
4. Serial monitor at 115200. DevKit test commands: `p` tilt plastic, `b` tilt bio, `c` classify.

## Changes from the approved proposal

| Proposal | Current build | Reason |
|---|---|---|
| Two HC-SR04 ultrasonic sensors to measure how full each compartment is | Not installed yet. Fill level is **estimated** from the number of sorted items (10% per item) and shown as an estimate on the dashboard | Not covered by the budget yet. The dashboard already has the fill bars and 85% alerts, so the sensors can replace the estimate later |
| Blynk mobile app dashboard | Local web dashboard served by the ESP32, built from our Figma design | Matches the approved wireframe and needs no Blynk account or internet for live data |
| Quantized (int8) model | Float32 model | int8 accuracy was 61.7% vs 94.0% for float32 (see table above) |
| Servo powered from the ESP32 | Needs a separate 5V ≥2A supply for the MG996R | Laptop USB could not supply the servo current; the DevKit reset during tests |

## Known limitations and next steps

- Classification takes about 10 s per item (float32 model).
- The Kaggle photos have plain backgrounds; more photos of real items taken through the ESP32-CAM
  would make the model more reliable in the actual bin.
- Add the HC-SR04 sensors when the budget allows, and a dedicated 5V 2A power supply.
