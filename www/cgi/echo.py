#!/usr/bin/env python3
import os, sys, urllib.parse
length = int(os.environ.get("CONTENT_LENGTH","0") or 0)
data = sys.stdin.read(length)
fields = urllib.parse.parse_qs(data)
name = fields.get("name", ["Anon"])[0]
print("Content-Type: text/plain")
print()
print(f"Hello, {name} (from CGI)")
