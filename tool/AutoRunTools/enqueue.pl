#!/usr/bin/perl

use strict;
use FindBin;
use lib "$FindBin::Bin/lib-perl";
use lib "$FindBin::Bin/../../lib/perl/XML-TreePP/lib";

use IO::File;
use XML::TreePP;
use Data::Dumper;
use Storable qw(dclone);

use Cfg;

require "$FindBin::Bin/execscript.pl";

# Default configuration file
my $g_defaultCfgFile = "$FindBin::Bin/cfg.xml";


#
# --- Exception & terminating
#

sub Throw
{
	print shift;
	exit 1;
}
#
# --- File helper
#
sub GetFileList($)
{
	my $dir = shift;
	opendir(DIR, $dir);
	my @files = readdir(DIR);
	closedir(DIR);
	return sort(@files);
}

sub WriteFile($$)
{
	my $fileName = shift;
	my $str      = shift;

	foreach my $i (0..1000){	# Retry if writing failed.
		my $file = new IO::File;
		if($file->open( $fileName, "w" ) ){
			$file->print( $str );
			$file->close();
			chmod( oct('755'), $fileName );
			
			if( -s $fileName > 0 ){
				last;
			}
		}
		
		print( "Could not write '$fileName'. Retry ... ($i)\n" );
		sleep(5);
	}
}

#
# ---
#

sub Initialize()
{
	my $cfgFile = $g_defaultCfgFile;
	my $cfgObj;
	my @execParam;

	my $option = 
	{
		'test'   => 0, 
		'dump'   => 0, 
		'execParam' => \@execParam,
	};

	foreach my $arg (@ARGV){
		if($arg =~ /^-t$/){	$option->{'test'} = 1;	}
		if($arg =~ /^-d$/){	$option->{'dump'} = 1;	}
		if($arg =~ /^-c([^\s]+)$/){
			$cfgFile = $1; 
		}
		if($arg =~ /^-x([^\s]+)$/){
			push( @execParam, $1 ); 
		}
	}

	my $cfgObj = new Cfg( $cfgFile );	

	
	$cfgObj->SetUserData( $option );
	return $cfgObj;
}

# Get command line options
sub GetOption($)
{
	my $cfgObj = shift;
	return $cfgObj->GetUserData();
		
}

#
# GetOneProcessesCmd generates combinations of file names 
# generated from one <Processes> section.
#
# Ex.:
#
# <Processes>
#   <Process>
#     <Command FileNmaPattern = 'a'>
#     <Command FileNmaPattern = 'b'>
#   </Process>
#   <Process>
#     <Command FileNmaPattern = 'c'>
#     <Command FileNmaPattern = 'd'>
#     <Command FileNmaPattern = 'e'>
#   </Process>
# </Processes>
#
# GetOneProcessesCmd generates the following output from this <Processes> section.
#
# [a,c], [a,d], [a,e], [b,c], [b,d], [b,e]
#
sub GetOneProcessesCmd($$)
{
	my $processesNode = shift;	# One <Processes> node.
	my $basePath    = shift;	# '/Configuration/@BasePath'

	my $processList = $processesNode->{'process'};	
	my $combination = $processesNode->{'combination'};	

	my @allCmdList;	
	
	foreach my $process ( @{$processList} ){

		# This iteration processes one <Process> section.
		my @processCmdList;
		
		foreach my $cmd ( @{$process->{'command'}} ){
			
			# This iteration processes one <Command> section.
			
			my $cmdPath  = $basePath.$cmd->{'-BasePath'};
			my @fileList = GetFileList( $cmdPath );
			
			# Enumerate files that match '-FileNamePattern'.
			my $ptn = $cmd->{'-FileNamePattern'};
			foreach my $file ( @fileList ){
				if( $file =~ /$ptn/ ){
					my $fileName = $cmdPath.$file;
					push( @processCmdList, $fileName );
				}
			}
		}
		push( @allCmdList, \@processCmdList );
	}
	
	my @ret;	
	
	if( $combination eq 'OneToOne' ){
		# Combination is 'OneToOne'.
		
		# Check the total number of command files in each <Process>.
		my $cmdNum = $#{ $allCmdList[0] };
		foreach my $i ( @allCmdList ){
			my $curCmdNum = $#{$i};
			if( $cmdNum != $curCmdNum ){
				Throw( "The total number of command files must be same among <Process>s in 'OneToOne' mode." );
			}
		}

		foreach my $i ( 0..$cmdNum ){
			my @set;
			foreach my $cmd ( @allCmdList ){
				push( @set, $cmd->[ $i ] );
			}
			push( @ret, \@set );
		}
	}
	else{
		# Combination is 'AllToAll'.
	
		# Calculate the total number of combinations of commands.
		my $totalCound = 1;
		my @countList;	
		foreach my $i (@allCmdList){
			my $elementCount = $#{$i} + 1;
			$totalCound *= $elementCount;
			push( @countList, $elementCount );
		}

		# Make all combinations.
		foreach my $i ( 0..$totalCound-1 ){
			my @indexList;
			my $index = $i;
			foreach my $j ( @countList ){
				push( @indexList, $index % $j );
				$index /= $j;
			}
			
			my @set;
			my $setIndex = 0;
			foreach my $cmd ( @allCmdList ){
				push( @set, $cmd->[ $indexList[$setIndex] ] );
				$setIndex++;
			}
			push( @ret, \@set );
		}
	}
	
	return @ret;

}

