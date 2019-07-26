#!/bin/bash
mods="blue_keeper green_keeper"

mkdir -p mods
for mod in $mods; do
	mkdir -p mods/$mod
	rm mods/$mod/*
	cp keeperrl/data_free/game_config/vanilla/* mods/$mod/
	patch -f mods/$mod/creatures.txt mods/$mod.patch
done
