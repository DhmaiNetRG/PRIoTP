#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""PRIoTPS Launcher entry point."""
import sys

# Ensure UTF-8 output on Windows (Python 3.7+)
if sys.platform == 'win32':
    try:
        sys.stdout.reconfigure(encoding='utf-8', errors='replace')
        sys.stderr.reconfigure(encoding='utf-8', errors='replace')
    except AttributeError:
        pass  # Python < 3.7 fallback

from launcher.priotps_launcher import main

if __name__ == '__main__':
    main()
