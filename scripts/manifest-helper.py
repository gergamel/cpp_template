#!/usr/bin/env python3
import argparse
import os
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
import urllib.request


def load_manifest(path):
    tools = []
    current = None
    section = None

    with open(path, "r", encoding="utf-8") as manifest:
        for line in manifest:
            line = line.rstrip("\n")
            name_match = re.match(r"^\s*-\s*name:\s*(.+?)\s*$", line)
            if name_match:
                if current is not None:
                    tools.append(current)
                current = {
                    "name": name_match.group(1),
                    "command": None,
                    "packages": {},
                    "archives": {},
                }
                section = None
                continue

            if current is None:
                continue

            command_match = re.match(r"^\s*command:\s*(.+?)\s*$", line)
            if command_match:
                current["command"] = command_match.group(1)
                continue

            if re.match(r"^\s*packages:\s*$", line):
                section = "packages"
                continue

            if re.match(r"^\s*archives:\s*$", line):
                section = "archives"
                continue

            if section == "packages":
                package_line = re.match(r"^\s*(\w+):\s*(.+?)\s*$", line)
                if package_line:
                    current["packages"][package_line.group(1)] = package_line.group(2)
                continue

            if section == "archives":
                archive_line = re.match(r"^\s*(\w+):\s*(.+?)\s*$", line)
                if archive_line:
                    current["archives"][archive_line.group(1)] = archive_line.group(2)
                continue

    if current is not None:
        tools.append(current)

    return tools


def list_commands(path):
    for tool in load_manifest(path):
        if tool["command"]:
            print(tool["command"])


def list_packages(path, manager):
    values = []
    for tool in load_manifest(path):
        packages = tool["packages"].get(manager)
        if packages:
            values.append(packages)
    if values:
        print(" ".join(values))


def get_archive_url(path, platform, tool_name):
    tool = next((tool for tool in load_manifest(path) if tool["name"] == tool_name), None)
    if tool is None:
        return None
    return tool["archives"].get(platform)


def archive_url(path, platform, tool_name):
    url = get_archive_url(path, platform, tool_name)
    if url:
        print(url)


def check_commands(path):
    for command in [tool["command"] for tool in load_manifest(path) if tool["command"]]:
        exists = shutil.which(command) is not None
        print(f"OK: {command}" if exists else f"MISSING: {command}")


def install_packages(path, manager, dry_run=False):
    package_text = ""
    for tool in load_manifest(path):
        packages = tool["packages"].get(manager)
        if packages:
            package_text += f" {packages}"

    packages = shlex.split(package_text.strip())
    if not packages:
        print(f"No packages found for manager '{manager}'.")
        return 0

    if manager == "dnf":
        cmd = ["dnf", "install", "-y"] + packages
    elif manager == "apt":
        update_cmd = ["apt-get", "update", "-y"]
        install_cmd = ["apt-get", "install", "-y", "--no-install-recommends"] + packages
        if dry_run:
            print("+ " + shlex.join(update_cmd))
            print("+ " + shlex.join(install_cmd))
            return 0
        subprocess.run(update_cmd, check=True)
        return subprocess.run(install_cmd, check=True).returncode
    else:
        raise ValueError(f"Unsupported package manager: {manager}")

    if dry_run:
        print("+ " + shlex.join(cmd))
        return 0

    return subprocess.run(cmd, check=True).returncode


def download_file(url, dest, dry_run=False):
    if dry_run:
        print(f"+ Download {url} -> {dest}")
        return

    urllib.request.urlretrieve(url, dest)


def extract_archive(archive_path, dest_dir, dry_run=False):
    if dry_run:
        print(f"+ Extract {archive_path} -> {dest_dir}")
        return
    shutil.unpack_archive(archive_path, dest_dir)


