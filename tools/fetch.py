from typing import Generator, Optional
from dataclasses import dataclass,field
from pathlib import Path, PurePosixPath
from urllib.parse import urlsplit
import shutil
import copy
import tarfile
import urllib.request
import urllib.error
import ssl
import platform
import sys
import os
import yaml
import zipfile

"""
Python bundles its own CA certificate store (derived from Mozilla's) and ignores the 
Windows system certificate store entirely. When your machine has a corporate PKI root or an 
SSL-inspection proxy (e.g. Zscaler, Netskope, a corporate firewall), those custom CA certs live 
in the Windows ROOT/CA stores but Python never sees them — hence "unable to get local issuer 
certificate."

create_ssl_context() builds a normal ssl.create_default_context() (which includes Mozilla CAs), 
then on Windows it calls the Windows-only ssl.enum_certificates() to read every cert from the ROOT
and CA system stores and loads them in too.

ssl.DER_cert_to_PEM_cert() converts from the binary DER format Windows uses to PEM that Python's 
SSL stack accepts.

The ssl_ctx is created once and reused across all downloads.

If you're still seeing the error after this fix, the most likely cause is that the corporate CA 
cert itself hasn't been installed in the Windows system store — that's an IT/admin issue rather 
than a code one.
"""
def _windows_cert_store_names() -> list[str]:
    import winreg
    names: set[str] = set()
    for root in (winreg.HKEY_LOCAL_MACHINE, winreg.HKEY_CURRENT_USER):
        for path in (r'SOFTWARE\Microsoft\SystemCertificates',
                     r'SOFTWARE\Policies\Microsoft\SystemCertificates'):
            try:
                with winreg.OpenKey(root, path) as key:
                    i = 0
                    while True:
                        try:
                            names.add(winreg.EnumKey(key, i))
                            i += 1
                        except OSError:
                            break
            except OSError:
                pass
    return list(names)

def create_ssl_context() -> ssl.SSLContext:
    # certifi ships a more up-to-date Mozilla CA bundle than what Python/OpenSSL bundles.
    # e.g. Python 3.13 + OpenSSL 3.0.18 is missing GlobalSign ECC Root CA - R5, which
    # developer.arm.com's chain terminates at. Fall back to the default if not installed.
    try:
        import certifi
        ctx = ssl.create_default_context(cafile=certifi.where())
    except ImportError:
        ctx = ssl.create_default_context()
    if platform.system() == 'Windows':
        # Also load Windows system cert stores so corporate/proxy CAs are trusted.
        # Store names come from the registry rather than a hardcoded list.
        for store_name in _windows_cert_store_names():
            try:
                for cert, _, _ in ssl.enum_certificates(store_name):
                    if isinstance(cert, bytes):
                        try:
                            ctx.load_verify_locations(cadata=ssl.DER_cert_to_PEM_cert(cert))
                        except ssl.SSLError:
                            pass
            except OSError:
                pass
    return ctx

@dataclass
class FileUrl():
    url      : str
    filename : str=''


@dataclass
class ToolDefinition():
    name                : str
    command             : str
    packages            : dict[str,str]
    archives            : dict[str,FileUrl] = field(default_factory=dict)
    alternative_command : str = ''
    minimum_version     : Optional[str] = None
    version             : str = ''
    def prepare_fetch(self):
        os_type = platform.system().lower()
        if os_type not in self.archives:
            return
        response = urllib.request.Request(self.archives[os_type].url,method="HEAD")
        if "Content-Disposition" in response.headers:
            self.archives[os_type].filename = response.headers['Content-Disposition'].split("=")[1] 
        else:
            self.archives[os_type].filename = Path(urlsplit(self.archives[os_type].url).path).name
          
@dataclass
class ToolsManifest():
    tools: list[ToolDefinition]
    notes: list[str]

def load_manifest(path:str|Path):
    print(f"Reading    : {path}")
    with open(str(path),"r") as f:
        m = yaml.safe_load(f)
        tools:list[ToolDefinition] = []
        for x in m['tools']:
            archives = {k:FileUrl(v) for k,v in x['archives'].items()} if 'archives' in x else {}
            t = ToolDefinition(
                name                = x['name'],
                command             = x['command'],
                packages            = x['packages'],
                archives            = archives
            )
            if 'alternative_command' in x:
                t.alternative_command = x['alternative_command']
            if 'minimum_version' in x:
                t.minimum_version = x['minimum_version']
            if 'version' in x:
                t.version = x['version']
            try:
                t.prepare_fetch()
            except Exception:
                pass
            tools.append(t)

        return ToolsManifest(
            tools = tools,
            notes = m['notes']
        )

