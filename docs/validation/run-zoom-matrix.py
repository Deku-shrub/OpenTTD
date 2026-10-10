"""Windows runtime zoom matrix. Artifacts stay in build-smoothzoom/coverage.

Requires a built game, staged OpenGFX and language files (launch-prototype.ps1),
and Pillow. Only the process started by this script receives input or is closed.
"""
import argparse
import ctypes as C
from ctypes import wintypes as W
import json
from pathlib import Path
import shutil
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[2]
WORK = ROOT / 'build-smoothzoom'
sys.path.insert(0, str(WORK / 'capture-deps'))
from PIL import Image, ImageChops
u = C.WinDLL('user32', use_last_error=True)
u.GetWindowThreadProcessId.argtypes = [W.HWND, C.POINTER(W.DWORD)]
u.GetWindowTextW.argtypes = [W.HWND, W.LPWSTR, C.c_int]
u.PostMessageW.argtypes = [W.HWND, W.UINT, W.WPARAM, W.LPARAM]
u.IsHungAppWindow.argtypes = [W.HWND]
CASES = {
 'software': ('32bpp-anim', 'win32:no_threads', 'english.lng', 100, (960,600)),
 'simple': ('32bpp-simple', 'win32:no_threads', 'english.lng', 100, (960,600)),
 'optimized': ('32bpp-optimized', 'win32:no_threads', 'english.lng', 100, (960,600)),
 'sse2': ('32bpp-sse2-anim', 'win32:no_threads', 'english.lng', 100, (960,600)),
 'sse4': ('32bpp-sse4-anim', 'win32:no_threads', 'english.lng', 100, (960,600)),
 'opengl': ('40bpp-anim', 'win32-opengl:no_threads', 'english.lng', 100, (960,600)),
 'legacy': ('8bpp-optimized', 'win32:no_threads', 'english.lng', 100, (960,600)),
 'large': ('32bpp-anim', 'win32:no_threads', 'english.lng', 100, (1920,1080)),
 'hidpi': ('32bpp-anim', 'win32:no_threads', 'english.lng', 200, (1600,1000)),
 'rtl': ('32bpp-anim', 'win32:no_threads', 'arabic_egypt.lng', 100, (1280,800)),
 'cjk': ('32bpp-anim', 'win32:no_threads', 'japanese.lng', 100, (1280,800)),
}
class Game:
 def __init__(self, executable, config, blitter, driver, size, save, graphics, out):
  self.out=out;self.hwnd=None;self.graphics=graphics
  self.stdout=open(out/'stdout.log','w');self.stderr=open(out/'stderr.log','w')
  args=[str(executable),'-c',str(config),'-x','-X','-s','null','-m','null','-b',blitter,'-v',driver,'-I',graphics,'-r',f'{size[0]}x{size[1]}','-g']
  if save:args.append(str(save))
  self.process=subprocess.Popen(args,cwd=WORK,stdout=self.stdout,stderr=self.stderr)
  (WORK/'game.pid').write_text(str(self.process.pid))
  deadline=time.monotonic()+45
  while time.monotonic()<deadline:
   self.check()
   windows=[]
   @C.WINFUNCTYPE(W.BOOL,W.HWND,W.LPARAM)
   def enum(h,_):
    pid=W.DWORD();u.GetWindowThreadProcessId(h,C.byref(pid));name=C.create_unicode_buffer(512);u.GetWindowTextW(h,name,len(name))
    if pid.value==self.process.pid and name.value.startswith('OpenTTD '):windows.append(h)
    return True
   u.EnumWindows(enum,0)
   if windows:self.hwnd=windows[0];break
   time.sleep(.1)
  if not self.hwnd:raise RuntimeError('No game window')
  # High-definition sets can spend several seconds decoding sprites after
  # creating the window. Allow startup to finish before testing responsiveness.
  deadline=time.monotonic()+90
  time.sleep(30 if graphics=='OpenGFX2 High Def' or not save else 2)
  while u.IsHungAppWindow(self.hwnd) and time.monotonic()<deadline:
   if self.process.poll() is not None:raise RuntimeError('Game exited during graphics loading')
   time.sleep(.5)
  self.check()
 def check(self):
  if self.process.poll() is not None:raise RuntimeError(f'Game exited {self.process.returncode}: '+(self.out/'stderr.log').read_text(errors='replace')[-1000:])
  if self.hwnd and u.IsHungAppWindow(self.hwnd):raise RuntimeError('Game window unresponsive')
 def post(self,msg,w=0,l=0):
  self.check()
  if not u.PostMessageW(self.hwnd,msg,w,l):raise C.WinError(C.get_last_error())
 def key(self,key,scan=0):
  self.post(0x100,key,1|(scan<<16));self.post(0x101,key,0xc0000001|(scan<<16))
 def toggle(self):
  self.key(0,41);self.key(0);time.sleep(.2)
 def command(self,text):
  for ch in text:self.post(0x102,ord(ch),1)
  self.post(0x100,13,0x1c0001);self.post(0x102,13,0x1c0001);self.post(0x101,13,0xc01c0001);time.sleep(.35)
 def commands(self,*texts):
  self.toggle()
  for text in texts:self.command(text)
  if texts and texts[-1].startswith('load '):
   time.sleep(30 if self.graphics=='OpenGFX2 High Def' else 1)
  else:self.toggle();time.sleep(.3)
 def snapshot(self,name):
  filename='matrix-'+self.out.name+'-'+name
  path=WORK/'screenshot'/(filename+'.png')
  self.toggle();self.command('screenshot viewport no_con '+filename)
  deadline=time.monotonic()+15
  while time.monotonic()<deadline:
   self.check()
   if path.exists() and path.read_bytes().endswith(b'\x00\x00\x00\x00IEND\xaeB\x60\x82'):
    try:
     im=Image.open(path).convert('RGB');im.save(self.out/(name+'.png'));return im
    except (OSError, SyntaxError, ValueError):pass
   time.sleep(.1)
  raise RuntimeError('Screenshot not produced: '+filename)
 def close(self):
  if self.process.poll() is None:
   try:self.commands('quit');self.process.wait(timeout=10)
   except Exception:self.process.terminate();self.process.wait(timeout=10)
  self.stdout.close();self.stderr.close()

