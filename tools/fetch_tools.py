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

AVI_TOOLDIR = Path(os.environ['AVI_TOOLDIR'])
SCRIPT_ROOT = Path(__file__).parent

@dataclass
class ToolDefinition():
    name                : str
    command             : str
    packages            : dict[str,str]
    archives            : dict[str,str] = field(default_factory=dict)
    alternative_command : str = ''
    minimum_version     : Optional[str] = None

@dataclass
class ToolsManifest():
    tools: list[ToolDefinition]
    notes: list[str]

def load_manifest(path:str|Path):
    with open(str(path),"r") as f:
        m = yaml.safe_load(f)
        tmp = [ToolDefinition(**x) for x in m['tools']]
        return ToolsManifest(
            tools = tmp,
            notes = m['notes']
        )

if __name__=="__main__":
    dry_run = True if (len(sys.argv)>1 and sys.argv[1] in {"-d","--dry-run"}) else False
    
    download_root = AVI_TOOLDIR / "dl"
    download_root.mkdir(parents=True,exist_ok=True)
    m = load_manifest(SCRIPT_ROOT / "manifest.yaml")
    os_type = platform.system().lower()
    for t in m.tools:
        if os_type in t.archives:
            url = t.archives[os_type]
            msg = ''
            try:
                with urllib.request.urlopen(url) as response:
                    if "Content-Disposition" in response.headers:
                        file_name:str = response.headers['Content-Disposition'].split("=")[1] 
                    else:
                        file_name:str = Path(urlsplit(url).path).name
                    file_size:int = response.length
                    sys.stdout.write(f'[{t.name:7s}] fetching {file_name:48s}|         |')
                    sys.stdout.write('\b'*10)
                    sys.stdout.flush()
                    prog_step = float(file_size)/10.0
                    prog_next = prog_step
                    rbytes = 0
                    wbytes = 0
                    if dry_run:
                        sys.stdout.write(f" DRY RUN | {file_size} Bytes")
                    else:
                        with open(file_name,"wb") as fp:
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
                    sys.stdout.write("\n")
            except urllib.error.HTTPError as e:
                print(f"ERROR: {e} \"{url}\"")
            except urllib.error.URLError as e:
                # except ssl.SSLCertVerificationError as e:
                if len(e.args)>0 and type(e.args[0])==ssl.SSLCertVerificationError:
                    print(f"ERROR: {e.args[0]} \"{url}\"")
                else:
                    raise e