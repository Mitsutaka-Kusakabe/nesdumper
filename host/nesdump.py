#!/usr/bin/env python3
"""
NES Dumper CLI for Arduino Mega 2560
Reads PRG and CHR ROMs and outputs standard .nes (iNES format) files.
"""

import argparse
import sys
import time
import serial

def read_exact(ser, total_bytes):
    data = bytearray()
    start_time = time.time()
    while len(data) < total_bytes:
        chunk = ser.read(total_bytes - len(data))
        if chunk:
            data.extend(chunk)
            print(f"\r  Received: {len(data)} / {total_bytes} bytes ({len(data)*100//total_bytes}%)", end="", flush=True)
        elif time.time() - start_time > 10.0:
            raise TimeoutError(f"Timeout waiting for binary data (got {len(data)}/{total_bytes})")
    print()
    return bytes(data)

def main():
    parser = argparse.ArgumentParser(description="NES ROM Dumper via Arduino Mega 2560")
    parser.add_argument("port", help="Serial port (e.g. COM3 or /dev/ttyACM0)")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate (default: 115200)")
    parser.add_argument("--prg", type=int, default=16, help="PRG size in KB (16 or 32, default: 16)")
    parser.add_argument("--chr", type=int, default=8, help="CHR size in KB (default: 8)")
    parser.add_argument("--mirroring", choices=["vertical", "horizontal"], default="vertical", help="Mirroring type")
    parser.add_argument("--mapper", type=int, default=0, help="iNES Mapper number (default: 0)")
    parser.add_argument("--test", action="store_true", help="Run bus test only and exit")
    parser.add_argument("-o", "--out", default="dump.nes", help="Output .nes file name")
    args = parser.parse_args()

    print(f"Connecting to {args.port} at {args.baud} baud...")
    ser = serial.Serial(args.port, args.baud, timeout=2.0)
    time.sleep(2.0)  # Wait for Arduino reset

    ser.reset_input_buffer()
    ser.write(b"PING\n")
    resp = ser.readline().decode("latin-1", errors="ignore").strip()
    print(f"Handshake response: {resp}")

    if args.test:
        print("\n--- Running Bus Test ---")
        ser.write(b"TEST\n")
        while True:
            line = ser.readline().decode("latin-1", errors="ignore").strip()
            if not line:
                break
            print(line)
            if "TEST COMPLETE" in line:
                break
        ser.close()
        return

    # Calculate PRG parameters
    prg_bytes = args.prg * 1024
    chr_bytes = args.chr * 1024
    prg_start = 0xC000 if args.prg == 16 else 0x8000

    print(f"\nDumping PRG-ROM ({args.prg} KB from 0x{prg_start:04X})...")
    cmd = f"DUMP_PRG {prg_start:04X} {prg_bytes:04X}\n".encode("ascii")
    ser.write(cmd)

    # Read until PRG_START
    while True:
        line = ser.readline().decode("latin-1", errors="ignore").strip()
        if "PRG_START" in line:
            break

    prg_data = read_exact(ser, prg_bytes)
    ser.readline()  # PRG_END

    chr_data = bytearray()
    if chr_bytes > 0:
        num_banks = max(1, chr_bytes // 8192)
        print(f"\nDumping CHR-ROM ({args.chr} KB across {num_banks} bank(s))...")

        for b in range(num_banks):
            if num_banks > 1 or args.mapper == 3:
                # Switch CNROM bank via P 8000 <bank>
                ser.write(f"P 8000 {b:02X}\n".encode("ascii"))
                time.sleep(0.04)
                ser.readline()  # drain WROK

            ser.write(b"C 0000 2000\n")
            # Wait for OK 2000
            while True:
                line = ser.readline().decode("latin-1", errors="ignore").strip()
                if "OK" in line:
                    break
            bank_chunk = read_exact(ser, 8192)
            chr_data.extend(bank_chunk)
            crc_line = ser.readline().decode("latin-1", errors="ignore").strip()
            print(f"  Bank {b} done ({crc_line})")

    # Generate iNES Header (16 bytes)
    header = bytearray(16)
    header[0:4] = b"NES\x1a"
    header[4] = args.prg // 16
    header[5] = args.chr // 8
    mirror_bit = 1 if args.mirroring == "vertical" else 0
    header[6] = ((args.mapper & 0x0F) << 4) | mirror_bit
    header[7] = (args.mapper & 0xF0)

    # Save .nes
    with open(args.out, "wb") as f:
        f.write(header)
        f.write(prg_data)
        f.write(chr_data)

    print(f"\n[SUCCESS] Successfully saved {len(header) + len(prg_data) + len(chr_data)} bytes to {args.out}!")
    ser.close()

if __name__ == "__main__":
    main()
