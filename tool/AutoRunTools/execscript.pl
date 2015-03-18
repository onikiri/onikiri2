use strict;
use Data::Dumper;

sub MakeExecuteCommand($$$)
{
	my $cfgObj = shift;
	my $scriptFile = shift;
	my $resultPath = shift;
	
	# '$scriptFile' and '$resultPath' are given by full path
	
	my $cfg = $cfgObj->GetConfig();
	
	# for torque on MTL amateras
	my $queue = $cfg->{'macro'}->{'USER'};
	my $exec = "qsub -d $resultPath/work/ -o $resultPath/work/ -e $resultPath/work/ -q $queue -l nodes=1 -l ncpus=1 $scriptFile";
	
	
	# Sample: for direct execution
	#my $exec = "sh $scriptFile";

	# Sample: Dump all $cfg hash data.
	# print Dumepr( $cfg ); exit 0;
	
	return $exec;
}

1;