sub GetCmdFiles($)
{
	my $cfgObj = shift;
	my $cfg = $cfgObj->GetConfig();
	my $basePath = $cfg->{'basePath'};
	
	my @cmd;
	foreach my $i ( @{$cfg->{'processes'}} ){
		push( @cmd, GetOneProcessesCmd( $i, $basePath ) );
	}
	
	return @cmd;
}

# Make each execution script and enqueue command.
sub MakeExecuteScript($$$$)
{
	my $cfgObj = shift;
	my $cmd         = shift;
	my $resultPath  = shift;
	my $session     = shift;
	
	my $cfg = $cfgObj->GetConfig();
	my $bin         = $cfg->{'basePath'}.$cfg->{'binaryFile'};
	my $execStr;
	my $outputFileName;
	my $option = GetOption($cfgObj);

	# $execStr stores the file name of an execution binary and passed option strings.
	$execStr .=	"$bin \\\n";

	# Add a session name to options.
	if( exists( $session->{'-Name'} ) ){
		$execStr .= '  -x @Name='.$session->{'-Name'}." \\\n";
	}

	# Add command XML files.
	foreach my $i ( @{$cmd} ){
		$execStr .= "  $i\\\n";

		$i =~ /([^\/]+)\.xml$/;
		$outputFileName .= $1."-";
	}

	# Make parameter options
	my @parameters = @{ dclone(ToArray( $session->{'Parameter'} ) ) };   # Each parameter may be replaced by macros.
	my $baseOutFileName = $outputFileName;
	$baseOutFileName =~ s/-$//;

    # Reserved Macros
    my $macroHash = 
    {
        'RESULT_DIR' => $resultPath,
        'RESULT_SIGNITURE' => $baseOutFileName,
        'RESULT_BASE_FILE_NAME' => $resultPath."/".$baseOutFileName,
    };
    $cfgObj->ApplyMacroToTree(\@parameters, $macroHash);

    #print Dumper($macroHash);


	foreach my $i ( @parameters ){
		
		# File
		if(exists($i->{'-FileName'})){
			$i->{'-FileName'} =~ /([^\/]+)$/;
			my $fileName = $1;
			$execStr .= "  $resultPath"."param/$fileName \\\n";
			
			$fileName =~ s/\.[^\.]+$//;
			$outputFileName .= "$fileName-";
		}
		
		# Option
		if(exists($i->{'-Option'})){
			$execStr .= "  $i->{'-Option'} \\\n";
		}

		# Node(must have Value)
		# Range is discarded in this phase
		if(exists($i->{'-Node'}) && exists($i->{'-Value'})){
			if($i->{'-Node'} =~ /[=,]/){
				Throw("'=' or ',' are invalid characters in Node attribute.\n".$i->{'-Node'});
			}
			
			$execStr .= "  -x ".$i->{'-Node'}."=".$i->{'-Value'}." \\\n";
		}
	}

	# Make additional option through command line
	foreach my $i (@{$option->{'execParam'}}){
		my $node;
		my $value;
		if($i =~ /([^=]+)=([^=]+)/){
			$node  = $1;
			$value = $2;
		}
		else{
			Throw("Command line option '-x$i' is invalid format.\n");
		}
		
		$execStr .= "  -x ".$node."=".$value." \\\n";
		print $execStr;
	}
	
	$outputFileName =~ s/-$//;

    # Set XML output
    my $resultXML_File = "$resultPath$outputFileName.txt";
    $execStr .= "  -x /Session/Environment/OutputXML/@"."FileName=$resultXML_File \\\n";

	# result
	my $resultFile = "$resultPath$outputFileName.onikiri.stdout";
	$execStr .= "  > $resultFile\\\n";

	# Exec script
	my $execScriptFile = "$resultPath/sh/exec/$outputFileName.sh";
	WriteFile( $execScriptFile, $execStr );
		
	# Enqueue script
	# Make enqueue command
	my $enqueueScript = "$resultPath/sh/enqueue/$outputFileName.sh";
	my $enqueueStr    = MakeExecuteCommand($cfgObj, $execScriptFile, $resultPath);
	WriteFile( $enqueueScript, $enqueueStr );

	my $sessionInfo =
	{
		'-resultFile'      => $resultXML_File,
		'-execScriptFile'  => $execScriptFile,
		'-enqueScriptFile'	=> $enqueueScript
	};


	return $sessionInfo;
}

