#	["ベンチマークの名前", "オリジナルの実行ファイル名", "入力サイズ", "引数", [
#		["出力ファイル名 (-)は標準出力", "正しい出力が書かれたファイルの名称 (比較用)"], 
#	]], 
$benchCmds = [
	["168.wupwise", "wupwise", "test", "", [
	]], 
	["171.swim", "swim", "test", " < swim.in", [
	]], 
	["172.mgrid", "mgrid", "test", " < mgrid.in", [
	]], 
	["173.applu", "applu", "test", " < applu.in", [
	]], 
	["177.mesa", "mesa", "test", "-frames 10 -meshfile mesa.in -ppmfile mesa.ppm", [
	]], 
	["178.galgel", "galgel", "test", " < galgel.in", [
	]], 
	["179.art", "art", "test", "-scanfile c756hel.in -trainfile1 a10.img -stride 2 -startx 134 -starty 220 -endx 139 -endy 225 -objects 1", [
	]], 
	["183.equake", "equake", "test", " < inp.in", [
	]], 
	["187.facerec", "facerec", "test", " < test.in", [
	]], 
	["188.ammp", "ammp", "test", " < ammp.in", [
	]], 
	["189.lucas", "lucas", "test", " < lucas2.in", [
	]], 
	["191.fma3d", "fma3d", "test", "", [
	]], 
	["200.sixtrack", "sixtrack", "test", " < inp.in", [
	]], 
	["301.apsi", "apsi", "test", "", [
	]], 

	["168.wupwise", "wupwise", "train", "", [
	]], 
	["171.swim", "swim", "train", " < swim.in", [
	]], 
	["172.mgrid", "mgrid", "train", " < mgrid.in", [
	]], 
	["173.applu", "applu", "train", " < applu.in", [
	]], 
	["177.mesa", "mesa", "train", "-frames 500 -meshfile mesa.in -ppmfile mesa.ppm", [
	]], 
	["178.galgel", "galgel", "train", " < galgel.in", [
	]], 
	["179.art", "art", "train", "-scanfile c756hel.in -trainfile1 a10.img -stride 2 -startx 134 -starty 220 -endx 184 -endy 240 -objects 3", [
	]], 
	["183.equake", "equake", "train", " < inp.in", [
	]], 
	["187.facerec", "facerec", "train", " < train.in", [
	]], 
	["188.ammp", "ammp", "train", " < ammp.in", [
	]], 
	["189.lucas", "lucas", "train", " < lucas2.in", [
	]], 
	["191.fma3d", "fma3d", "train", "", [
	]], 
	["200.sixtrack", "sixtrack", "train", " < inp.in", [
	]], 
	["301.apsi", "apsi", "train", "", [
	]], 

	["168.wupwise", "wupwise", "ref", "", [
	]], 
	["171.swim", "swim", "ref", " < swim.in", [
	]], 
	["172.mgrid", "mgrid", "ref", " < mgrid.in", [
	]], 
	["173.applu", "applu", "ref", " < applu.in", [
	]], 
	["177.mesa", "mesa", "ref", "-frames 1000 -meshfile mesa.in -ppmfile mesa.ppm", [
	]], 
	["178.galgel", "galgel", "ref", " < galgel.in", [
	]], 
	["179.art", "art", "ref", "-scanfile c756hel.in -trainfile1 a10.img -trainfile2 hc.img -stride 2 -startx 110 -starty 200 -endx 160 -endy 240 -objects 10", [
		["-", ""], 
	]], 
	["179.art", "art", "ref", "-scanfile c756hel.in -trainfile1 a10.img -trainfile2 hc.img -stride 2 -startx 470 -starty 140 -endx 520 -endy 180 -objects 10", [
		["-", ""], 
	]], 
	["183.equake", "equake", "ref", " < inp.in", [
	]], 
	["187.facerec", "facerec", "ref", " < ref.in", [
	]], 
	["188.ammp", "ammp", "ref", " < ammp.in", [
	]], 
	["189.lucas", "lucas", "ref", " < lucas2.in", [
	]], 
	["191.fma3d", "fma3d", "ref", "", [
	]], 
	["200.sixtrack", "sixtrack", "ref", " < inp.in", [
	]], 
	["301.apsi", "apsi", "ref", "", [
	]], 
]
