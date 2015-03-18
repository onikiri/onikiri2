#	["ベンチマークの名前", "オリジナルの実行ファイル名", "入力サイズ", "引数", [
#		["出力ファイル名 (-)は標準出力", "正しい出力が書かれたファイルの名称 (比較用)"], 
#	]], 
$benchCmds = [
	["410.bwaves", "bwaves", "test", "", [
		["bwaves.out", "410.bwaves/data/test/output/bwaves.out"], 
		["bwaves2.out", "410.bwaves/data/test/output/bwaves2.out"], 
		["bwaves3.out", "410.bwaves/data/test/output/bwaves3.out"], 
	]], 
	["416.gamess", "gamess", "test", " < exam29.config", [
		["-", "416.gamess/data/test/output/exam29.out"], 
	]], 
	["433.milc", "milc", "test", " < su3imp.in", [
		["-", "433.milc/data/test/output/su3imp.out"], 
	]], 
	["434.zeusmp", "zeusmp", "test", "", [
		["tsl000aa", "434.zeusmp/data/test/output/tsl000aa"], 
	]], 
	["435.gromacs", "gromacs", "test", "-silent -deffnm gromacs -nice 0", [
		["gromacs.out", "435.gromacs/data/test/output/gromacs.out"], 
	]], 
	["436.cactusADM", "cactusADM", "test", "benchADM.par", [
		["-", "436.cactusADM/data/test/output/benchADM.out"], 
	]], 
	["437.leslie3d", "leslie3d", "test", " < leslie3d.in", [
		["leslie3d.out", "437.leslie3d/data/test/output/leslie3d.out"], 
	]], 
	["444.namd", "namd", "test", "--input namd.input --iterations 1 --output namd.out ", [
		["namd.out", "444.namd/data/test/output/namd.out"], 
	]], 
	["447.dealII", "dealII", "test", "8", [
		["grid-8.eps", "447.dealII/data/test/output/grid-8.eps"], 
		["solution-3.gmv", "447.dealII/data/test/output/solution-3.gmv"], 
		["solution-4.gmv", "447.dealII/data/test/output/solution-4.gmv"], 
		["solution-1.gmv", "447.dealII/data/test/output/solution-1.gmv"], 
		["grid-6.eps", "447.dealII/data/test/output/grid-6.eps"], 
		["grid-7.eps", "447.dealII/data/test/output/grid-7.eps"], 
		["grid-0.eps", "447.dealII/data/test/output/grid-0.eps"], 
		["-", "447.dealII/data/test/output/log"], 
		["solution-2.gmv", "447.dealII/data/test/output/solution-2.gmv"], 
		["grid-1.eps", "447.dealII/data/test/output/grid-1.eps"], 
		["solution-7.gmv", "447.dealII/data/test/output/solution-7.gmv"], 
		["solution-5.gmv", "447.dealII/data/test/output/solution-5.gmv"], 
		["grid-4.eps", "447.dealII/data/test/output/grid-4.eps"], 
		["solution-6.gmv", "447.dealII/data/test/output/solution-6.gmv"], 
		["solution-0.gmv", "447.dealII/data/test/output/solution-0.gmv"], 
		["grid-5.eps", "447.dealII/data/test/output/grid-5.eps"], 
		["grid-2.eps", "447.dealII/data/test/output/grid-2.eps"], 
		["solution-8.gmv", "447.dealII/data/test/output/solution-8.gmv"], 
		["grid-3.eps", "447.dealII/data/test/output/grid-3.eps"], 
	]], 
	["450.soplex", "soplex", "test", "-m10000 test.mps", [
		["-", "450.soplex/data/test/output/test.out"], 
		["test.stderr", "450.soplex/data/test/output/test.stderr"], 
	]], 
	["453.povray", "povray", "test", "SPEC-benchmark-test.ini", [
		["SPEC-benchmark.tga", "453.povray/data/test/output/SPEC-benchmark.tga"], 
		["SPEC-benchmark.log", "453.povray/data/test/output/SPEC-benchmark.log"], 
	]], 
	["454.calculix", "calculix", "test", "-i  beampic", [
		["beampic.dat", "454.calculix/data/test/output/beampic.dat"], 
		["SPECtestformatmodifier_z.txt", "454.calculix/data/test/output/SPECtestformatmodifier_z.txt"], 
	]], 
	["459.GemsFDTD", "GemsFDTD", "test", "", [
		["sphere_td.nft", "459.GemsFDTD/data/test/output/sphere_td.nft"], 
	]], 
	["465.tonto", "tonto", "test", "", [
		["stdout", "465.tonto/data/test/output/stdout"], 
	]], 
	["470.lbm", "lbm", "test", "20 reference.dat 0 1 100_100_130_cf_a.of", [
		["-", "470.lbm/data/test/output/lbm.out"], 
	]], 
	["481.wrf", "wrf", "test", "", [
		["-", "481.wrf/data/test/output/rsl.out.0000"], 
	]], 
	["482.sphinx3", "sphinx_livepretend", "test", "ctlfile . args.an4", [
		["total_considered.out", "482.sphinx3/data/test/output/total_considered.out"], 
		["considered.out", "482.sphinx3/data/test/output/considered.out"], 
		["-", "482.sphinx3/data/test/output/an4.log"], 
	]], 

	["410.bwaves", "bwaves", "train", "", [
		["bwaves.out", "410.bwaves/data/train/output/bwaves.out"], 
		["bwaves2.out", "410.bwaves/data/train/output/bwaves2.out"], 
		["bwaves3.out", "410.bwaves/data/train/output/bwaves3.out"], 
	]], 
	["416.gamess", "gamess", "train", " < h2ocu2+.energy.config", [
		["-", "416.gamess/data/train/output/h2ocu2+.energy.out"], 
	]], 
	["433.milc", "milc", "train", " < su3imp.in", [
		["-", "433.milc/data/train/output/su3imp.out"], 
	]], 
	["434.zeusmp", "zeusmp", "train", "", [
		["tsl000aa", "434.zeusmp/data/train/output/tsl000aa"], 
	]], 
	["435.gromacs", "gromacs", "train", "-silent -deffnm gromacs -nice 0", [
		["gromacs.out", "435.gromacs/data/train/output/gromacs.out"], 
	]], 
	["436.cactusADM", "cactusADM", "train", "benchADM.par", [
		["-", "436.cactusADM/data/train/output/benchADM.out"], 
	]], 
	["437.leslie3d", "leslie3d", "train", " < leslie3d.in", [
		["leslie3d.out", "437.leslie3d/data/train/output/leslie3d.out"], 
	]], 
	["444.namd", "namd", "train", "--input namd.input --iterations 1 --output namd.out ", [
		["namd.out", "444.namd/data/train/output/namd.out"], 
	]], 
	["447.dealII", "dealII", "train", "10", [
		["solution-9.gmv", "447.dealII/data/train/output/solution-9.gmv"], 
		["grid-8.eps", "447.dealII/data/train/output/grid-8.eps"], 
		["grid-9.eps", "447.dealII/data/train/output/grid-9.eps"], 
		["solution-3.gmv", "447.dealII/data/train/output/solution-3.gmv"], 
		["solution-4.gmv", "447.dealII/data/train/output/solution-4.gmv"], 
		["solution-1.gmv", "447.dealII/data/train/output/solution-1.gmv"], 
		["grid-6.eps", "447.dealII/data/train/output/grid-6.eps"], 
		["grid-7.eps", "447.dealII/data/train/output/grid-7.eps"], 
		["grid-0.eps", "447.dealII/data/train/output/grid-0.eps"], 
		["-", "447.dealII/data/train/output/log"], 
		["grid-10.eps", "447.dealII/data/train/output/grid-10.eps"], 
		["solution-2.gmv", "447.dealII/data/train/output/solution-2.gmv"], 
		["grid-1.eps", "447.dealII/data/train/output/grid-1.eps"], 
		["solution-7.gmv", "447.dealII/data/train/output/solution-7.gmv"], 
		["solution-5.gmv", "447.dealII/data/train/output/solution-5.gmv"], 
		["grid-4.eps", "447.dealII/data/train/output/grid-4.eps"], 
		["solution-6.gmv", "447.dealII/data/train/output/solution-6.gmv"], 
		["solution-0.gmv", "447.dealII/data/train/output/solution-0.gmv"], 
		["grid-5.eps", "447.dealII/data/train/output/grid-5.eps"], 
		["grid-2.eps", "447.dealII/data/train/output/grid-2.eps"], 
		["solution-10.gmv", "447.dealII/data/train/output/solution-10.gmv"], 
		["solution-8.gmv", "447.dealII/data/train/output/solution-8.gmv"], 
		["grid-3.eps", "447.dealII/data/train/output/grid-3.eps"], 
	]], 
	["450.soplex", "soplex", "train", "-s1 -e -m5000 pds-20.mps", [
		["-", "450.soplex/data/train/output/pds-20.mps.out"], 
	]], 
	["450.soplex", "soplex", "train", "-m1200 train.mps", [
		["-", "450.soplex/data/train/output/train.out"], 
	]], 
	["453.povray", "povray", "train", "SPEC-benchmark-train.ini", [
		["SPEC-benchmark.tga", "453.povray/data/train/output/SPEC-benchmark.tga"], 
		["SPEC-benchmark.log", "453.povray/data/train/output/SPEC-benchmark.log"], 
	]], 
	["454.calculix", "calculix", "train", "-i  stairs", [
		["stairs.dat", "454.calculix/data/train/output/stairs.dat"], 
		["SPECtestformatmodifier_z.txt", "454.calculix/data/train/output/SPECtestformatmodifier_z.txt"], 
	]], 
	["459.GemsFDTD", "GemsFDTD", "train", "", [
		["sphere_td.nft", "459.GemsFDTD/data/train/output/sphere_td.nft"], 
	]], 
	["465.tonto", "tonto", "train", "", [
		["stdout", "465.tonto/data/train/output/stdout"], 
	]], 
	["470.lbm", "lbm", "train", "300 reference.dat 0 1 100_100_130_cf_b.of", [
		["-", "470.lbm/data/train/output/lbm.out"], 
	]], 
	["481.wrf", "wrf", "train", "", [
		["-", "481.wrf/data/train/output/rsl.out.0000"], 
	]], 
	["482.sphinx3", "sphinx_livepretend", "train", "ctlfile . args.an4", [
		["total_considered.out", "482.sphinx3/data/train/output/total_considered.out"], 
		["considered.out", "482.sphinx3/data/train/output/considered.out"], 
		["-", "482.sphinx3/data/train/output/an4.log"], 
	]], 

	["410.bwaves", "bwaves", "ref", "", [
		["bwaves.out", "410.bwaves/data/ref/output/bwaves.out"], 
		["bwaves2.out", "410.bwaves/data/ref/output/bwaves2.out"], 
		["bwaves3.out", "410.bwaves/data/ref/output/bwaves3.out"], 
	]], 
	["416.gamess", "gamess", "ref", " < cytosine.2.config", [
		["-", "416.gamess/data/ref/output/cytosine.2.out"], 
	]], 
	["416.gamess", "gamess", "ref", " < h2ocu2+.gradient.config", [
		["-", "416.gamess/data/ref/output/h2ocu2+.gradient.out"], 
	]], 
	["416.gamess", "gamess", "ref", " < triazolium.config", [
		["-", "416.gamess/data/ref/output/triazolium.out"], 
	]], 
	["433.milc", "milc", "ref", " < su3imp.in", [
		["-", "433.milc/data/ref/output/su3imp.out"], 
	]], 
	["434.zeusmp", "zeusmp", "ref", "", [
		["tsl000aa", "434.zeusmp/data/ref/output/tsl000aa"], 
	]], 
	["435.gromacs", "gromacs", "ref", "-silent -deffnm gromacs -nice 0", [
		["gromacs.out", "435.gromacs/data/ref/output/gromacs.out"], 
	]], 
	["436.cactusADM", "cactusADM", "ref", "benchADM.par", [
		["-", "436.cactusADM/data/ref/output/benchADM.out"], 
	]], 
	["437.leslie3d", "leslie3d", "ref", " < leslie3d.in", [
		["leslie3d.out", "437.leslie3d/data/ref/output/leslie3d.out"], 
	]], 
	["444.namd", "namd", "ref", "--input namd.input --iterations 38 --output namd.out ", [
		["namd.out", "444.namd/data/ref/output/namd.out"], 
	]], 
	["447.dealII", "dealII", "ref", "23", [
		["solution-9.gmv", "447.dealII/data/ref/output/solution-9.gmv"], 
		["solution-11.gmv", "447.dealII/data/ref/output/solution-11.gmv"], 
		["grid-21.eps", "447.dealII/data/ref/output/grid-21.eps"], 
		["grid-17.eps", "447.dealII/data/ref/output/grid-17.eps"], 
		["grid-8.eps", "447.dealII/data/ref/output/grid-8.eps"], 
		["solution-23.gmv", "447.dealII/data/ref/output/solution-23.gmv"], 
		["solution-19.gmv", "447.dealII/data/ref/output/solution-19.gmv"], 
		["grid-9.eps", "447.dealII/data/ref/output/grid-9.eps"], 
		["grid-22.eps", "447.dealII/data/ref/output/grid-22.eps"], 
		["grid-18.eps", "447.dealII/data/ref/output/grid-18.eps"], 
		["solution-3.gmv", "447.dealII/data/ref/output/solution-3.gmv"], 
		["solution-13.gmv", "447.dealII/data/ref/output/solution-13.gmv"], 
		["grid-20.eps", "447.dealII/data/ref/output/grid-20.eps"], 
		["grid-12.eps", "447.dealII/data/ref/output/grid-12.eps"], 
		["solution-4.gmv", "447.dealII/data/ref/output/solution-4.gmv"], 
		["solution-1.gmv", "447.dealII/data/ref/output/solution-1.gmv"], 
		["grid-6.eps", "447.dealII/data/ref/output/grid-6.eps"], 
		["solution-21.gmv", "447.dealII/data/ref/output/solution-21.gmv"], 
		["solution-14.gmv", "447.dealII/data/ref/output/solution-14.gmv"], 
		["grid-7.eps", "447.dealII/data/ref/output/grid-7.eps"], 
		["grid-0.eps", "447.dealII/data/ref/output/grid-0.eps"], 
		["solution-22.gmv", "447.dealII/data/ref/output/solution-22.gmv"], 
		["-", "447.dealII/data/ref/output/log"], 
		["grid-10.eps", "447.dealII/data/ref/output/grid-10.eps"], 
		["solution-2.gmv", "447.dealII/data/ref/output/solution-2.gmv"], 
		["grid-1.eps", "447.dealII/data/ref/output/grid-1.eps"], 
		["solution-12.gmv", "447.dealII/data/ref/output/solution-12.gmv"], 
		["grid-15.eps", "447.dealII/data/ref/output/grid-15.eps"], 
		["grid-11.eps", "447.dealII/data/ref/output/grid-11.eps"], 
		["solution-7.gmv", "447.dealII/data/ref/output/solution-7.gmv"], 
		["solution-17.gmv", "447.dealII/data/ref/output/solution-17.gmv"], 
		["grid-23.eps", "447.dealII/data/ref/output/grid-23.eps"], 
		["grid-16.eps", "447.dealII/data/ref/output/grid-16.eps"], 
		["solution-18.gmv", "447.dealII/data/ref/output/solution-18.gmv"], 
		["grid-13.eps", "447.dealII/data/ref/output/grid-13.eps"], 
		["solution-5.gmv", "447.dealII/data/ref/output/solution-5.gmv"], 
		["grid-4.eps", "447.dealII/data/ref/output/grid-4.eps"], 
		["solution-15.gmv", "447.dealII/data/ref/output/solution-15.gmv"], 
		["grid-14.eps", "447.dealII/data/ref/output/grid-14.eps"], 
		["solution-6.gmv", "447.dealII/data/ref/output/solution-6.gmv"], 
		["solution-0.gmv", "447.dealII/data/ref/output/solution-0.gmv"], 
		["grid-5.eps", "447.dealII/data/ref/output/grid-5.eps"], 
		["solution-20.gmv", "447.dealII/data/ref/output/solution-20.gmv"], 
		["solution-16.gmv", "447.dealII/data/ref/output/solution-16.gmv"], 
		["grid-19.eps", "447.dealII/data/ref/output/grid-19.eps"], 
		["grid-2.eps", "447.dealII/data/ref/output/grid-2.eps"], 
		["solution-10.gmv", "447.dealII/data/ref/output/solution-10.gmv"], 
		["solution-8.gmv", "447.dealII/data/ref/output/solution-8.gmv"], 
		["grid-3.eps", "447.dealII/data/ref/output/grid-3.eps"], 
	]], 
	["450.soplex", "soplex", "ref", "-s1 -e -m45000 pds-50.mps", [
		["-", "450.soplex/data/ref/output/pds-50.mps.out"], 
	]], 
	["450.soplex", "soplex", "ref", "-m3500 ref.mps", [
		["-", "450.soplex/data/ref/output/ref.out"], 
	]], 
	["453.povray", "povray", "ref", "SPEC-benchmark-ref.ini", [
		["SPEC-benchmark.tga", "453.povray/data/ref/output/SPEC-benchmark.tga"], 
		["SPEC-benchmark.log", "453.povray/data/ref/output/SPEC-benchmark.log"], 
	]], 
	["454.calculix", "calculix", "ref", "-i  hyperviscoplastic", [
		["hyperviscoplastic.dat", "454.calculix/data/ref/output/hyperviscoplastic.dat"], 
		["SPECtestformatmodifier_z.txt", "454.calculix/data/ref/output/SPECtestformatmodifier_z.txt"], 
	]], 
	["459.GemsFDTD", "GemsFDTD", "ref", "", [
		["sphere_td.nft", "459.GemsFDTD/data/ref/output/sphere_td.nft"], 
	]], 
	["465.tonto", "tonto", "ref", "", [
		["stdout", "465.tonto/data/ref/output/stdout"], 
	]], 
	["470.lbm", "lbm", "ref", "3000 reference.dat 0 0 100_100_130_ldc.of", [
		["-", "470.lbm/data/ref/output/lbm.out"], 
	]], 
	["481.wrf", "wrf", "ref", "", [
		["-", "481.wrf/data/ref/output/rsl.out.0000"], 
	]], 
	["482.sphinx3", "sphinx_livepretend", "ref", "ctlfile . args.an4", [
		["total_considered.out", "482.sphinx3/data/ref/output/total_considered.out"], 
		["considered.out", "482.sphinx3/data/ref/output/considered.out"], 
		["-", "482.sphinx3/data/ref/output/an4.log"], 
	]], 
]
