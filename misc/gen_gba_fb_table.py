for i in range(0, 256):
	col_0 = 0x10 + (i & 0x03)
	col_1 = 0x10 + ((i>>2) & 0x03)
	col_2 = 0x10 + ((i>>4) & 0x03)
	col_3 = 0x10 + ((i>>6) & 0x03)
	print("0x%02X%02X%02X%02X," % (col_3, col_2, col_1, col_0))
