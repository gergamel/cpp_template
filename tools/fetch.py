from typing import Optional
from dataclasses import dataclass,field
from pathlib import Path
from urllib.parse import urlsplit
import urllib.request
import urllib.error
import ssl
import platform
import sys
import os
import yaml

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
                t.alternative_command = x['minimum_version']
            try:
                t.prepare_fetch()
            except Exception:
                pass
            tools.append(t)

        return ToolsManifest(
            tools = tools,
            notes = m['notes']
        )

# urllib.request.urlopen(url,head)
# if "Content-Disposition" in response.headers:
#     file_name:str = response.headers['Content-Disposition'].split("=")[1] 
# else:
#     file_name:str = Path(urlsplit(url).path).name

if __name__=="__main__":
    dry_run = True if (len(sys.argv)>1 and sys.argv[1] in {"-d","--dry-run"}) else False

    AVI_TOOLDIR = Path(os.environ['AVI_TOOLDIR'])
    SCRIPT_ROOT = Path(__file__).parent
    
    download_root = AVI_TOOLDIR / "dl"
    download_root.mkdir(parents=True,exist_ok=True)
    print(f"-"*80)
    print(f"AVI Development Tools Downloader")
    print(f" AVI_TOOLDIR = {AVI_TOOLDIR}")
    if dry_run: print(f" Dry run mode")
    print(f"-"*80)
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
        msg = ''
        try:
            with urllib.request.urlopen(url,context=ssl_ctx) as response:
                file_size:int = response.length
                if dry_run:
                    sys.stdout.write(f'{t.name:{col1_len}s}:{file_size:12}B : {filename:{col2_len}s}')
                else:
                    sys.stdout.write(f'{t.name:{col1_len}s}:{file_size:12}B : {filename:{col2_len}s} |         |' + '\b'*10)
                    sys.stdout.flush()
                prog_step = float(file_size)/10.0
                prog_next = prog_step
                rbytes = 0
                wbytes = 0
                file_path = download_root / filename
                if not dry_run:
                    with open(file_path,"wb") as fp:
                        # # 3. Stream data from response straight into fp
                        # shutil.copyfileobj(response, fp)
                        chunk_size = 4096
                        10*float(wbytes)/float(file_size)
                        while True:
                            chunk = response.read(chunk_size)
                            if not chunk:
                                break  # End of file reached
                            rlen = len(chunk)
                            rbytes += rlen
                            wlen = fp.write(chunk)
                            wbytes += wlen
                            if (wbytes>=prog_next):
                                sys.stdout.write('|')
                                sys.stdout.flush()
                                prog_next += prog_step
                            # progress = 100.0*float(wbytes)/float(file_size)
                            # sys.stdout.write('\b'*len(msg))
                            # msg = f"{wlen}/{file_size} Bytes {progress:.01f} %"
                            # sys.stdout.write(msg)
                            # sys.stdout.flush()
                sys.stdout.write(f"\n  \"{file_path}\"\n")
        except urllib.error.HTTPError as e:
            print(f"ERROR: {e} \"{url}\"")
        except urllib.error.URLError as e:
            # except ssl.SSLCertVerificationError as e:
            if len(e.args)>0 and type(e.args[0])==ssl.SSLCertVerificationError:
                print(f"ERROR: {e.args[0]} \"{url}\"")
            else:
                raise e