sub ToArray($)
{
	my $val = shift;
	if(ref($val) ne 'ARRAY'){
		$val = [ $val ];
	}
	return $val;
}

# Copy all parameter-file to session directory
sub SetUpParam($$$)
{
	my $cfgObj = shift;
	my $result  = shift;
	my $session = shift;
	
	my $cfg = $cfgObj->GetConfig();
	my $paramDesPath = $result."param";

	if(!exists($session->{'Parameter'})){
		return;
	}

	my @parameters = @{ ToArray($session->{'Parameter'}) };
	my @paramFiles;
	
	foreach my $i ( @parameters ){
		if(exists($i->{'-FileName'})){
			push(@paramFiles, $i->{'-FileName'});
		}
	}
	
	foreach my $i (@paramFiles){
		my $src = $cfg->{'basePath'}.$i;
		if(!(-e $src)){
			Throw("'$src' is not found.\nParameter setup failed.");
		}
		`cp $src $paramDesPath`;
	}
}

# Parse and expand range attribute.
sub ExpandRangeParameter($)
{
	my $arg = shift;
	my @ret;
	
	if($arg =~ /^((\[[^\]]+\])|,)+$/){
		# format [n,n],[n,n]
		@ret = split(/\]\s*,\s*\[/, $arg);
		foreach my $i (@ret){
			$i =~ s/[\[\]]//g;
		}
	}
	elsif($arg =~ /^([0-9,-]+)$/){
		# format : n-n,n 
		my @nums = split(/,/, $arg);
		foreach my $num ( @nums ){
			if($num =~ /^([0-9]+)\-([0-9]+)$/){
				my $front = $1;
				my $last  = $2;
				if($front > $last){
					Throw("Range '$arg' is invalid.");
				}
				
				foreach my $i ($front..$last){
					push(@ret, $i);
				}
			}
			else{
				push(@ret, $num);
			}
		}
	}
	else{
		# format : others
		@ret = split(/,/, $arg);
	}
	return @ret;
}

sub GetDigestParamName($)
{
	my $base = shift;
	$base =~ /([^\/]+)$/;
	my $digest = $1;

	$digest =~ s/[\@aiueo]//g;
	if(length($digest) < 3){
		return substr($base,0,3);
	}
	elsif($digest =~ /[A-Z]+[a-z]+([A-Z])/){
		return substr($digest,0,3).$1;
	}
	else{
		return substr($digest,0,3);
	}
}

sub GetRangeParamtersFromSession($)
{
	my $src = shift;
	my @ret;
	foreach my $i ( @{$src} ){
		if(exists($i->{'-Range'})){
			if(!exists($i->{'-Node'})){
				Throw("A 'Parameter' node which has 'Range' attribute must have both 'Node' and 'Range' attributes.");
			}
			push(@ret, $i);
		}
	}
	return @ret;
}

