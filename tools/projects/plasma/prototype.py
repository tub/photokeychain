import psyco
psyco.full()
import pygame
from math import *

pygame.init()
screen = pygame.display.set_mode((128, 128))
surface = pygame.surface.Surface((128,128))

frame = 0
quit = False

def rond(x):
    return int(x)

def sn1(x):
    x = rond(x)
    return rond(sin(x/256.0*2.0*pi + 0.9)*63.0 + sin(x/128.0*2.0*pi + 0.2)*63.0 + 127.0)

def sn2(x):
    x = rond(x)
    return rond(sin(x/128.0*2.0*pi + 0.3)*63.0 + sin(x/256.0*2.0*pi + 0.7)*63.0 + 127.0)

def plasma(f, surface):
    for y in range(128):
        fy1 = sn1(sn1(f*2+y/2)/2 + sn2(f*4-y/4)/8 - f*2)
        fy2 = sn2(sn2(f*2-y/4)/4 + sn2(f*4+y/2)/4 + f*4)
        fy = fy1 - fy2
        for x in range(128):
            v = int(sn1(fy1-x)/2+sn2(x+fy2)/2)
            r = v
            g = v
            b = v
            surface.set_at( (x,y),(r,g,b,0))

while not quit:
    plasma(frame, surface)
    screen.blit( surface, (0,0) )
    pygame.display.flip()
    frame += 1
    for event in pygame.event.get():
        if event.type == pygame.KEYDOWN:
            quit = True                


