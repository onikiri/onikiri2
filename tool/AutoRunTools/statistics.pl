#!/usr/bin/perl

use strict;
use FindBin;
use lib "$FindBin::Bin/lib-perl";
use lib "$FindBin::Bin/../../lib/perl/XML-TreePP/lib";

use IO::File;
use Data::Dumper;

use Cfg;


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
# --- Implementation
#

# Serializing all xml data.
# A return value is hash of 'xpath -> data'
sub Serialize($$$)
{
	my $path = shift;
	my $xml  = shift;
	my $data = shift;
	
	
	if(ref($xml) eq 'ARRAY'){
		my $index = 0;
		foreach my $i ( @{$xml} ){
			Serialize( $path."[$index]", $i, $data );
			$index++;
		}
	}
	elsif(ref($xml) eq 'HASH'){
		if($path ne ''){
			$path .= '/';
		}
		my @steps = keys( %{$xml} );
		foreach my $i (@steps){
			Serialize( "$path$i", $xml->{"$i"}, $data );
		}
	}
	else{

		my @cols = split( /,/, $xml );
		if($#cols < 1){
			$data->{ "$path" } = $xml;
		}
		else{
			# Array
			for(my $i = 0; $i <= $#cols; $i++){
				$data->{ "$path".'['.$i.']' } = $cols[ $i ];
			}
		}

	}
}

# Make csv row data (each row data is hash of 'header' -> 'data')
sub MakeCSVRow($$)
{
	my $cfgObj = shift;
	my $xml = shift;
	my $result = $xml->{'Session'}->{'Result'};
	my $data = {};
	
	Serialize('', $result, $data);
	
	my %row;
	my $ptn = $cfgObj->GetConfig()->{'resultXMLNodePattern'};

	foreach my $i ( sort(keys(%{$data})) ){
		
		if( !($i =~ /$ptn/) ){
			next;
		}

		my $value = $data->{$i};
		
		# Escape new lines.
		$value =~ s/\n/\\n/g;
		$value =~ s/\r/\\r/g;
		
		# Double quate.
		$value = "\"$value\"";
		$row{$i} = $value;

	}
	
	return \%row;
}

sub Initialize()
{
	my $cfgFile = $g_defaultCfgFile;
	my $cfgObj;
	my @sessions;
	my $sessionAll = 0;
	my $sessionLatest = 0;
	my $nextIsCfgFileName = 0;
	my $forceProcessing = 0;
	
	foreach my $arg (@ARGV){
		
		if( $nextIsCfgFileName ){
			$nextIsCfgFileName = 0;
			$cfgFile = $arg;
			next;
		}
		
		if($arg =~ /^[0-9]+$/){
			push(@sessions, $arg);
		}
		elsif($arg =~ /^([0-9,-]+)$/){
			my @nums = split(/,/, $arg);
			foreach my $num ( @nums ){
				if($num =~ /^([0-9]+)\-([0-9]+)$/){
					my $front = $1;
					my $last  = $2;
					if($front > $last){
						Throw("Session range '$arg' is invalid.");
					}
					
					foreach my $i ($front..$last){
						push(@sessions, $i);
					}
				}
				else{
					push(@sessions, $num);
				}
			}
		}
		elsif($arg =~ /^all$/){
			$sessionAll = 1;
		}
		elsif($arg =~ /^latest$/){
			$sessionLatest = 1;
		}
		elsif($arg =~ /^-c$/){
			$nextIsCfgFileName = 1; 
		}
		elsif($arg =~ /^-f$/){
			$forceProcessing = 1; 
		}
		else{
			Throw(
				"Usage: perl statistics.pl (n|n-n|n,n,n-n...|all|latest) [options]\n\n".
				"-c CONFIG_XML \n\t Use CONFIG_XML as a configuration XML. \n".
				"-f \n\t Force processing all session results.\n".
				"-h \n\t Print help messages.\n"
			);
		}
	}


	my $cfgObj = new Cfg( $cfgFile );	

	if($sessionLatest){
		# latest
		my @outPaths = $cfgObj->GetSessions();
		push(@sessions, pop(@outPaths));
	}
	elsif($sessionAll || $#sessions < 0){
		@sessions = $cfgObj->GetSessions();
	}

	my $option = 
	{
		'forceProcessing' => $forceProcessing,
		'sessions' => \@sessions,
	};
	
	$cfgObj->SetUserData( $option );
	

	return $cfgObj;
}

