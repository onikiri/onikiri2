# usage:
#  ruby makexml.rb benchcmd.rb [architecture] [OS]
#  カレントディレクトリに，benchcmd.rbに定義された$benchCmdsに従いxmlを生成

load "#{ARGV[0]}"

arch   = ARGV[1]
osArch = ARGV[2]

binDir = "./bin/"
runBase = "../run/"

multiCmdCount = Hash.new

$benchCmds.each do |benchCmd|
	benchName = benchCmd[0]
	benchBin = benchCmd[0]
	inputSize = benchCmd[2]

	runDir = "#{runBase}#{benchName}/#{inputSize}/"
	args = benchCmd[3].split(" < ")[0]
	inFile = benchCmd[3].split(" < ")[1]

	baseName = "#{benchName}-#{inputSize}"
	if (multiCmdCount[baseName] == nil)
		multiCmdCount[baseName] = 0
	else
		multiCmdCount[baseName] += 1
	end
	xmlName = "#{baseName}.#{multiCmdCount[baseName]}.xml"

	file = open(xmlName, "w")
	file.print <<"END"
<?xml version='1.0' encoding='utf-8'?>
<Session>
  <!-- Emulator parameters-->
  <Emulator
    TargetArchitecture='#{osArch}'
  >
    <Processes>
      <Process
        TargetBasePath='../'
        TargetWorkPath='#{runDir}'
        Command='#{binDir}#{benchBin}.#{arch}'
        CommandArguments='#{args}'
        STDIN='#{inFile}'
      />
    </Processes>
  </Emulator>
</Session>
END
	file.close
end
