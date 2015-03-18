#	["ベンチマークの名前", "オリジナルの実行ファイル名", "入力サイズ", "引数", [
#		["出力ファイル名 (-)は標準出力", "正しい出力が書かれたファイルの名称 (比較用)"], 
#	]], 
$benchCmds = [
	["400.perlbench", "perlbench", "test", "-I. -I./lib attrs.pl", [
		["-", "400.perlbench/data/test/output/attrs.out"], 
	]], 
	["400.perlbench", "perlbench", "test", "-I. -I./lib gv.pl", [
		["-", "400.perlbench/data/test/output/gv.out"], 
	]], 
	["400.perlbench", "perlbench", "test", "-I. -I./lib makerand.pl", [
		["-", "400.perlbench/data/test/output/makerand.out"], 
	]], 
	["400.perlbench", "perlbench", "test", "-I. -I./lib pack.pl", [
		["-", "400.perlbench/data/test/output/pack.out"], 
	]], 
	["400.perlbench", "perlbench", "test", "-I. -I./lib redef.pl", [
		["-", "400.perlbench/data/test/output/redef.out"], 
	]], 
	["400.perlbench", "perlbench", "test", "-I. -I./lib ref.pl", [
		["-", "400.perlbench/data/test/output/ref.out"], 
	]], 
	["400.perlbench", "perlbench", "test", "-I. -I./lib regmesg.pl", [
		["-", "400.perlbench/data/test/output/regmesg.out"], 
	]], 
	["400.perlbench", "perlbench", "test", "-I. -I./lib test.pl", [
		["-", ""], 
	]], 
	["401.bzip2", "bzip2", "test", "input.program 5", [
		["-", "401.bzip2/data/test/output/input.program.out"], 
	]], 
	["401.bzip2", "bzip2", "test", "dryer.jpg 2", [
		["-", "401.bzip2/data/test/output/dryer.jpg.out"], 
	]], 
	["403.gcc", "gcc", "test", "cccp.i -o cccp.s", [
		["cccp.s", "403.gcc/data/test/output/cccp.s"], 
	]], 
	["429.mcf", "mcf", "test", "inp.in", [
		["-", "429.mcf/data/test/output/inp.out"], 
		["mcf.out", "429.mcf/data/test/output/mcf.out"], 
	]], 
	["445.gobmk", "gobmk", "test", "--quiet --mode gtp < capture.tst", [
		["-", "445.gobmk/data/test/output/capture.out"], 
	]], 
	["445.gobmk", "gobmk", "test", "--quiet --mode gtp < connect.tst", [
		["-", "445.gobmk/data/test/output/connect.out"], 
	]], 
	["445.gobmk", "gobmk", "test", "--quiet --mode gtp < connect_rot.tst", [
		["-", "445.gobmk/data/test/output/connect_rot.out"], 
	]], 
	["445.gobmk", "gobmk", "test", "--quiet --mode gtp < connection.tst", [
		["-", "445.gobmk/data/test/output/connection.out"], 
	]], 
	["445.gobmk", "gobmk", "test", "--quiet --mode gtp < connection_rot.tst", [
		["-", "445.gobmk/data/test/output/connection_rot.out"], 
	]], 
	["445.gobmk", "gobmk", "test", "--quiet --mode gtp < cutstone.tst", [
		["-", "445.gobmk/data/test/output/cutstone.out"], 
	]], 
	["445.gobmk", "gobmk", "test", "--quiet --mode gtp < dniwog.tst", [
		["-", "445.gobmk/data/test/output/dniwog.out"], 
	]], 
	["456.hmmer", "hmmer", "test", "--fixed 0 --mean 325 --num 45000 --sd 200 --seed 0 bombesin.hmm", [
		["-", "456.hmmer/data/test/output/bombesin.out"], 
	]], 
	["458.sjeng", "sjeng", "test", "test.txt", [
		["-", "458.sjeng/data/test/output/test.out"], 
	]], 
	["462.libquantum", "libquantum", "test", "33 5", [
		["-", "462.libquantum/data/test/output/test.out"], 
	]], 
	["464.h264ref", "h264ref", "test", "-d foreman_test_encoder_baseline.cfg", [
		["-", "464.h264ref/data/test/output/foreman_test_baseline_encodelog.out"], 
		["foreman_test_baseline_leakybucketparam.cfg", "464.h264ref/data/test/output/foreman_test_baseline_leakybucketparam.cfg"], 
	]], 
	["471.omnetpp", "omnetpp", "test", "omnetpp.ini", [
		["omnetpp.sca", "471.omnetpp/data/test/output/omnetpp.sca"], 
		["-", "471.omnetpp/data/test/output/omnetpp.log"], 
	]], 
	["473.astar", "astar", "test", "lake.cfg", [
		["-", "473.astar/data/test/output/lake.out"], 
	]], 
	["483.xalancbmk", "Xalan", "test", "-v test.xml xalanc.xsl", [
		["-", "483.xalancbmk/data/test/output/test.out"], 
	]], 

	["400.perlbench", "perlbench", "train", "-I./lib diffmail.pl 2 550 15 24 23 100", [
		["-", "400.perlbench/data/train/output/diffmail.2.550.15.24.23.100.out"], 
	]], 
	["400.perlbench", "perlbench", "train", "-I./lib perfect.pl b 3", [
		["-", "400.perlbench/data/train/output/perfect.b.3.out"], 
	]], 
	["400.perlbench", "perlbench", "train", "-I. -I./lib scrabbl.pl < scrabbl.in", [
		["-", "400.perlbench/data/train/output/scrabbl.out"], 
	]], 
	["400.perlbench", "perlbench", "train", "-I./lib splitmail.pl 535 13 25 24 1091", [
		["-", "400.perlbench/data/train/output/splitmail.535.13.25.24.1091.out"], 
	]], 
	["400.perlbench", "perlbench", "train", "-I. -I./lib suns.pl", [
		["-", "400.perlbench/data/train/output/suns.out"], 
	]], 
	["401.bzip2", "bzip2", "train", "input.program 10", [
		["-", "401.bzip2/data/train/output/input.program.out"], 
	]], 
	["401.bzip2", "bzip2", "train", "byoudoin.jpg 5", [
		["-", "401.bzip2/data/train/output/byoudoin.jpg.out"], 
	]], 
	["401.bzip2", "bzip2", "train", "input.combined 80", [
		["-", "401.bzip2/data/train/output/input.combined.out"], 
	]], 
	["403.gcc", "gcc", "train", "integrate.i -o integrate.s", [
		["integrate.s", "403.gcc/data/train/output/integrate.s"], 
	]], 
	["429.mcf", "mcf", "train", "inp.in", [
		["-", "429.mcf/data/train/output/inp.out"], 
		["mcf.out", "429.mcf/data/train/output/mcf.out"], 
	]], 
	["445.gobmk", "gobmk", "train", "--quiet --mode gtp < arb.tst", [
		["-", "445.gobmk/data/train/output/arb.out"], 
	]], 
	["445.gobmk", "gobmk", "train", "--quiet --mode gtp < arend.tst", [
		["-", "445.gobmk/data/train/output/arend.out"], 
	]], 
	["445.gobmk", "gobmk", "train", "--quiet --mode gtp < arion.tst", [
		["-", "445.gobmk/data/train/output/arion.out"], 
	]], 
	["445.gobmk", "gobmk", "train", "--quiet --mode gtp < atari_atari.tst", [
		["-", "445.gobmk/data/train/output/atari_atari.out"], 
	]], 
	["445.gobmk", "gobmk", "train", "--quiet --mode gtp < blunder.tst", [
		["-", "445.gobmk/data/train/output/blunder.out"], 
	]], 
	["445.gobmk", "gobmk", "train", "--quiet --mode gtp < buzco.tst", [
		["-", "445.gobmk/data/train/output/buzco.out"], 
	]], 
	["445.gobmk", "gobmk", "train", "--quiet --mode gtp < nicklas2.tst", [
		["-", "445.gobmk/data/train/output/nicklas2.out"], 
	]], 
	["445.gobmk", "gobmk", "train", "--quiet --mode gtp < nicklas4.tst", [
		["-", "445.gobmk/data/train/output/nicklas4.out"], 
	]], 
	["456.hmmer", "hmmer", "train", "--fixed 0 --mean 425 --num 85000 --sd 300 --seed 0 leng100.hmm", [
		["-", "456.hmmer/data/train/output/leng100.out"], 
	]], 
	["458.sjeng", "sjeng", "train", "train.txt", [
		["-", "458.sjeng/data/train/output/train.out"], 
	]], 
	["462.libquantum", "libquantum", "train", "143 25", [
		["-", "462.libquantum/data/train/output/train.out"], 
	]], 
	["464.h264ref", "h264ref", "train", "-d foreman_train_encoder_baseline.cfg", [
		["-", "464.h264ref/data/train/output/foreman_train_baseline_encodelog.out"], 
		["foreman_train_baseline_leakybucketparam.cfg", "464.h264ref/data/train/output/foreman_train_baseline_leakybucketparam.cfg"], 
	]], 
	["471.omnetpp", "omnetpp", "train", "omnetpp.ini", [
		["omnetpp.sca", "471.omnetpp/data/train/output/omnetpp.sca"], 
		["-", "471.omnetpp/data/train/output/omnetpp.log"], 
	]], 
	["473.astar", "astar", "train", "BigLakes1024.cfg", [
		["-", "473.astar/data/train/output/BigLakes1024.out"], 
	]], 
	["473.astar", "astar", "train", "rivers1.cfg", [
		["-", "473.astar/data/train/output/rivers1.out"], 
	]], 
	["483.xalancbmk", "Xalan", "train", "-v allbooks.xml xalanc.xsl", [
		["-", "483.xalancbmk/data/train/output/train.out"], 
	]], 

	["400.perlbench", "perlbench", "ref", "-I./lib checkspam.pl 2500 5 25 11 150 1 1 1 1", [
		["-", "400.perlbench/data/ref/output/checkspam.2500.5.25.11.150.1.1.1.1.out"], 
	]], 
	["400.perlbench", "perlbench", "ref", "-I./lib diffmail.pl 4 800 10 17 19 300", [
		["-", "400.perlbench/data/ref/output/diffmail.4.800.10.17.19.300.out"], 
	]], 
	["400.perlbench", "perlbench", "ref", "-I./lib splitmail.pl 1600 12 26 16 4500", [
		["-", "400.perlbench/data/ref/output/splitmail.1600.12.26.16.4500.out"], 
	]], 
	["401.bzip2", "bzip2", "ref", "input.source 280", [
		["-", "401.bzip2/data/ref/output/input.source.out"], 
	]], 
	["401.bzip2", "bzip2", "ref", "chicken.jpg 30", [
		["-", "401.bzip2/data/ref/output/chicken.jpg.out"], 
	]], 
	["401.bzip2", "bzip2", "ref", "liberty.jpg 30", [
		["-", "401.bzip2/data/ref/output/liberty.jpg.out"], 
	]], 
	["401.bzip2", "bzip2", "ref", "input.program 280", [
		["-", "401.bzip2/data/ref/output/input.program.out"], 
	]], 
	["401.bzip2", "bzip2", "ref", "text.html 280", [
		["-", "401.bzip2/data/ref/output/text.html.out"], 
	]], 
	["401.bzip2", "bzip2", "ref", "input.combined 200", [
		["-", "401.bzip2/data/ref/output/input.combined.out"], 
	]], 
	["403.gcc", "gcc", "ref", "166.i -o 166.s", [
		["-", ""], 
	]], 
	["403.gcc", "gcc", "ref", "200.i -o 200.s", [
		["-", ""], 
	]], 
	["403.gcc", "gcc", "ref", "c-typeck.i -o c-typeck.s", [
		["-", ""], 
	]], 
	["403.gcc", "gcc", "ref", "cp-decl.i -o cp-decl.s", [
		["-", ""], 
	]], 
	["403.gcc", "gcc", "ref", "expr.i -o expr.s", [
		["-", ""], 
	]], 
	["403.gcc", "gcc", "ref", "expr2.i -o expr2.s", [
		["-", ""], 
	]], 
	["403.gcc", "gcc", "ref", "g23.i -o g23.s", [
		["-", ""], 
	]], 
	["403.gcc", "gcc", "ref", "s04.i -o s04.s", [
		["-", ""], 
	]], 
	["403.gcc", "gcc", "ref", "scilab.i -o scilab.s", [
		["-", ""], 
	]], 
	["429.mcf", "mcf", "ref", "inp.in", [
		["-", "429.mcf/data/ref/output/inp.out"], 
		["mcf.out", "429.mcf/data/ref/output/mcf.out"], 
	]], 
	["445.gobmk", "gobmk", "ref", "--quiet --mode gtp < 13x13.tst", [
		["-", "445.gobmk/data/ref/output/13x13.out"], 
	]], 
	["445.gobmk", "gobmk", "ref", "--quiet --mode gtp < nngs.tst", [
		["-", "445.gobmk/data/ref/output/nngs.out"], 
	]], 
	["445.gobmk", "gobmk", "ref", "--quiet --mode gtp < score2.tst", [
		["-", "445.gobmk/data/ref/output/score2.out"], 
	]], 
	["445.gobmk", "gobmk", "ref", "--quiet --mode gtp < trevorc.tst", [
		["-", "445.gobmk/data/ref/output/trevorc.out"], 
	]], 
	["445.gobmk", "gobmk", "ref", "--quiet --mode gtp < trevord.tst", [
		["-", "445.gobmk/data/ref/output/trevord.out"], 
	]], 
	["456.hmmer", "hmmer", "ref", "nph3.hmm swiss41", [
		["-", "456.hmmer/data/ref/output/nph3.out"], 
	]], 
	["456.hmmer", "hmmer", "ref", "--fixed 0 --mean 500 --num 500000 --sd 350 --seed 0 retro.hmm", [
		["-", "456.hmmer/data/ref/output/retro.out"], 
	]], 
	["458.sjeng", "sjeng", "ref", "ref.txt", [
		["-", "458.sjeng/data/ref/output/ref.out"], 
	]], 
	["462.libquantum", "libquantum", "ref", "1397 8", [
		["-", "462.libquantum/data/ref/output/ref.out"], 
	]], 
	["464.h264ref", "h264ref", "ref", "-d foreman_ref_encoder_baseline.cfg", [
		["-", "464.h264ref/data/ref/output/foreman_ref_baseline_encodelog.out"], 
	]], 
	["464.h264ref", "h264ref", "ref", "-d foreman_ref_encoder_main.cfg", [
		["-", "464.h264ref/data/ref/output/foreman_ref_main_encodelog.out"], 
	]], 
	["464.h264ref", "h264ref", "ref", "-d sss_encoder_main.cfg", [
		["-", "464.h264ref/data/ref/output/sss_main_encodelog.out"], 
	]], 
	["471.omnetpp", "omnetpp", "ref", "omnetpp.ini", [
		["omnetpp.sca", "471.omnetpp/data/ref/output/omnetpp.sca"], 
		["-", "471.omnetpp/data/ref/output/omnetpp.log"], 
	]], 
	["473.astar", "astar", "ref", "BigLakes2048.cfg", [
		["-", "473.astar/data/ref/output/BigLakes2048.out"], 
	]], 
	["473.astar", "astar", "ref", "rivers.cfg", [
		["-", "473.astar/data/ref/output/rivers.out"], 
	]], 
	["483.xalancbmk", "Xalan", "ref", "-v t5.xml xalanc.xsl", [
		["-", "483.xalancbmk/data/ref/output/ref.out"], 
	]], 
]
