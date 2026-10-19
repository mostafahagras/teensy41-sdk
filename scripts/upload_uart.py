#!/usr/bin/env python3
"""Upload a Teensy 4.1 SDK .bin image over UART1."""

from __future__ import annotations

import argparse
import os
import select
import struct
import sys
import termios
import time
import tty
import zlib


MAGIC = b"TUF1"
VERSION = 1
HEADER = struct.Struct("<4sBBIIH")
CRC = struct.Struct("<I")
MAX_PAYLOAD = 256
CHUNK_SIZE = 4096
ACK_TIMEOUT = 12.0
HELLO_RETRIES = 8

BEGIN = 1
END = 3
ABORT = 4
RAW_CHUNK = 10
HELLO = 0x81
READY = 0x82
ACK = 0x83
NAK = 0x84
DONE = 0x85
ERROR = 0xFF
UPDATE_REQUEST = b"\x55"

REMOTE_ERROR_LABELS = {
    1: "bad request",
    2: "invalid image",
    3: "image does not fit",
    4: "flash erase failed",
    5: "update aborted",
    6: "UART receive timed out",
    0x10: "invalid flash program range",
    0x11: "flash write-enable command failed",
    0x12: "flash page-program command failed",
    0x13: "flash stayed busy after page programming",
    0x14: "flash page readback mismatch",
    0x15: "flash protection check failed",
}

PROTECTION_REASONS = {
    1: "could not read flash configuration",
    2: "unsupported flash protection mode",
    3: "unsupported flash device",
    4: "staging range is protected",
    5: "new image range is protected",
    6: "active image range is protected",
}


class UploadError(RuntimeError):
    pass


def describe_remote_error(payload: bytes) -> str:
    if len(payload) < 4:
        return "invalid error response from device"
    code = struct.unpack("<I", payload[:4])[0]
    if code == 0x15 and len(payload) == 8:
        reason = struct.unpack("<I", payload[4:])[0]
        detail = PROTECTION_REASONS.get(reason, f"reason {reason}")
        return f"flash protection check failed: {detail}"
    if code == 0x14 and len(payload) == 16:
        _, offset, expected, actual = struct.unpack("<IIII", payload)
        return (f"flash page readback mismatch at +0x{offset:x}: "
                f"expected 0x{expected:08x}, got 0x{actual:08x}")
    label = REMOTE_ERROR_LABELS.get(code, "device update error")
    return f"{label} (remote error 0x{code:02x})"


