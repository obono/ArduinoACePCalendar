#!/usr/bin/python

import numpy as np
from PIL import Image

filename = 'font.gif'
img = Image.open(filename)

img_data = np.asarray(img).reshape(-1, 4)
for i in range(20):
	max_y = 32
	if i < 10:
		max_y = 48
	data = []
	for y in range(max_y):
		for x in range(0, 32, 4):
			b = 0
			for xx in range(4):
				b += img.getpixel((i * 32 + x + xx, y)) << (xx * 2)
			data.append(b)
	counter = 0
	out_str = ""
	for b in data:
		out_str += '0x' + format(b, '02X') + ','
		counter += 1
		if counter == 16:
			out_str += '\n'
			counter = 0
		else:
			out_str += ' '
	print('// Image %d' % i)
	print(out_str)

