import sys

sample_len = int(sys.argv[1])
sample_min = int(sys.argv[2])
sample_max = int(sys.argv[3])

for i in range(0, sample_len):
	phase = float(i) / (sample_len / 2)
	if phase > 1:
		phase = 2 - phase
	phase = sample_min + int((sample_max - sample_min) * phase + 0.5)
	print("%d, " % phase, end="")
