#!/bin/zsh
make CCMODE=64 all
for chunksize in 0 512 4096 8192 65536 262144
do
	make CCMODE=64 CHUNK=${chunksize} check | tee chunk-${chunksize}.log
done