def install_tool(path, tool_name, tools_root, platform, dry_run=False):
    url = get_archive_url(path, platform, tool_name)
    if not url:
        raise RuntimeError(f"No archive URL found for {tool_name} on platform {platform}")

    with tempfile.TemporaryDirectory() as tmp_dir:
        archive_file = os.path.join(tmp_dir, os.path.basename(url))
        download_file(url, archive_file, dry_run=dry_run)
        extract_dir = os.path.join(tmp_dir, "extract")
        extract_archive(archive_file, extract_dir, dry_run=dry_run)

        if dry_run:
            return 0

        contents = [entry for entry in os.listdir(extract_dir) if os.path.isdir(os.path.join(extract_dir, entry))]
        root_src = extract_dir
        if len(contents) == 1:
            root_src = os.path.join(extract_dir, contents[0])

        dest_dir = os.path.join(tools_root, tool_name)
        if os.path.exists(dest_dir):
            shutil.rmtree(dest_dir)
        shutil.copytree(root_src, dest_dir)

        bin_dir = os.path.join(dest_dir, "bin")
        if os.path.isdir(bin_dir):
            target_bin = os.path.join(tools_root, "bin")
            os.makedirs(target_bin, exist_ok=True)
            for item in os.listdir(bin_dir):
                src = os.path.join(bin_dir, item)
                dst = os.path.join(target_bin, item)
                if os.path.isfile(src):
                    shutil.copy2(src, dst)

        print(f"Installed {tool_name} into {dest_dir}")


def parse_args(argv=None):
    parser = argparse.ArgumentParser(description="Manifest helper for tool discovery, package management, and archive installs.")
    subparsers = parser.add_subparsers(dest="command", required=True)

    subparsers.add_parser("commands", help="List manifest commands").add_argument("manifest", nargs="?", default=os.path.join(os.path.dirname(os.path.realpath(__file__)), "../tools/manifest.yaml"))

    packages_parser = subparsers.add_parser("packages", help="List manifest packages for a package manager")
    packages_parser.add_argument("manager", choices=["apt", "dnf"])
    packages_parser.add_argument("manifest", nargs="?", default=os.path.join(os.path.dirname(os.path.realpath(__file__)), "../tools/manifest.yaml"))

    archive_parser = subparsers.add_parser("archive-url", help="Lookup archive URL for a tool and platform")
    archive_parser.add_argument("platform", choices=["linux", "windows"])
    archive_parser.add_argument("tool")
    archive_parser.add_argument("manifest", nargs="?", default=os.path.join(os.path.dirname(os.path.realpath(__file__)), "../tools/manifest.yaml"))

    subparsers.add_parser("check", help="Check manifest commands are installed").add_argument("manifest", nargs="?", default=os.path.join(os.path.dirname(os.path.realpath(__file__)), "../tools/manifest.yaml"))

    install_parser = subparsers.add_parser("install-packages", help="Install packages from manifest using a package manager")
    install_parser.add_argument("manager", choices=["apt", "dnf"])
    install_parser.add_argument("manifest", nargs="?", default=os.path.join(os.path.dirname(os.path.realpath(__file__)), "../tools/manifest.yaml"))
    install_parser.add_argument("--dry-run", action="store_true")

    fetch_parser = subparsers.add_parser("fetch-tool", help="Download and install a tool archive from the manifest")
    fetch_parser.add_argument("tool")
    fetch_parser.add_argument("--platform", choices=["linux", "windows"], default=None)
    fetch_parser.add_argument("--tools-root", default=None)
    fetch_parser.add_argument("--manifest", default=os.path.join(os.path.dirname(os.path.realpath(__file__)), "../tools/manifest.yaml"))
    fetch_parser.add_argument("--dry-run", action="store_true")

    return parser.parse_args(argv)


def platform_name():
    if sys.platform.startswith("linux"):
        return "linux"
    if sys.platform.startswith("win"):
        return "windows"
    raise RuntimeError(f"Unsupported platform: {sys.platform}")


def main(argv=None):
    args = parse_args(argv)

    if args.command == "commands":
        list_commands(args.manifest)
    elif args.command == "packages":
        list_packages(args.manifest, args.manager)
    elif args.command == "archive-url":
        archive_url(args.manifest, args.platform, args.tool)
    elif args.command == "check":
        check_commands(args.manifest)
    elif args.command == "install-packages":
        install_packages(args.manifest, args.manager, dry_run=args.dry_run)
    elif args.command == "fetch-tool":
        platform = args.platform or platform_name()
        tools_root = args.tools_root or ("C:/tools" if platform == "windows" else "/home/share/tools")
        install_tool(args.manifest, args.tool, tools_root, platform, dry_run=args.dry_run)
    else:
        raise RuntimeError(f"Unknown command: {args.command}")


if __name__ == "__main__":
    main()
