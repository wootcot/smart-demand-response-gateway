# Hardware - Smart Demand Response Gateway

## Directory Structure

```
hardware/
├── .gitignore                         # Prevents tracking .lck and -bak files
├── README.md                          # This file
│
├── design-files/                      # Raw, editable KiCad CAD workspace
│   ├── gateway.kicad_pro              # KiCad 8.x/10.x project file
│   ├── gateway.kicad_sch              # Root schematic (hierarchical top-level)
│   ├── gateway.kicad_pcb              # Physical PCB board layout (2-layer)
│   ├── ct_sensor.kicad_sch            # SCT-013 CT interface + ADS1115 16-bit ADC (I2C)
│   ├── esp32_module.kicad_sch         # ESP32-WROOM-32 MCU (I2C master, GPIO22/21)
│   ├── power_supply.kicad_sch         # 230VAC → 5V/3.3V DC power supply
│   ├── relay_driver.kicad_sch         # Opto-isolated relay driver (PC817 + NPN)
│   ├── sym-lib-table                  # Symbol library references
│   └── fp-lib-table                   # Footprint library references
│
├── documentation/                     # High-level viewing artifacts for reviews
│   ├── schematic_v1.0.pdf             # Combined schematics plotted to PDF
│   ├── pcb_top_render.png             # 3D render view of assembled board
│   └── BOM.csv                        # Component procurement list
│
└── fabrication/                       # Production-ready manufacturing outputs
    └── gateway_gerbers_v1.0.zip       # Gerber + drill files for PCB fab
```

## Opening the Project

1. Install [KiCad 10.x](https://www.kicad.org/download/)
2. Open KiCad → File → Open Project
3. Navigate to `hardware/design-files/gateway.kicad_pro`

## Schematic Hierarchy

The design uses hierarchical sheets for clarity and modularity:

```
gateway.kicad_sch (Root)
├── Power Supply        → power_supply.kicad_sch
├── CT Sensor Interface → ct_sensor.kicad_sch  (includes ADS1115 ADC)
├── Relay Driver        → relay_driver.kicad_sch
└── ESP32 Module        → esp32_module.kicad_sch (I2C master to ADS1115)
```

## PCB Specifications

| Parameter        | Value                                        |
| ---------------- | -------------------------------------------- |
| Board dimensions | 100mm × 70mm                                 |
| Layer count      | 2 (F.Cu + B.Cu)                              |
| Substrate        | FR4, 1.6mm thickness                         |
| Copper weight    | 1 oz (35μm)                                  |
| Surface finish   | HASL (lead-free)                             |
| Min track width  | 0.2mm (signal), 0.5mm (power)                |
| Min clearance    | 0.2mm (signal), 0.5mm (power)                |
| Via size         | 0.6mm dia / 0.3mm drill                      |
| Isolation gap    | >6mm between AC and DC domains (IEC 60950-1) |

## Design Zones

The PCB is divided into two distinct domains separated by a >6mm creepage barrier:

- **AC Domain (left)**: Fuse, TVS, HLK-PM01, relay contacts, terminal blocks
- **DC Domain (right)**: ESP32, CT sensor interface, optocoupler logic side, LEDs

Ground planes (GND fill) cover the DC domain on both layers. The AC domain uses dedicated traces with no ground pour to maintain isolation.

## Bill of Materials (Key Components)

| Ref | Component      | Value        | Purpose                     |
| --- | -------------- | ------------ | --------------------------- |
| U1  | HLK-PM01       | 230VAC→5VDC  | AC-DC converter             |
| U2  | AMS1117-3.3    | 3.3V LDO     | 5V→3.3V regulator           |
| U3  | PC817          | Optocoupler  | Galvanic isolation          |
| U4  | ESP32-WROOM-32 | MCU          | Edge controller             |
| U5  | ADS1115IDGS    | 16-bit ADC   | CT sensor I2C ADC (0x48)    |
| Q1  | BC547B         | NPN          | Relay coil driver           |
| K1  | SRD-05VDC-SL-C | 5V relay     | 230V load switching         |
| F1  | Fuse           | 5A fast-blow | Overcurrent protection      |
| D1  | P6KE250CA      | TVS bidir.   | AC transient protection     |
| D5  | P6KE250CA      | TVS bidir.   | Relay arc suppression       |
| D8  | PESD3V3L1BA    | TVS          | 3.3V ESD protection         |
| J2  | 3.5mm jack     | Audio        | SCT-013 CT sensor connector |
| R1  | Burden         | 33Ω 1%       | CT sensor load (30A→1V)     |
| R2  | Bias high      | 10kΩ 1%      | VCC/2 midpoint divider      |
| R3  | Bias low       | 10kΩ 1%      | VCC/2 midpoint divider      |

Full BOM with supplier info: [`documentation/BOM.csv`](documentation/BOM.csv)

## Generating Fabrication Outputs

From the repository root:

```bash
make hw-all              # Export Gerbers + drill + schematic PDF
make hw-gerbers          # Export Gerber files only
make hw-drill            # Export drill files only
make hw-schematic-pdf    # Export combined schematic PDF
make hw-clean            # Remove generated outputs
```

Or directly from the `hardware/` directory:

```bash
make all
```

## Safety Notes

- **Isolation**: PC817 optocoupler provides >3.75kV galvanic isolation
- **Creepage**: >6mm between AC and DC copper on PCB per IEC 60950-1
- **Protection**: Fused input, TVS on AC and relay contacts, ESD on 3.3V rail
- **Ground**: Separate ground planes, star-ground topology at single point
