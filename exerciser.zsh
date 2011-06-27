#!/bin/zsh
make all
for chunksize in 0 512 4096 8192 65536 262144
do
	make CHUNK=${chunksize} check | tee chunk-${chunksize}.log
done
