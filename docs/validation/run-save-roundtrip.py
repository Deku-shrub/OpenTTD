"""Check stock -> modified -> stock saves in an isolated local profile.

Build the matching upstream revision first. Uses the runtime matrix's process
ownership and console helpers; never writes to a personal OpenTTD profile.
"""
import argparse
import json
from pathlib import Path
import shutil
import importlib.util

spec=importlib.util.spec_from_file_location('matrix',Path(__file__).with_name('run-zoom-matrix.py'))
matrix=importlib.util.module_from_spec(spec);spec.loader.exec_module(matrix)

def main():
 p=argparse.ArgumentParser()
 p.add_argument('--stock',required=True)
 p.add_argument('--modified',default='C:/ottd-build/Release/openttd.exe')
 args=p.parse_args()
 source=matrix.WORK
 matrix.WORK=source/'stock-profile'
 matrix.WORK.mkdir(exist_ok=True)
 for folder in ['baseset','lang','save','screenshot']:(matrix.WORK/folder).mkdir(exist_ok=True)
 for asset in (source/'baseset').iterdir():
  if asset.is_file() and not asset.name.startswith('OpenGFX2'):
   shutil.copy2(asset,matrix.WORK/'baseset'/asset.name)
 config=matrix.WORK/'roundtrip.cfg'
 config.write_text('''[misc]
language = english.lng
gui_scale = 100
[gui]
autosave = 0
errmsg_duration = 0
threaded_saves = false
smooth_zoom = true
show_finances = false
[game_creation]
starting_year = 2050
map_x = 8
map_y = 8
[graphicsset]
name = OpenGFX
''')
 results=[]
 stages=[('stock-created',args.stock,None,'stock-original'),
         ('modified-resaved',args.modified,'stock-original','modified-roundtrip'),
         ('stock-reopened',args.stock,'modified-roundtrip',None)]
 for name,executable,load,save in stages:
  exe=Path(executable).resolve()
  shutil.copy2(exe.parent.parent/'lang'/'english.lng',matrix.WORK/'lang'/'english.lng')
  out=matrix.WORK/name;out.mkdir(exist_ok=True)
  game=None
  try:
   game=matrix.Game.__new__(matrix.Game)
   game.__init__(exe,config,'32bpp-anim','win32:no_threads',(960,600),
                 matrix.WORK/'save'/(load+'.sav') if load else None,'OpenGFX',out)
   game.commands('script '+str(out/'console.log').replace('\\','/'))
   if not load:game.commands('pause')
   if name=='modified-resaved':game.commands('smoothzoom 1.3','smoothzoom')
   if save:
    game.commands('save '+save)
    saved=matrix.WORK/'save'/(save+'.sav')
    if not saved.exists() or saved.stat().st_size<1000:raise AssertionError('Save not created')
   game.snapshot(name)
   game.commands('script')
   results.append({'stage':name,'status':'passed'})
  finally:
   if game and hasattr(game,'process'):game.close()
  print(json.dumps(results[-1]),flush=True)
 (matrix.WORK/'result.json').write_text(json.dumps(results,indent=2))

if __name__=='__main__':main()
