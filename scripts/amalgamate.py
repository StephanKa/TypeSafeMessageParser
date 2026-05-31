#!/usr/bin/env python3
"""
Single-header amalgamation script for TypeSafeMessageParser.

Produces a single self-contained header file by reading MessageParser.h
and wrapping it with include guards and all necessary standard includes.
"""

import argparse
import re
import sys
from pathlib import Path

BANNER = """\
// ==========================================================================
// TypeSafeMessageParser - Single Header Amalgamation
// Generated automatically - do not edit manually
// ==========================================================================
"""


def amalgamate(input_path: Path, output_path: Path) -> None:
    content = input_path.read_text(encoding="utf-8")

    # The header is already self-contained with #pragma once,
    # so we just wrap it with the banner
    output = BANNER + "\n" + content + "\n"

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(output, encoding="utf-8")
    print(f"Generated single header: {output_path} ({len(output)} bytes)")


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate single-header amalgamation")
    parser.add_argument(
        "--input",
        type=Path,
        default=Path(__file__).parent.parent / "src" / "include" / "MessageParser.h",
        help="Path to the main header file",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path(__file__).parent.parent / "single_include" / "MessageParser.h",
        help="Path to the output amalgamated header",
    )
    args = parser.parse_args()

    if not args.input.exists():
        print(f"Error: Input file not found: {args.input}", file=sys.stderr)
        return 1

    amalgamate(args.input, args.output)
    return 0


if __name__ == "__main__":
    sys.exit(main())
