# Output example

## 1. Live data
`7476 ms | -20, 71, 980 mG | -5092, 3675, -1085 mdps`

`7486 ms | -20, 61, 988 mG | -2887, 2607, -1120 mdps`

`7496 ms | -20, 62, 993 mG | -2520, 875, -245 mdps`
- 1st column: measurement moment (tick, ms)
- 2nd column: proper acceleration (accelerometer, mG)
- 3rd column: angular velocity (gyroscope, mdps)

## 2. Free fall phase detection
- ~300 magnitude = free fall (mG)
- Detects the start && end of the free fall, duration, distance
    - `>>> Freefall start at 8486 ms`
    - `>>> Freefall end at 8576 ms, duration 90 ms, distance 39 mm`

### Environment
- IDE: STM32CubeIDE
- Firmware version: STM32F411E-Discovery_v1.28.0
- Board: STM32F411E-Discovery (reference VMB1115B)
