#	["ベンチマークの名前", "オリジナルの実行ファイル名", "入力サイズ", "引数", [
#		["出力ファイル名 (-)は標準出力", "正しい出力が書かれたファイルの名称 (比較用)"], 
#	]], 
$benchCmds = [
	["164.gzip", "gzip", "test", "input.compressed 2", [
	]], 
	["175.vpr", "vpr", "test", "net.in arch.in place.out dum.out -nodisp -place_only -init_t 5 -exit_t 0.005 -alpha_t 0.9412 -inner_num 2", [
		["-", ""], 
	]], 
	["175.vpr", "vpr", "test", "net.in arch.in place.in route.out -nodisp -route_only -route_chan_width 15 -pres_fac_mult 2 -acc_fac 1 -first_iter_pres_fac 4 -initial_pres_fac 8", [
		["-", ""], 
	]], 
	["176.gcc", "cc1", "test", "cccp.i -o cccp.s", [
	]], 
	["181.mcf", "mcf", "test", "inp.in", [
	]], 
	["186.crafty", "crafty", "test", " < crafty.in", [
	]], 
	["197.parser", "parser", "test", "2.1.dict -batch < test.in", [
	]], 
	["252.eon", "eon", "test", "chair.control.cook chair.camera chair.surfaces chair.cook.ppm ppm pixels_out.cook", [
		["-", ""], 
	]], 
	["252.eon", "eon", "test", "chair.control.rushmeier chair.camera chair.surfaces chair.rushmeier.ppm ppm pixels_out.rushmeier", [
		["-", ""], 
	]], 
	["252.eon", "eon", "test", "chair.control.kajiya chair.camera chair.surfaces chair.kajiya.ppm ppm pixels_out.kajiya", [
		["-", ""], 
	]], 
	["253.perlbmk", "perlbmk", "test", "-I. -I./lib test.pl < test.in", [
	]], 
	["254.gap", "gap", "test", "-l ./ -q -m 64M < test.in", [
	]], 
	["255.vortex", "vortex", "test", "lendian.raw", [
	]], 
	["256.bzip2", "bzip2", "test", "input.random 2", [
	]], 
	["300.twolf", "twolf", "test", "test", [
	]], 

	["164.gzip", "gzip", "train", "input.combined 32", [
	]], 
	["175.vpr", "vpr", "train", "net.in arch.in place.out dum.out -nodisp -place_only -init_t 5 -exit_t 0.005 -alpha_t 0.9412 -inner_num 2", [
		["-", ""], 
	]], 
	["175.vpr", "vpr", "train", "net.in arch.in place.in route.out -nodisp -route_only -route_chan_width 15 -pres_fac_mult 2 -acc_fac 1 -first_iter_pres_fac 4 -initial_pres_fac 8", [
		["-", ""], 
	]], 
	["176.gcc", "cc1", "train", "cp-decl.i -o cp-decl.s", [
	]], 
	["181.mcf", "mcf", "train", "inp.in", [
	]], 
	["186.crafty", "crafty", "train", " < crafty.in", [
	]], 
	["197.parser", "parser", "train", "2.1.dict -batch < train.in", [
	]], 
	["252.eon", "eon", "train", "chair.control.cook chair.camera chair.surfaces chair.cook.ppm ppm pixels_out.cook", [
		["-", ""], 
	]], 
	["252.eon", "eon", "train", "chair.control.rushmeier chair.camera chair.surfaces chair.rushmeier.ppm ppm pixels_out.rushmeier", [
		["-", ""], 
	]], 
	["252.eon", "eon", "train", "chair.control.kajiya chair.camera chair.surfaces chair.kajiya.ppm ppm pixels_out.kajiya", [
		["-", ""], 
	]], 
	["253.perlbmk", "perlbmk", "train", "-I./lib diffmail.pl 2 350 15 24 23 150", [
		["-", ""], 
	]], 
	["253.perlbmk", "perlbmk", "train", "-I./lib perfect.pl b 3", [
		["-", ""], 
	]], 
	["253.perlbmk", "perlbmk", "train", "-I. -I./lib scrabbl.pl < scrabbl.in", [
		["-", ""], 
	]], 
	["254.gap", "gap", "train", "-l ./ -q -m 128M < train.in", [
	]], 
	["255.vortex", "vortex", "train", "lendian.raw", [
	]], 
	["256.bzip2", "bzip2", "train", "input.compressed 8", [
	]], 
	["300.twolf", "twolf", "train", "train", [
	]], 

	["164.gzip", "gzip", "ref", "input.source 60", [
		["-", ""], 
	]], 
	["164.gzip", "gzip", "ref", "input.log 60", [
		["-", ""], 
	]], 
	["164.gzip", "gzip", "ref", "input.graphic 60", [
		["-", ""], 
	]], 
	["164.gzip", "gzip", "ref", "input.random 60", [
		["-", ""], 
	]], 
	["164.gzip", "gzip", "ref", "input.program 60", [
		["-", ""], 
	]], 
	["175.vpr", "vpr", "ref", "net.in arch.in place.out dum.out -nodisp -place_only -init_t 5 -exit_t 0.005 -alpha_t 0.9412 -inner_num 2", [
		["-", ""], 
	]], 
	["175.vpr", "vpr", "ref", "net.in arch.in place.in route.out -nodisp -route_only -route_chan_width 15 -pres_fac_mult 2 -acc_fac 1 -first_iter_pres_fac 4 -initial_pres_fac 8", [
		["-", ""], 
	]], 
	["176.gcc", "cc1", "ref", "166.i -o 166.s", [
		["-", ""], 
	]], 
	["176.gcc", "cc1", "ref", "200.i -o 200.s", [
		["-", ""], 
	]], 
	["176.gcc", "cc1", "ref", "expr.i -o expr.s", [
		["-", ""], 
	]], 
	["176.gcc", "cc1", "ref", "integrate.i -o integrate.s", [
		["-", ""], 
	]], 
	["176.gcc", "cc1", "ref", "scilab.i -o scilab.s", [
		["-", ""], 
	]], 
	["181.mcf", "mcf", "ref", "inp.in", [
	]], 
	["186.crafty", "crafty", "ref", " < crafty.in", [
	]], 
	["197.parser", "parser", "ref", "2.1.dict -batch < ref.in", [
	]], 
	["252.eon", "eon", "ref", "chair.control.cook chair.camera chair.surfaces chair.cook.ppm ppm pixels_out.cook", [
		["-", ""], 
	]], 
	["252.eon", "eon", "ref", "chair.control.rushmeier chair.camera chair.surfaces chair.rushmeier.ppm ppm pixels_out.rushmeier", [
		["-", ""], 
	]], 
	["252.eon", "eon", "ref", "chair.control.kajiya chair.camera chair.surfaces chair.kajiya.ppm ppm pixels_out.kajiya", [
		["-", ""], 
	]], 
	["253.perlbmk", "perlbmk", "ref", "-I./lib diffmail.pl 2 550 15 24 23 100", [
		["-", ""], 
	]], 
	["253.perlbmk", "perlbmk", "ref", "-I. -I./lib makerand.pl", [
		["-", ""], 
	]], 
	["253.perlbmk", "perlbmk", "ref", "-I./lib perfect.pl b 3 m 4", [
		["-", ""], 
	]], 
	["253.perlbmk", "perlbmk", "ref", "-I./lib splitmail.pl 850 5 19 18 1500", [
		["-", ""], 
	]], 
	["253.perlbmk", "perlbmk", "ref", "-I./lib splitmail.pl 704 12 26 16 836", [
		["-", ""], 
	]], 
	["253.perlbmk", "perlbmk", "ref", "-I./lib splitmail.pl 535 13 25 24 1091", [
		["-", ""], 
	]], 
	["253.perlbmk", "perlbmk", "ref", "-I./lib splitmail.pl 957 12 23 26 1014", [
		["-", ""], 
	]], 
	["254.gap", "gap", "ref", "-l ./ -q -m 192M < ref.in", [
	]], 
	["255.vortex", "vortex", "ref", "lendian1.raw", [
		["-", ""], 
	]], 
	["255.vortex", "vortex", "ref", "lendian2.raw", [
		["-", ""], 
	]], 
	["255.vortex", "vortex", "ref", "lendian3.raw", [
		["-", ""], 
	]], 
	["256.bzip2", "bzip2", "ref", "input.source 58", [
		["-", ""], 
	]], 
	["256.bzip2", "bzip2", "ref", "input.graphic 58", [
		["-", ""], 
	]], 
	["256.bzip2", "bzip2", "ref", "input.program 58", [
		["-", ""], 
	]], 
	["300.twolf", "twolf", "ref", "ref", [
	]], 
]
