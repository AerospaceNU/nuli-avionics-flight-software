# Puts an Adafruit SAMD board into its bootloader with a 1200-baud touch and prints the
# bootloader's COM port. PlatformIO's own touch (opening straight at 1200) isn't reliably
# picked up by these boards; opening at 115200 with DTR set, then dropping to 1200 is.
# usage: touch.py PORT
import sys, time
import serial
import serial.tools.list_ports as lp

BOOTLOADER_PIDS = {0x000B, 0x0015, 0x0022}  # Feather M0 (SillyGoose) and M4 (SeriousGoose) UF2 bootloaders
port = sys.argv[1]
for attempt in range(6):
    try:
        s = serial.Serial(port, 115200)
        s.dtr = True
        time.sleep(0.2)
        s.baudrate = 1200
        s.dtr = False
        time.sleep(0.3)
        s.close()
    except serial.SerialException:
        pass  # already gone - the board may have reset on an earlier attempt
    for _ in range(10):
        time.sleep(0.5)
        boot = [p.device for p in lp.comports() if p.vid == 0x239A and p.pid in BOOTLOADER_PIDS]
        if boot:
            print(boot[0])
            sys.exit(0)
print("no bootloader port appeared", file=sys.stderr)
sys.exit(1)
