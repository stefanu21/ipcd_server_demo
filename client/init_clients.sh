#!/bin/bash

for i in {0..200}
do
	echo "start client $i"
	./ipcd_client &
done