def run(name,args):
 blitter,driver,language,scale,size=CASES[name]
 stamp=time.strftime('%Y%m%dT%H%M%S');out=WORK/'coverage'/(name+'-'+stamp);out.mkdir(parents=True)
 exe=Path(args.executable).resolve();save=Path(args.save).resolve()
 for lng in [language]:shutil.copy2(exe.parent.parent/'lang'/lng,WORK/'lang'/lng)
 config=WORK/('matrix-'+name+'-'+stamp+'.cfg')
 config.write_text(f'''[misc]
fullscreen = false
language = {language}
gui_scale = {scale}
sounddriver = null
musicdriver = null
[gui]
smooth_zoom = true
smooth_scroll = true
fly_navigation = true
autosave = 0
errmsg_duration = 0
show_finances = false
threaded_saves = false
[graphicsset]
name = {args.graphics}
''')
 result={'case':name,'graphics':args.graphics,'blitter':blitter,'driver':driver,'language':language,'interface_scale':scale,'size':size,'artifacts':str(out.relative_to(ROOT)),'checks':[]}
 game=None
 try:
  game=Game.__new__(Game)
  game.__init__(exe,config,blitter,driver,size,save,args.graphics,out)
  game.commands('debug_level misc=5 script=5','script '+str(out/'console.log').replace('\\','/'),'scrollto instant 3644')
  scales=[1,1.3,2,.84,.25] if name!='legacy' else [1]
  for zoom in scales:
   game.commands('smoothzoom '+str(zoom),'smoothzoom');game.snapshot('zoom-'+str(zoom));result['checks'].append('render '+str(zoom))
  if name!='legacy':
   game.commands('smoothzoom 1.3')
   a=game.snapshot('settled-a');time.sleep(.7);b=game.snapshot('settled-b')
   # Station land crop, away from the GUI, mouse cursor and animated shoreline.
   crop=(int(size[0]*.32),int(size[1]*.35),int(size[0]*.7),int(size[1]*.72))
   bbox=ImageChops.difference(a.crop(crop),b.crop(crop)).getbbox()
   result['settled_crop_difference']=bbox
   if bbox is not None:raise AssertionError('Static land changed after rest')
   result['checks'].append('stable fractional rest')
   roundtrip='matrix-roundtrip-'+out.name
   game.commands('save '+roundtrip)
   game.commands('load '+roundtrip+'.sav');time.sleep(1)
   game.commands('smoothzoom');game.snapshot('roundtrip');result['checks'].append('save/load screenshot')
  else:
   game.commands('smoothzoom 1.3','smoothzoom');game.snapshot('unsupported-console-safe')
   result['checks'].append('unsupported fractional request does not crash')
  game.commands('script')
  result['status']='passed'
 except Exception as error:result['status']='failed';result['error']=str(error)
 finally:
  if game and hasattr(game,'process'):game.close()
  (out/'result.json').write_text(json.dumps(result,indent=2))
 print(json.dumps(result),flush=True)
 return result

if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--case',choices=list(CASES)+['all'],default='all');p.add_argument('--executable',default='C:/ottd-build/Release/openttd.exe');p.add_argument('--save',default=str(ROOT/'docs/validation/zoom-navigation.sav'));p.add_argument('--graphics',default='OpenGFX');args=p.parse_args()
 results=[run(name,args) for name in (CASES if args.case=='all' else [args.case])]
 summary=WORK/'coverage'/('matrix-summary-'+args.graphics.replace(' ','-')+'.json')
 previous=json.loads(summary.read_text()) if summary.exists() else []
 combined={item['case']:item for item in previous+results}
 summary.write_text(json.dumps([combined[name] for name in CASES if name in combined],indent=2))
 raise SystemExit(any(x['status']=='failed' for x in results))
