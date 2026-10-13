"""Uncaptured Windows camera performance and repeated-use checks.

Run against the downloaded network described in promotional-captures.md.
Requires staged OpenGFX and Pillow, as does run-zoom-matrix.py. Input goes only
to the process launched here. Losing foreground stops the measurements unless
--background is supplied for CPU/memory diagnosis, without presentation claims.
"""

import argparse
import ctypes as C
from ctypes import wintypes as W
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import re
import shutil
import time


ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--executable', type=Path, required=True)
    parser.add_argument('--save', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--group', choices=['baseline', 'interaction', 'soak', 'follow', 'all'], default='all')
    parser.add_argument('--rounds', type=int, default=2)
    parser.add_argument('--seconds', type=float, default=8)
    parser.add_argument('--cycles', type=int, default=32)
    parser.add_argument('--soak-paused', action='store_true')
    parser.add_argument('--actions', nargs='+', choices=['idle', 'pan', 'redraw', 'traffic'], default=['idle', 'pan', 'redraw', 'traffic'])
    parser.add_argument('--size', type=int, nargs=2, default=[1920, 1080])
    parser.add_argument('--no-labels', action='store_true')
    parser.add_argument('--no-animation', action='store_true')
    parser.add_argument('--opengl', action='store_true')
    parser.add_argument('--background', action='store_true', help='CPU/memory diagnosis without foreground presentation measurements')
    parser.add_argument('--vehicle-location', type=int, nargs=2, help='Centre button coordinates in the downloaded network train view')
    parser.add_argument('--vehicle-probe', action='store_true', help='Save one owned-game setup screenshot before following measurements')
    args = parser.parse_args()
    if args.rounds < 1 or args.seconds < 4 or args.cycles < 1:
        parser.error('Use positive rounds/cycles and at least four seconds per measurement.')
    if args.group == 'follow' and args.vehicle_location is None and not args.vehicle_probe:
        parser.error('Supply --vehicle-location or use --vehicle-probe to inspect the train view first.')
    exe, save, out = args.executable.resolve(), args.save.resolve(), args.out.resolve()
    out.mkdir(parents=True, exist_ok=False)
    profile = out / 'profile'
    for folder in ['baseset', 'lang', 'save', 'screenshot', 'ai', 'scripts']:
        (profile / folder).mkdir(parents=True, exist_ok=True)
    for pattern in ['opengfx-8*.tar']:
        archives = list((ROOT / 'build-smoothzoom/baseset').glob(pattern))
        if not archives:
            parser.error('Stage OpenGFX 8 in build-smoothzoom/baseset first.')
        for archive in archives:
            shutil.copy2(archive, profile / 'baseset' / archive.name)
    for descriptor in ['no_sound.obs', 'no_music.obm']:
        shutil.copy2(ROOT / 'build-smoothzoom/baseset' / descriptor, profile / 'baseset' / descriptor)
    languages = [exe.parent / 'lang/english.lng', exe.parent.parent / 'lang/english.lng']
    language = next((path for path in languages if path.is_file()), None)
    if language is None:
        parser.error('Cannot find english.lng alongside the executable or its build directory.')
    shutil.copy2(language, profile / 'lang/english.lng')
    shutil.copy2(save, profile / 'save/performance-base.sav')
    (profile / 'hotkeys.cfg').write_text('[global]\nrefresh_screen = F7\nprint_framerate = F6\n[maintoolbar]\ncompanies =\nsubsidies =\n', encoding='utf-8')
    display = ['FULL_DETAIL']
    if not args.no_labels:
        display += ['SHOW_TOWN_NAMES', 'SHOW_STATION_NAMES', 'SHOW_SIGNS', 'WAYPOINTS', 'SHOW_COMPETITOR_SIGNS']
    if not args.no_animation:
        display += ['FULL_ANIMATION']
    news = re.findall(r'var\s*=\s*news_display\.(\w+)', (ROOT / 'src/table/settings/news_display_settings.ini').read_text())
    cfg = profile / 'performance.cfg'
    cfg.write_text('[misc]\nfullscreen = false\ngui_scale = 100\nlanguage = english.lng\ndisplay_opt = ' + '|'.join(display) +
                   '\n[gui]\nautosave = 0\nshow_finances = false\nerrmsg_duration = 0\nthreaded_saves = false\n'
                   'smooth_scroll = true\ntoolbar_pos = 0\nfly_pan_speed = 120\nfly_duration_scale = 100\nfly_curve = 142\nfly_zoom_out = 2\n'
                   '[graphicsset]\nname = OpenGFX\n[news_display]\n' + ''.join(f'{name} = off\n' for name in news), encoding='utf-8')
    spec = importlib.util.spec_from_file_location('matrix', Path(__file__).with_name('run-zoom-matrix.py'))
    matrix = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(matrix)
    matrix.WORK = profile
    u = matrix.u
    u.SetProcessDPIAware()
    u.GetForegroundWindow.restype = W.HWND
    u.SetForegroundWindow.argtypes = [W.HWND]
    u.GetWindowLongPtrW.argtypes = [W.HWND, C.c_int]
    u.GetWindowLongPtrW.restype = C.c_ssize_t
    u.SetWindowLongPtrW.argtypes = [W.HWND, C.c_int, C.c_ssize_t]
    u.SetWindowPos.argtypes = [W.HWND, W.HWND, C.c_int, C.c_int, C.c_int, C.c_int, W.UINT]
    u.GetClientRect.argtypes = [W.HWND, C.POINTER(W.RECT)]
    kernel = C.WinDLL('kernel32', use_last_error=True)
    kernel.OpenProcess.argtypes = [W.DWORD, W.BOOL, W.DWORD]
    kernel.OpenProcess.restype = W.HANDLE
    kernel.CloseHandle.argtypes = [W.HANDLE]
    class Counters(C.Structure):
        _fields_ = [('cb', W.DWORD), ('faults', W.DWORD)] + [(name, C.c_size_t) for name in
                    ['peak_working', 'working', 'peak_paged', 'paged', 'peak_nonpaged', 'nonpaged', 'pagefile', 'peak_pagefile', 'private']]
    psapi = C.WinDLL('psapi', use_last_error=True)
    psapi.GetProcessMemoryInfo.argtypes = [W.HANDLE, C.POINTER(Counters), W.DWORD]
    report = {'executable': str(exe), 'executable_sha256': hashlib.sha256(exe.read_bytes()).hexdigest(),
              'save': str(save), 'save_sha256': hashlib.sha256(save.read_bytes()).hexdigest(),
              'configuration': vars(args) | {'executable': str(exe), 'save': str(save), 'out': str(out)},
              'recording': False, 'presentation_mode': 'background diagnosis' if args.background else 'foreground', 'results': []}
    game = handle = None
    def persist():
        (out / 'results.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    try:
        blitter, driver = ('40bpp-anim', 'win32-opengl') if args.opengl else ('32bpp-sse4-anim', 'win32')
        game = matrix.Game(exe, cfg, blitter, driver, tuple(args.size), save, 'OpenGFX', out)
        title = C.create_unicode_buffer(512)
        u.GetWindowTextW(game.hwnd, title, len(title))
        report.update({'window_title': title.value, 'blitter': blitter, 'driver': driver})
        original_check = game.check
        next_popup_check = 0
        def check():
            nonlocal next_popup_check
            original_check()
            if time.monotonic() < next_popup_check:
                return
            next_popup_check = time.monotonic() + 1
            popups = []
            @C.WINFUNCTYPE(W.BOOL, W.HWND, W.LPARAM)
            def window(hwnd, _):
                pid = W.DWORD()
                u.GetWindowThreadProcessId(hwnd, C.byref(pid))
                if pid.value != game.process.pid:
                    return True
                name = C.create_unicode_buffer(256)
                u.GetClassNameW(hwnd, name, len(name))
                if name.value != '#32770':
                    return True
                text = []
                @C.WINFUNCTYPE(W.BOOL, W.HWND, W.LPARAM)
                def child(hwnd, _):
                    label = C.create_unicode_buffer(4096)
                    u.GetWindowTextW(hwnd, label, len(label))
                    if label.value:
                        text.append(label.value)
                    return True
                u.EnumChildWindows(hwnd, child, 0)
                popups.append(text)
                return True
            u.EnumWindows(window, 0)
            if popups:
                raise RuntimeError('Owned game popup: ' + repr(popups))
        u.GetClassNameW.argtypes = [W.HWND, W.LPWSTR, C.c_int]
        game.check = check
        game.check()
        # Scan-code console toggles survive keyboard-layout differences.
        def toggle():
            game.key(0, 41)
            time.sleep(.1)
            game.key(0)
            time.sleep(.3)
        game.toggle = toggle
        def command(text):
            for character in text:
                game.post(0x102, ord(character), 1)
            # TranslateMessage generates Return's WM_CHAR. Posting it again
            # would cancel a flight immediately after its script starts.
            game.key(13, 28)
            time.sleep(.35)
        game.command = command
        style = u.GetWindowLongPtrW(game.hwnd, -16)
        u.SetWindowLongPtrW(game.hwnd, -16, (style & ~0x00CF0000) | 0x80000000)
        u.SetWindowPos(game.hwnd, None, 0, 0, *args.size, 0x0024)
        if not args.background:
            u.SetForegroundWindow(game.hwnd)
        time.sleep(1)
        rect = W.RECT()
        u.GetClientRect(game.hwnd, C.byref(rect))
        assert [rect.right, rect.bottom] == args.size
        handle = kernel.OpenProcess(0x410, False, game.process.pid)
        if not handle:
            raise C.WinError(C.get_last_error())
        def memory():
            counters = Counters()
            counters.cb = C.sizeof(counters)
            if not psapi.GetProcessMemoryInfo(handle, C.byref(counters), counters.cb):
                raise C.WinError(C.get_last_error())
            return {'private_bytes': counters.private, 'working_set_bytes': counters.working}
        def guard():
            game.check()
            pid = W.DWORD()
            u.GetWindowThreadProcessId(game.hwnd, C.byref(pid))
            assert pid.value == game.process.pid, 'Test window no longer belongs to the launched process'
            if not args.background:
                assert u.GetForegroundWindow() == game.hwnd, 'Owned game lost foreground'
        def mouse(message, buttons, x, y):
            guard()
            game.post(message, buttons, x | (y << 16))
        def prepare(enabled, scale, tile=35744, moving=True):
            guard()
            game.key(46)
            game.commands('load performance-base')
            game.commands('set gui.smooth_zoom 1', f'set gui.fly_navigation {int(enabled)}',
                          f'scrollto instant {tile}', f'smoothzoom {scale}', f'set gui.smooth_zoom {int(enabled)}',
                          'unpause' if moving else 'pause', 'debug_level misc=5')
            time.sleep(.5)
        def exercise(kind, seconds):
            x, y = args.size[0] // 2, args.size[1] // 2
            start = time.monotonic()
            if kind == 'pan':
                mouse(0x200, 0, x, y)
                mouse(0x204, 2, x, y)
            try:
                while (elapsed := time.monotonic() - start) < seconds:
                    guard()
                    if kind == 'redraw':
                        game.key(118)
                    elif kind == 'pan':
                        mouse(0x200, 2, round(x + min(240, x // 2) * math.sin(elapsed * 2 * math.pi / seconds)), y)
                    time.sleep(.01)
            finally:
                if kind == 'pan':
                    game.post(0x200, 2, x | (y << 16))
                    game.post(0x205, 0, x | (y << 16))
        def measure(name, action):
            guard()
            logfile = out / f'{name}.log'
            game.commands('script "' + logfile.as_posix() + '"')
            offset = (out / 'stderr.log').stat().st_size
            before = memory()
            started = time.monotonic()
            action()
            after = memory()
            # Sample with the console closed so its opaque window cannot
            # replace full viewport samples with cheaper clipped redraws.
            game.key(117)
            time.sleep(.1)
            game.commands('smoothzoom', 'script')
            text = logfile.read_text(encoding='utf-8', errors='replace')
            with (out / 'stderr.log').open('rb') as stream:
                stream.seek(offset)
                trace = stream.read().decode(errors='replace')
            assert 'ERROR:' not in text, text
            times = {m[0].strip(): list(map(float, m[1:])) for m in re.findall(r'^(.+?) times: ([\d.]+)ms  ([\d.]+)ms  ([\d.]+)ms', text, re.M)}
            rates = {m[0]: float(m[1]) for m in re.findall(r'^(.+?) rate: ([\d.]+)fps', text, re.M)}
            assert 'Viewport drawing' in times, text
            state = re.search(r'Camera centre (-?\d+),(-?\d+)\. Following vehicle (none|\d+)\.', text)
            row = {'name': name, 'elapsed_seconds': time.monotonic() - started, 'timings_ms_64_128_512': times,
                   'camera_state': {'centre': [int(state[1]), int(state[2])], 'following': state[3]} if state else None,
                   'foreground_at_end': u.GetForegroundWindow() == game.hwnd,
                   'timing_scope': 'Recent samples at the end of the action. Use camera traces for zoom/flight cadence.',
                   'rates_fps': rates, 'memory_before': before, 'memory_after': after,
                   'camera_traces': [line for line in trace.splitlines() if 'Smooth zoom:' in line or 'Camera flight' in line]}
            report['results'].append(row)
            persist()
            print(name, 'viewport_ms', times['Viewport drawing'][0], 'private_bytes', after['private_bytes'], flush=True)
            return row
        def wheel(direction):
            x, y = args.size[0] // 2, args.size[1] // 2
            mouse(0x200, 0, x, y)
            mouse(0x20a, ((120 * direction) & 0xffff) << 16, x, y)
        def navigate(target):
            script = profile / 'scripts/navigate.scr'
            script.write_text(f'hide_console\nscrollto {target}\n', encoding='utf-8')
            game.toggle()
            game.command('exec "' + script.as_posix() + '"')
        def click(x, y):
            mouse(0x200, 0, x, y)
            mouse(0x201, 1, x, y)
            time.sleep(.08)
            mouse(0x202, 0, x, y)
            time.sleep(.1)
        if args.group in ['baseline', 'all']:
            for run in range(args.rounds):
                for kind in [kind for kind in args.actions if kind != 'traffic']:
                    for enabled, scale in [(False, .25), (True, .25), (True, .25001), (True, 1 / 3)]:
                        prepare(enabled, scale)
                        measure(f'{run}-{kind}-{enabled}-{scale:.6f}', lambda kind=kind: exercise(kind, args.seconds))
                if 'traffic' in args.actions:
                    for moving in [False, True]:
                        for tile in [35744, 62418]:
                            prepare(True, 1 / 3, tile, moving)
                            measure(f'{run}-traffic-{moving}-{tile}', lambda: exercise('redraw', args.seconds))
        if args.group in ['interaction', 'all']:
            for run in range(args.rounds):
                for name, interval, reverse in [('slow', .24, False), ('fast', .08, False), ('reverse', .08, True)]:
                    prepare(True, 1)
                    def zoom():
                        for i in range(8):
                            wheel(1 if reverse and i >= 4 else -1)
                            time.sleep(interval)
                        exercise('idle', 1)
                    measure(f'{run}-zoom-{name}', zoom)
                for enabled in [False, True]:
                    for target in [35760, 5850]:
                        prepare(enabled, 1)
                        def flight(target=target):
                            navigate(target)
                            exercise('idle', 4)
                        row = measure(f'{run}-flight-{enabled}-{target}', flight)
                        started = any('Camera flight start:' in line for line in row['camera_traces'])
                        arrived = any('Camera flight:' in line for line in row['camera_traces'])
                        assert started == enabled and arrived == enabled, row
                for takeover in ['pan', 'zoom']:
                    prepare(True, 1)
                    def interrupt():
                        navigate(5850)
                        if takeover == 'pan':
                            exercise('pan', 1)
                        else:
                            wheel(-1)
                        exercise('idle', 3)
                    row = measure(f'{run}-takeover-{takeover}', interrupt)
                    assert any('Camera flight start:' in line for line in row['camera_traces']), row
                    assert not any('Camera flight:' in line for line in row['camera_traces']), row
        if args.group in ['soak', 'all']:
            prepare(True, 1 / 3, moving=not args.soak_paused)
            measure('soak-pan-before', lambda: exercise('pan', args.seconds))
            game.commands('smoothzoom 1')
            for cycle in range(args.cycles):
                def repeated():
                    for direction in [-1, 1]:
                        for _ in range(8):
                            wheel(direction)
                            time.sleep(.12)
                        exercise('idle', .5)
                    exercise('pan', 1)
                measure(f'soak-{cycle:03}', repeated)
            game.commands('scrollto instant 35744', 'smoothzoom ' + str(1 / 3))
            measure('soak-pan-after', lambda: exercise('pan', args.seconds))
        if args.group == 'follow':
            prepare(True, 1)
            for point in [(350, 10), (410, 29), (390, 76), (5, 30)]:
                click(*point)
                time.sleep(.3)
            if args.vehicle_probe:
                game.snapshot('vehicle-window')
            if args.vehicle_location:
                x, y = args.vehicle_location
                assert 0 <= x < args.size[0] and 0 <= y < args.size[1]
                for scale in [1, 1.3, 1 / 3]:
                    game.commands('set gui.fly_navigation 0', f'smoothzoom {scale}')
                    click(x, y)
                    click(x, y)
                    row = measure(f'follow-{scale:.6f}', lambda: exercise('idle', args.seconds))
                    assert row['camera_state'] and row['camera_state']['following'] != 'none', row
        report['status'] = 'complete'
        persist()
    except BaseException as exc:
        report['status'] = 'interrupted'
        report['error'] = str(exc)
        persist()
        raise
    finally:
        if handle:
            kernel.CloseHandle(handle)
        if game:
            game.close()


if __name__ == '__main__':
    main()