"Downloading: "
"Downloaded : "
"Extracting : "

def fetch_archive(context:ssl.SSLContext,url:str,filepath:Path,dry_run:bool=False):
    try:
        with urllib.request.urlopen(url,context=ssl_ctx) as response:
            file_size:int = response.length
            print(f'Downloading: {t.name:{col1_len}s}:{file_size:12}B : {filename}')
            # prog_step = float(file_size)/10.0
            # prog_next = prog_step
            # rbytes = 0
            # wbytes = 0
            if not dry_run:
                with filepath.open("wb") as fp:
                    # 3. Stream data from response straight into fp
                    shutil.copyfileobj(response, fp)
                    ##### STATUS BAR. TODO: Replace with something that can handle multiple threads? #####
                    # chunk_size = 4096
                    # # 10*float(wbytes)/float(file_size)
                    # while True:
                    #     chunk = response.read(chunk_size)
                    #     if not chunk:
                    #         break  # End of file reached
                    #     rlen = len(chunk)
                    #     rbytes += rlen
                    #     wlen = fp.write(chunk)
                    #     wbytes += wlen
                    #     if (wbytes>=prog_next):
                    #         sys.stdout.write('|')
                    #         sys.stdout.flush()
                    #         prog_next += prog_step
                    #     # progress = 100.0*float(wbytes)/float(file_size)
                    #     # sys.stdout.write('\b'*len(msg))
                    #     # msg = f"{wlen}/{file_size} Bytes {progress:.01f} %"
                    #     # sys.stdout.write(msg)
                    #     # sys.stdout.flush()      
                print(f"Downloaded : {filepath}")
    except urllib.error.HTTPError as e:
        print(f"ERROR: {e} \"{url}\"")
    except urllib.error.URLError as e:
        # except ssl.SSLCertVerificationError as e:
        if len(e.args)>0 and type(e.args[0])==ssl.SSLCertVerificationError:
            print(f"ERROR: {e.args[0]} \"{url}\"")
        else:
            raise e

def ls_archive(path:str,maxdepth:int=1)->list[str]:
    """
    List all file/directories in an archive up to a specified maximum depth
    (behaviour similar to 'find')

    :param path: str        Path of the archive to process
    :param maxdepth: int    Maximum depth for recursion
    :rtype: list[str]       List of file/directory names
    """
    def _entry(name: str) -> str | None:
        parts = PurePosixPath(name).parts
        # Some tar tools emit entries like './bin/foo' — skip the leading '.'
        if parts and parts[0] == '.':
            parts = parts[1:]
        if not parts or len(parts) > maxdepth:
            return None
        return '/'.join(parts)

    entries: set[str] = set()
    name_lower = path.lower()

    if any(name_lower.endswith(s) for s in ('.tar', '.tar.gz', '.tgz', '.tar.bz2', '.tar.xz')):
        # r|* = streaming (non-seekable) mode — essential for .xz which has no random access.
        # At maxdepth=1, break on the first member that exceeds depth 1: we already have the
        # root name and can avoid decompressing the rest of a multi-GB archive.
        # At maxdepth>1 we must read all members at the requested depth (siblings of a
        # directory appear after its children in tar order, so early exit isn't safe).
        with tarfile.open(path, 'r|*') as tf:
            for member in tf:
                e = _entry(member.name)
                if e:
                    entries.add(e)
                if maxdepth == 1:
                    parts = PurePosixPath(member.name).parts
                    if parts and parts[0] == '.':
                        parts = parts[1:]
                    if len(parts) > 1:
                        break
    elif name_lower.endswith('.zip'):
        with zipfile.ZipFile(path) as zf:
            for member_name in zf.namelist():
                e = _entry(member_name)
                if e:
                    entries.add(e)

    return sorted(entries)

