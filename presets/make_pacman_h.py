"""Генерує ../pacman.h зі спрайтами й палітрами PacChaseLogo.ino (MakerWorld #1133312).
Використання: python3 make_pacman_h.py [шлях до .ino]"""
import os, re, sys

here = os.path.dirname(os.path.abspath(__file__))
src = open(sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, 'PacChaseLogo.ino'), encoding='utf-8', errors='replace').read()

def arr(name):
    body = re.search(r'\b' + name + r'\[\]\s*PROGMEM\s*=\s*\{(.*?)\};', src, re.S).group(1)
    return [int(t, 0) for t in re.findall(r'0x[0-9A-Fa-f]+|\d+', body)]

out = ['#pragma once', '', '// Спрайти й палітри з PacChaseLogo.ino (MakerWorld #1133312), без змін.',
       '// Спрайти 112 пікселів завширшки: логотип 112x32, кадри погоні 112x16.', '']
for n in ['LogoSprite', 'PacSpriteM', 'PacSpriteT', 'PacSpriteB', 'PacSpriteM2', 'PacSpriteT2', 'PacSpriteB2']:
    v = arr(n)
    out.append(f'const uint8_t {n}[] = {{\n' + ',\n'.join(','.join(map(str, v[i:i + 112])) for i in range(0, len(v), 112)) + '\n};')
for n in ['LogoPalette', 'PacPaletteM', 'PacPaletteT', 'PacPaletteB', 'PacPaletteM2', 'PacPaletteT2', 'PacPaletteB2']:
    out.append(f'const uint32_t {n}[] = {{' + ', '.join('0x%06x' % c for c in arr(n)) + '};')
open(os.path.join(here, '..', 'pacman.h'), 'w').write('\n'.join(out) + '\n')
