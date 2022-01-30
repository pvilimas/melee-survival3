from math import floor
from os import system
from pprint import pprint
from time import sleep
from typing import NewType, Tuple, Type
import PIL.Image as Image

START = [15, 148, 0]
END = [219, 11, 11]

def select(*args):
  return [[args[i], args[i+1]] for i in range(len(args)-1)]

def make_color(color):
    return tuple([int(round(x, 0)) for x in color])

def gradient(color_start, color_end, count=20):
  def make_gradient(start, end, count=20) -> list:
    l = list()
    def gradient(_min, _max, count) -> int:
      return (_max - _min) / count

    curr = start
    g = gradient(start, end, count)

    for i in range(count):
        l.append(curr)
        curr += g
    l.append(curr)
    return l  
  
  gradients = [make_gradient(x, y) for x, y in zip(color_start, color_end)]
  return list(zip(*[make_color(c) for c in gradients]))

def display_gradient(start, end):
  for c in gradient(start, end):
    curr = make_color(c)
    print(curr)
    #Image.new('RGB', (100, 100), make_color(c)).show()
    #sleep(0.3)

colors = select(
  [15, 148, 0],
  [191, 186, 31],
  [219, 11, 11],
)
for c in colors:
  display_gradient(*c)
  system('killall Preview')
  sleep(0.3)

