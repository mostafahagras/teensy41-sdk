#!/usr/bin/env python3
"""Statistical sanity check for TRNG output captured over USB serial.

Capture with the serial monitor, then run:
    ./scripts/monitor --validate 2>/dev/null | ./scripts/verify_rng

The sketch prints lines like "rng: <hex>" containing random bytes.
"""
import collections
import math
import sys

BYTES_LIMIT = 1 << 20  # 1 MiB is plenty for a chi-square


def collect(path):
    text = open(path, "r", errors="ignore").read() if path else sys.stdin.read()
    out = bytearray()
    for line in text.splitlines():
        marker = "rng:"
        pos = line.find(marker)
        if pos == -1:
            continue
        hex_str = line[pos + len(marker):].strip().split()[0]
        try:
            out += bytes.fromhex(hex_str)
        except ValueError:
            # tolerate truncated/incomplete monitor lines
            continue
        if len(out) >= BYTES_LIMIT:
            break
    return bytes(out[:BYTES_LIMIT])


def main():
    data = collect(sys.argv[1] if len(sys.argv) > 1 else None)
    n = len(data)

    if n < 8192:
        print(f"not enough data: {n} bytes captured (need >= 8192)")
        return 2

    counts = [0] * 256
    ones = 0
    for byte in data:
        counts[byte] += 1
        ones += bin(byte).count("1")

    expected = n / 256.0
    chi2 = sum((v - expected) ** 2 / expected for v in counts)
    # df = 255; the common chi-square pass band is 197.605..306.395
    passed = 197.605 < chi2 < 306.395

    print(f"bytes analysed        : {n}")
    print(f"ones proportion       : {ones / (n * 8):.4f} (want ~0.5)")
    print(f"chi-square over bytes : {chi2:.2f} on 255 df "
          f"(pass if 197.6 < x < 306.4) -> {'PASS' if passed else 'FAIL'}")

    if passed:
        print("result: data is uniform; TRNG looks healthy")
        return 0
    print("result: distribution looks biased; do NOT trust samples")
    return 1


if __name__ == "__main__":
    sys.exit(main())