def extract_archive(archive_path:str,target_dir:str)->Path:
    """
    Extract an archive into target_dir, preserving its top-level folder structure.
    Supports tar, tar.gz/tgz, tar.bz2, tar.xz, and zip.

    If the archive has a single root directory (e.g. arm-gnu-toolchain-15.2.rel1-.../),
    it is extracted directly into target_dir so that root folder appears inside it.
    If the archive is flat (e.g. ninja-linux.zip containing only 'ninja'), a wrapper
    directory named after the archive file (minus extension) is created inside target_dir.

    :param archive_path: str  Path of the archive to extract
    :param target_dir:   str  Parent directory to extract into (created if absent)
    :returns: Path            Path of the extracted top-level directory
    """
    target = Path(target_dir)
    target.mkdir(parents=True, exist_ok=True)
    name_lower = archive_path.lower()

    # Stem used as the wrapper folder name for flat archives (strips compound extensions)
    arch_name = Path(archive_path).name
    stem = arch_name
    for ext in ('.tar.gz', '.tar.bz2', '.tar.xz', '.tar', '.tgz', '.zip'):
        if arch_name.lower().endswith(ext):
            stem = arch_name[:-len(ext)]
            break

    def _norm(name: str) -> str:
        n = name.lstrip('/')
        return n[2:] if n.startswith('./') else n

    if any(name_lower.endswith(s) for s in ('.tar', '.tar.gz', '.tgz', '.tar.bz2', '.tar.xz')):
        root_dir = None
        first = True

        def _members(tf: tarfile.TarFile) -> Generator[tarfile.TarInfo, None, None]:
            nonlocal root_dir, first
            for m in tf:
                n = _norm(m.name)
                if first:
                    first = False
                    if '/' in n:
                        # First member is nested: first path component is the root
                        root_dir = n.split('/')[0]
                    elif m.isdir():
                        # Explicit root directory entry with no children yet seen
                        root_dir = n
                    # Already extracted: stop the generator before yielding anything
                    if (target / (root_dir if root_dir else stem)).exists():
                        return
                if root_dir is None:
                    # Flat archive: wrap every entry under the stem subfolder
                    m = copy.copy(m)
                    m.name = stem + '/' + n
                yield m

        with tarfile.open(archive_path, 'r|*') as tf:
            tf.extractall(path=target, members=_members(tf), filter='tar')
        return target / (root_dir if root_dir else stem)

    elif name_lower.endswith('.zip'):
        with zipfile.ZipFile(archive_path) as zf:
            names = zf.namelist()
            root_dir = None
            if names:
                parts0 = PurePosixPath(names[0]).parts
                if parts0:
                    candidate = parts0[0]
                    prefix = candidate + '/'
                    if all(n == prefix or n.startswith(prefix) for n in names):
                        root_dir = candidate
            dest = target / (root_dir if root_dir else stem)
            if dest.exists():
                return dest
            if root_dir:
                zf.extractall(path=target)
            else:
                dest.mkdir(exist_ok=True)
                zf.extractall(path=dest)
            return dest
    return target

if __name__=="__main__":
    dry_run = True if (len(sys.argv)>1 and sys.argv[1] in {"-d","--dry-run"}) else False

    AVI_TOOLDIR = Path(os.environ['AVI_TOOLDIR'])
    SCRIPT_ROOT = Path(__file__).parent
    
    download_root = AVI_TOOLDIR / "dl"
    download_root.mkdir(parents=True,exist_ok=True)
    print(f"-"*80)
    print(f" AVI Development Tools Manager")
    print(f"-"*80)
    print(f"AVI_TOOLDIR: {AVI_TOOLDIR}")
    if dry_run: print(f"*** Dry run mode ***")
    
    os_type = platform.system().lower()
    ssl_ctx = create_ssl_context()

    m = load_manifest(SCRIPT_ROOT / "manifest.yaml")
    fetch_list = [t for t in m.tools if os_type in t.archives]
    for t in fetch_list:
        t.prepare_fetch()
    col1_len = max(len(t.name) for t in fetch_list)
    col2_len = max(len(t.archives[os_type].filename) for t in fetch_list)
    for t in fetch_list:
        url = t.archives[os_type].url
        filename = t.archives[os_type].filename
        filepath = download_root / filename

        if not filepath.exists():
            fetch_archive(ssl_ctx,url,filepath,dry_run=dry_run)
            # TODO: Crypto digest checking of archives
        if dry_run:
            print(f"**dry run** : {filepath}" )
        else:
            print(f"Extracting : {filepath}" )
            tooldir = extract_archive(str(filepath),str(AVI_TOOLDIR))
            if t.version and t.version not in tooldir.name:
                versioned = tooldir.parent / (tooldir.name + '-' + t.version)
                if not versioned.exists():
                    tooldir.rename(versioned)
                tooldir = versioned
            print(f"DONE       : {tooldir}" )
