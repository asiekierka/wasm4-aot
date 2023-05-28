PAL_OFFSET_FINE = 1

for i in range(0, 256):
	col_0 = (PAL_OFFSET_FINE + (i & 0x03)) * 17
	col_1 = (PAL_OFFSET_FINE + ((i>>2) & 0x03)) * 17
	col_2 = (PAL_OFFSET_FINE + ((i>>4) & 0x03)) * 17
	col_3 = (PAL_OFFSET_FINE + ((i>>6) & 0x03)) * 17
	print("0x%02X%02X%02X%02X," % (col_3, col_2, col_1, col_0))