sub ExpandSessions($$)
{
	my $parameter = shift;
	my $srcSessions = shift;

	my $paramName = $parameter->{'-Node'};
	my @valList   = ExpandRangeParameter($parameter->{'-Range'});

	my @ret;
	
	foreach my $session ( @{$srcSessions} ){
		foreach my $val (@valList){
			# Copy session data
			my $tmpSession = dclone($session);
			
			# Add node
			$tmpSession->{'Parameter'} = ToArray( $tmpSession->{'Parameter'} );
			push( 
				@{ $tmpSession->{'Parameter'} },
				{
					'-Node'  => $paramName,
					'-Value' => $val
				}
			);
			
			# Name expand
			$tmpSession->{'-Name'} .= "-".GetDigestParamName($paramName).$val;
			
			# Add session data
			push(@ret, $tmpSession);
		}
	}
	
	return @ret;
}

sub GetSessions($)
{
	my $cfgObj = shift();
	my @srcSessionList = @{ $cfgObj->GetConfig()->{'session'} };
	my @dstSessionList;

	foreach my $src (@srcSessionList){
		
		if( exists($src->{'-Enable'}) && $src->{'-Enable'} == 0 ){
			next;
		}
		
		#
		# Pickup session which has range attribute.
		#
		my @rangeParameter = 
			GetRangeParamtersFromSession( ToArray($src->{'Parameter'}) );
		
		if($#rangeParameter < 0){
			push(@dstSessionList, $src);
			next;
		}
		
		
		#
		# Expand session
		#
		my @expanded;
		push(@expanded, $src);

		foreach my $parameter ( @rangeParameter ){
			@expanded = ExpandSessions( $parameter, \@expanded );	
		}
		
		push(@dstSessionList, @expanded);
	}

	if(GetOption($cfgObj)->{'dump'} == 1){
		print Dumper(\@dstSessionList);
	}

	return @dstSessionList;
}


sub Main()
{
	my $cfgObj = Initialize();
	my $cfg = $cfgObj->GetConfig();
	my $option = GetOption($cfgObj);
	my @cmd = GetCmdFiles($cfgObj);
	my @sessions = GetSessions( $cfgObj );
	my $xmlTreePP = XML::TreePP->new;
	my @sessionQueue;

	print "Total session count : ".(($#sessions+1)*($#cmd+1))."\n";

	print "Setting up queuing environment ...\n";
	{
		my $resultPath = $cfg->{'basePath'}.$cfg->{'resultBasePath'};
		if(!(-e $resultPath)){
			print( "The result directory '$resultPath' does not exist and now making it ... ");
			if( !mkdir( $resultPath ) ){
				Throw( "Failed\nCould not make the directory '$resultPath'." );
			}
			else{
				print( "Success\n" );
			}
		}		
		
		my $index = 0;
		foreach my $session	( @sessions ){

			my $resultPath = $cfgObj->MakeNewResultPath();
			SetUpParam($cfgObj, $resultPath, $session);

			printf( "Setting up a session (%d/%d) '%s' ...", $index+1, $#sessions+1, $session->{'-Name'} );
			
			my $sessionInfo = 
			{
				'Session' => 
				{
					'Job' => [],
					'-Name' => $session->{'-Name'},
					'-ResultPath' => $resultPath, 
				}
			};
			
			foreach my $i (@cmd){
				my $job = MakeExecuteScript($cfgObj, $i, $resultPath, $session);
				push( @{$sessionInfo->{'Session'}->{'Job'}}, $job );
			}
		
			push( @sessionQueue, $sessionInfo );

			my $sessionInfoFile = "$resultPath/session.xml";
			my $sessionInfoStr = $xmlTreePP->write( $sessionInfo );		
			WriteFile( $sessionInfoFile, $sessionInfoStr );

			print " finished.\n";
			$index++;
			sleep(2); # for reducing load
		}
		print "\n\n";
	}
	
	sleep(3);
	print "--- Queuing sessions ...\n";
	{
		my $index = 0;
		foreach my $session	( @sessionQueue ){

			printf(
				"Queueing a session (%d/%d) '%s' ...\n", 
				$index+1,
				$#sessionQueue+1,
				$session->{'Session'}->{'-Name'}
			);
			
			foreach my $job ( @{$session->{'Session'}->{'Job'}} ){
				my $exec = $job->{'-enqueScriptFile'};
				if($option->{'test'} == 0){
					`$exec`;
					print ".";
					sleep(1); # for reducing load
				}
				else{
					print "$exec\n";
				}
			}
			print " finished.\n";
			$index++;
		}
	}
}

Main();
