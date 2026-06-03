#!/usr/bin/env python3
import re
import sys


def list_commands(path):
    with open(path, "r", encoding="utf-8") as manifest:
        for line in manifest:
            match = re.match(r"^\s*command:\s*(.+?)\s*$", line)
            if match:
                print(match.group(1))


def list_packages(path, manager):
    current_packages = None
    package_values = []

    with open(path, "r", encoding="utf-8") as manifest:
        for line in manifest:
            name_match = re.match(r"^\s*-\s*name:\s*(.+?)\s*$", line)
            if name_match:
                if current_packages is not None and manager in current_packages:
                    package_values.append(current_packages[manager])
                current_packages = {}
                continue

            if current_packages is None:
                continue

            if re.match(r"^\s*packages:\s*$", line):
                continue

            package_line = re.match(r"^\s*(\w+):\s*(.+?)\s*$", line)
            if package_line:
                key, value = package_line.group(1), package_line.group(2)
                current_packages[key] = value
                continue

        if current_packages is not None and manager in current_packages:
            package_values.append(current_packages[manager])

    for packages in package_values:
        print(packages)


def main():
    if len(sys.argv) < 3:
        sys.exit(1)

    command = sys.argv[1]
    path = sys.argv[2]

    if command == "commands":
        list_commands(path)
    elif command == "packages" and len(sys.argv) == 4:
        list_packages(path, sys.argv[3])
    else:
        sys.exit(1)


if __name__ == "__main__":
    main()
