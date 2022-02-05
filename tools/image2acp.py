#!/usr/bin/python

import numpy as np
import os
import pathlib
import re
import sys
import subprocess
from PIL import Image

def convert2acp(filepath, size_option, ascii_option):

	magick_exe = 'magick'
	work_filename = 'work.gif'

	print('Processing "%s"...' % filepath)

	if size_option == 'keep':
		resize_params = ''
	else:
		resize_params = '-resize %s -gravity center -background white -extent %s' % (size_option, size_option)
	subprocess.run('%s %s %s -dither FloydSteinberg -remap palette.gif %s' % (magick_exe, filepath, resize_params, work_filename))

	img = Image.open(work_filename)
	img_pal = np.array(img.getpalette()).reshape(-1, 3)
	master_pal = np.array([[0,0,0], [255,255,255], [0,128,0], [0,0,255], [255,0,0], [255,255,0], [255,170,0]])

	index_map = list(range(7))
	for src_index in range(7):
		for target_index in range(7):
			if np.array_equal(img_pal[src_index], master_pal[target_index]):
				index_map[src_index] = target_index

	img_data = np.asarray(img).reshape(-1, 2)
	acep_data = []
	for pair in img_data:
		acep_data.append(index_map[pair[0]] * 16 + index_map[pair[1]])
	img.close()
	os.remove(work_filename)

	outputpath_base = pathlib.PurePath(filepath).stem
	if ascii_option:
		a = 0
		out_str = ""
		for b in acep_data:
			out_str += '0x' + format(b, '02X') + ','
			a += 1
			if a == 16:
				out_str += '\n'
				a = 0
			else:
				out_str += ' '
		with open(outputpath_base + '.txt', 'w') as f:
			f.write(out_str)
	else:
		outputpath = pathlib.PurePath(filepath).stem + '.acp'
		with open(outputpath_base + '.acp', 'wb') as f:
			f.write(bytes(acep_data))

	return

if __name__ == '__main__':

	ascii_option = False
	size_option = '600x448'
	target_paths = []

	argvs = sys.argv
	for arg in argvs[1:]:
		if arg == '-ascii':
			ascii_option = True
		elif arg == '-keep':
			size_option = 'keep'
		elif re.compile('^-\d+x\d+$').search(arg):
			size_option = arg[1:]
		else:
			target_paths.append(arg)

	if len(target_paths) == 0:
		print('Usage: %s [-ascii] [-keep] [-WxH] filename ...' % argvs[0])
		quit()

	for filepath in target_paths:
		convert2acp(filepath, size_option, ascii_option)

	print('Done!');