sub GetSessions($)
{
	my $cfgObj = shift;
	return @{$cfgObj->GetUserData()->{'sessions'}};
}

sub MakeRowHeader($$)
{
	my $src = shift;
	my $cfg = shift;
	my $rowHeader;
	my $i = 0;
	
	my $pattern = $cfg->{'rowHeaderPattern'};
	
	if( $pattern eq '' ){
		$rowHeader = $src;
	}
	else{
		$src =~ /($cfg->{'rowHeaderPattern'})/;
		$rowHeader = $&;
	}
	return $rowHeader;
}


sub Main()
{
	my $cfgObj = Initialize();
	my $cfg = $cfgObj->GetConfig();
	
	# flush stdout
	$| = 1;

	foreach	my $session ( GetSessions($cfgObj) ){
		my @fileList = $cfgObj->GetResultFiles($session);
		if($#fileList < 0){
			print("Output files of a session '$session' are not found.\n");
			next;
		}
		
		# open output file
		my $outFileName = 
			$cfgObj->GetResultPath( $session ).
			sprintf( "/statistics.%03d.csv", $session );
		
		# Check timestamps of result files 
		if( !$cfgObj->GetUserData()->{ 'forceProcessing' } ){
			my $skip = 1;
			
			if( !(-e $outFileName) ){
				$skip = 0;
			}
			else{
				foreach my $resultFileName ( @fileList ){
					if( !(-e $resultFileName) ){
						$skip = 0;
						last;
					} 
					
					if( -M $resultFileName < -M $outFileName ){
						$skip = 0;
						last;
					}
				}
			}
			
			if( $skip ){
				print( "--- Skip to process a session '$session' to '$outFileName'.\n\n" );
				next;
			}
		}
			
		print( "--- Processing a session '$session' to '$outFileName' ...\n" );
	
		my $head = 1;
		my @colHeaders;
		my $strColHeader;
		my $strData;
		foreach my $fileName ( @fileList ){
			chmod(oct('755'), $fileName);
			my $cfgXML = new XMLHelper( $fileName );
			my $cfgXMLHash = $cfgXML->GetHash();


			# row header
			my $rowHeader = $fileName;
			$rowHeader =~ /([^\/]+$)/;
			$rowHeader = $&;
			$rowHeader = MakeRowHeader( $rowHeader, $cfg);

			if( !$cfgXMLHash ){
				$strData .= $rowHeader.",\n";
				print("\n  '$fileName' is not found and skip this file.\n");
				next;
			}
			
			my $row = MakeCSVRow( $cfgObj, $cfgXMLHash );
			
			# col header
			if($head){
				# make session name
				my $sessionName = 
					sprintf("%03d-%s",
						$session,
						$cfgXMLHash->{'Session'}->{'-Name'} 
					);
				$strColHeader .= "\"$sessionName\",";
				
				# make column header
				foreach my $i ( sort(keys(%{$row})) ){
					my $colHeader = $i;
					push( @colHeaders, $colHeader );
					
					# filter
					$colHeader =~ /($cfg->{'columnHeaderPattern'})/;
					$colHeader = $&;
					
					$strColHeader .= "$colHeader,";
				}
				$strColHeader .= "\n";
				$head = 0;
			}

			# row header
			$strData .= $rowHeader.",";
			
			# data
			foreach my $i ( @colHeaders ){
				$strData .= "$row->{$i},";
			}
			$strData .= "\n";
			
			print(".");
		}
		
		# output string data
		my $file = new IO::File;
		$file->open( $outFileName, "w" );

		if( $head ){
			$file->print( "\n" );	# dummy
		}
		else{
			$file->print( $strColHeader );
		}
		$file->print( $strData );
		$file->close();

		print(" finished.\n\n");
		
	}

}

Main();