def configure_port(path: str, baudrate: int) -> int:
    baud_constants = {
        9600: termios.B9600,
        19200: termios.B19200,
        38400: termios.B38400,
        57600: termios.B57600,
        115200: termios.B115200,
        230400: termios.B230400,
    }
    if baudrate not in baud_constants:
        raise UploadError(f"unsupported baud rate {baudrate}")
    fd = os.open(path, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    tty.setraw(fd)
    attributes = termios.tcgetattr(fd)
    attributes[2] |= termios.CLOCAL | termios.CREAD | termios.CS8
    attributes[2] &= ~(termios.PARENB | termios.CSTOPB | termios.CSIZE)
    attributes[2] |= termios.CS8
    attributes[4] = baud_constants[baudrate]
    attributes[5] = baud_constants[baudrate]
    attributes[6][termios.VMIN] = 0
    attributes[6][termios.VTIME] = 0
    termios.tcsetattr(fd, termios.TCSANOW, attributes)
    termios.tcflush(fd, termios.TCIOFLUSH)
    return fd


def write_all(fd: int, data: bytes, timeout: float = 5.0) -> None:
    deadline = time.monotonic() + timeout
    offset = 0
    while offset < len(data):
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise UploadError("serial write timed out")
        _, writable, _ = select.select([], [fd], [], remaining)
        if not writable:
            raise UploadError("serial write timed out")
        try:
            offset += os.write(fd, data[offset:])
        except BlockingIOError:
            continue
    termios.tcdrain(fd)


def read_exact(fd: int, length: int, timeout: float) -> bytes:
    deadline = time.monotonic() + timeout
    result = bytearray()
    while len(result) < length:
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise TimeoutError("serial read timed out")
        readable, _, _ = select.select([fd], [], [], remaining)
        if not readable:
            raise TimeoutError("serial read timed out")
        try:
            chunk = os.read(fd, length - len(result))
        except BlockingIOError:
            continue
        if chunk:
            result.extend(chunk)
    return bytes(result)


def encode_packet(packet_type: int, sequence: int = 0, offset: int = 0,
                  payload: bytes = b"") -> bytes:
    if len(payload) > MAX_PAYLOAD:
        raise UploadError("packet payload exceeds protocol limit")
    header = HEADER.pack(MAGIC, VERSION, packet_type, sequence, offset,
                         len(payload))
    body = header + payload
    return body + CRC.pack(zlib.crc32(body) & 0xFFFFFFFF)


def read_packet(fd: int, timeout: float) -> tuple[int, int, int, bytes]:
    deadline = time.monotonic() + timeout
    matched = 0
    discarded = 0
    while matched < len(MAGIC):
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise TimeoutError(
                f"timed out waiting for updater response ({discarded} bytes ignored)")
        byte = read_exact(fd, 1, remaining)
        if byte == MAGIC[matched:matched + 1]:
            matched += 1
        else:
            discarded += 1
            matched = 1 if byte == MAGIC[:1] else 0

    rest = read_exact(fd, HEADER.size - len(MAGIC), deadline - time.monotonic())
    _, version, packet_type, sequence, offset, length = HEADER.unpack(MAGIC + rest)
    if version != VERSION or length > MAX_PAYLOAD:
        raise UploadError("invalid updater response header")
    payload = read_exact(fd, length, deadline - time.monotonic())
    expected_crc = CRC.unpack(read_exact(fd, CRC.size,
                                         deadline - time.monotonic()))[0]
    if zlib.crc32(MAGIC + rest + payload) & 0xFFFFFFFF != expected_crc:
        raise UploadError("updater response checksum mismatch")
    return packet_type, sequence, offset, payload


def wait_for_packet(fd: int, expected: set[int], timeout: float):
    packet = read_packet(fd, timeout)
    packet_type, _, _, payload = packet
    if packet_type == ERROR:
        raise UploadError(f"device rejected update: {describe_remote_error(payload)}")
    if packet_type not in expected:
        raise UploadError(f"unexpected device response type 0x{packet_type:02x}")
    return packet


def upload(port: str, image_path: str, baudrate: int) -> None:
    with open(image_path, "rb") as image_file:
        image = image_file.read()
    if not image:
        raise UploadError("firmware image is empty")

    checksum = zlib.crc32(image) & 0xFFFFFFFF
    fd = configure_port(port, baudrate)
    try:
        print(f"Requesting update mode on {port}...", flush=True)
        packet_type = None
        for attempt in range(1, HELLO_RETRIES + 1):
            if attempt > 1:
                print(f"No HELLO yet; retrying update request ({attempt}/"
                      f"{HELLO_RETRIES})", flush=True)
            write_all(fd, UPDATE_REQUEST)
            try:
                packet_type, _, _, payload = wait_for_packet(fd, {HELLO}, 2.0)
                break
            except TimeoutError:
                if attempt == HELLO_RETRIES:
                    raise
        if packet_type != HELLO or len(payload) != 4:
            raise UploadError("device did not enter update mode")
        capacity = struct.unpack("<I", payload)[0]
        if len(image) > capacity:
            raise UploadError(
                f"image is {len(image)} bytes but device staging capacity is "
                f"{capacity} bytes")

        print(f"Image: {len(image)} bytes; CRC32: {checksum:08x}", flush=True)
        begin = encode_packet(BEGIN, payload=struct.pack("<II", len(image), checksum))
        write_all(fd, begin)
        print("BEGIN sent; waiting for READY...", flush=True)
        _, _, _, payload = wait_for_packet(fd, {READY}, 30.0)
        if len(payload) != 8:
            raise UploadError("invalid READY response")
        chunk_size, device_capacity = struct.unpack("<II", payload)
        if chunk_size != CHUNK_SIZE or device_capacity != capacity:
            raise UploadError("device reported unsupported transfer settings")

        total_chunks = (len(image) + chunk_size - 1) // chunk_size
        print("Uploading...", flush=True)
        for sequence, offset in enumerate(range(0, len(image), chunk_size)):
            chunk = image[offset:offset + chunk_size]
            descriptor = encode_packet(
                RAW_CHUNK, sequence, offset,
                struct.pack("<II", len(chunk), zlib.crc32(chunk) & 0xFFFFFFFF))
            for attempt in range(1, 5):
                write_all(fd, descriptor)
                write_all(fd, chunk, timeout=10.0)
                try:
                    packet_type, ack_sequence, ack_offset, ack_payload = read_packet(
                        fd, ACK_TIMEOUT)
                except TimeoutError:
                    if attempt == 4:
                        raise UploadError(
                            f"no acknowledgement for raw chunk {sequence}")
                    continue
                if packet_type == ERROR:
                    raise UploadError(
                        f"device reported {describe_remote_error(ack_payload)} "
                        f"while writing raw chunk {sequence} at offset {offset}")
                if (packet_type == ACK and ack_sequence == sequence and
                        ack_offset == offset and not ack_payload):
                    break
                if packet_type != NAK or attempt == 4:
                    raise UploadError(
                        f"device rejected raw chunk {sequence} after {attempt} attempts")
            print(f"\r{(sequence + 1) * 100 // total_chunks:3d}% "
                  f"({sequence + 1}/{total_chunks} chunks)",
                  end="", flush=True)

        print("\nVerifying complete image on device...", flush=True)
        write_all(fd, encode_packet(END, total_chunks, len(image)))
        try:
            wait_for_packet(fd, {DONE}, 30.0)
        except TimeoutError:
            print("Device reset after activation began; verify its new firmware.",
                  flush=True)
            return
        print("Verified. Device is activating the image and rebooting.", flush=True)
    except KeyboardInterrupt:
        try:
            write_all(fd, encode_packet(ABORT), timeout=0.5)
        except (OSError, UploadError):
            pass
        print("\nUpdate aborted.", file=sys.stderr, flush=True)
        raise
    finally:
        os.close(fd)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("image", help="raw Teensy .bin image")
    parser.add_argument("--port", default="/dev/serial0",
                        help="UART device (default: /dev/serial0 on Raspberry Pi)")
    parser.add_argument("--baud", type=int, default=115200,
                        help="UART baud rate (match the device's UART setting)")
    args = parser.parse_args()
    try:
        upload(args.port, args.image, args.baud)
    except (OSError, UploadError, TimeoutError) as error:
        print(f"upload failed: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
