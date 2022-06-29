#!/bin/bash

for k in  11 12 13 14 15 16 17 18;do
	is=$(ping -c 1 192.168.1.1 | grep 100)
	[[ $is ]] && continue
	
	ssh "192.168.1.${k}"




done


