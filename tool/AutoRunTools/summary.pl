#!/usr/bin/perl

use strict;
use FindBin;
use lib "$FindBin::Bin/lib-perl";
use lib "$FindBin::Bin/../../lib/perl/XML-TreePP/lib";
use lib "$FindBin::Bin/../../lib/perl/Text-CSV/lib";

use IO::File;
use Data::Dumper;
use Text::CSV_PP;

use Cfg;
use XMLHelper;
use CSVMatrix;

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
# CSV parsing
#
sub QuatedCSVSplit($)
{
	my $line = shift;
	#return split(/,/, $line);
	
	my $csv = new Text::CSV_PP( { binary => 1 } );
	$csv->parse( $line );
	return $csv->fields();
}

#
# --- Option
#

sub Initialize()
{
	my @sessions;
	my $pattern = '';
	my $header  = undef;
	my $cfgFile = $g_defaultCfgFile;
	my $sessionAll = 0;
	my $force = 0;
	
	foreach my $arg (@ARGV){
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
		elsif($arg =~ /^-f$/){
			$force = 1;
		}
		elsif($arg =~ /^-e([^\s]+)$/){
			$pattern = $1; 
		}
		elsif($arg =~ /^-h([^\s]+)$/){
			$header = $1; 
		}
		elsif($arg =~ /^-c([^\s]+)$/){
			$cfgFile = $1; 
		}
		else{
			Throw("Usage: perl summary.pl (n|n-n|n,n,n...|all) -e(pattern) -c(config file) [-f]");
		}
	}
	
	my $cfgObj = new Cfg( $cfgFile );	
	
	if($sessionAll || $#sessions < 0){
		@sessions = $cfgObj->GetSessions();
	}
	if($pattern eq ''){
		$pattern = '.+';
	}
	
	my $option = 
	{
		'sessions' => \@sessions,
		'pattern'  => $pattern, 
		'header'   => $header, 
		'cfgFile'  => $cfgFile,
		'force'    => $force,
	};
	
	$cfgObj->SetUserData( $option );
	

	return $cfgObj;
}


#
# --- Implement
#
sub GetSessionNumbers($)
{
	my $cfgObj = shift;
	return @{$cfgObj->GetUserData()->{'sessions'}};
}

sub GetHeader($)
{
	my $cfgObj = shift;
	my $option = $cfgObj->GetUserData();

	my %header;
	if($option->{header}){
		my $file = new IO::File;
		if(!$file->open($option->{header})){
			Throw("Could not open header file '".$option->{header}."'.");
		}

		while(my $line = $file->getline()){
			$line =~ s/[\n\r]$//g;
			my @toks = QuatedCSVSplit($line);
			if($#toks == 1 && $toks[0] =~ /[0-9]+/){
				$header{ sprintf("%03d",$toks[0]) } = $toks[1];
			}
		}
		$file->close();		
	}
	return %header;
}

sub GetSessionResultFileName($$)
{
	my $cfgObj = shift;
	my $sessionNumber = shift;
	my $fileName = 
		$cfgObj->GetResultPath($sessionNumber).sprintf("/statistics.%03d.csv",$sessionNumber);
		
	return $fileName;
}

sub GetCfgFileName($)
{
	my $cfgObj = shift;
	return $cfgObj->GetUserData()->{'cfgFile'};
}

sub CheckAndProcessStatisticsFile($$$)
{
	my $cfgObj  = shift;
	my $cfgFile = shift;
	my $session = shift;

	my $pipe = new IO::File;
	my $cmd = "perl \"$FindBin::Bin/statistics.pl\" -c $cfgFile $session ";
	
	my $force = $cfgObj->GetUserData()->{'force'};

	if( $force ){
		$cmd .= "-f ";
	}
	
	$pipe->open( $cmd."|" );
	
	while( !$pipe->eof() ){
		my $msg = $pipe->getline();
		if( $msg =~ /Skip/ ){
			next;
		}
		#	print $msg;
		if( $msg =~ /Processing/ ){
			print "\n$msg";
			my $char;
			while( $char ne "\n" ){
				$char = $pipe->getc();
				print $char;
			}
			while( $msg = $pipe->getline() ){
				print $msg;
			}
		}
	}
	$pipe->close();

}

sub GetDataMatrices($)
{
	my $cfgObj = shift;
	my %header = GetHeader($cfgObj);
	
	
	print "Processing ... \n";
	my @dat;
	my $cfgFile = GetCfgFileName($cfgObj);
	foreach	my $session ( GetSessionNumbers($cfgObj) ){
		print "$session,";
		
		CheckAndProcessStatisticsFile( $cfgObj, $cfgFile, $session );

		my $sessionFileName = GetSessionResultFileName( $cfgObj, $session );
		if( !( -e $sessionFileName ) ){
			print "\nCould not make the result of '$session' and skip it.\n";
			next;
		}
		
		my $csvMatix = new CSVMatrix( $sessionFileName );
		
		my $sessionName = $csvMatix->GetTopLeftHeader();
		# Session name convert
		if( exists( $header{$session} ) ){
			$sessionName = $header{$session};
		}
		
		push(
			@dat,
			{
				'matrix'      => $csvMatix,
				'sessionName' => $sessionName
			}
		);
	}

#	print Dumper( \@dat );
	return \@dat;
}

sub Main()
{
	# flush stdout
	$| = 1;


	my $cfgObj = Initialize();
	my $dataMatrices = GetDataMatrices( $cfgObj );

	my $pattern = $cfgObj->GetUserData()->{'pattern'};
	
	if( $#{$dataMatrices} < 0){
		Throw( "There is no data.\n" );
	}
	
	my %rowHeader;
	my %colHeader;
	foreach my $matrix ( @{$dataMatrices} ){
		foreach my $i ( $matrix->{'matrix'}->GetRowHeader() ){
			$rowHeader{ $i } = 1;
		}
		foreach my $i ( $matrix->{'matrix'}->GetColHeader() ){
			if($i =~ /$pattern/){
				$colHeader{ $i } = 1;
			}
		}
	}
	
	my $resultBasePath = $cfgObj->GetResultBasePath()."summary/";
	mkdir($resultBasePath);
	
	print "\nWriting ... \n";
	foreach my $col ( sort(keys( %colHeader )) ){
		my $suffix = $col;
		$suffix =~ s/[\/\\]/-/g;
		my $fileName = $resultBasePath."$suffix.csv";
		my @matrix;
		
		# header
		push( @matrix, [ $col ] );
		foreach my $data ( @{$dataMatrices} ){
			push( @{$matrix[0]}, $data->{'sessionName'} );
		}
		
		# Merge session data
		foreach my $row ( sort( keys( %rowHeader ) ) ){
			my @rowList;
			push( @rowList, $row );
			
			foreach my $data ( @{$dataMatrices} ){
				push( @rowList, $data->{'matrix'}->GetDataFromName( $row, $col ) );
			}
			
			push( @matrix, \@rowList );
		}
		
		# Calculate average
		if( $cfgObj->GetConfig()->{'outputAverage'} ){
			my $height = $#matrix;			# -1 is size of header
			my $width  = $#{$matrix[0]};
			
			$matrix[ $height + 1 ]->[0] = 'average';
			
			for( my $i = 1; $i <= $width; $i++ ){
				my $sum = 0.0;
				for( my $j = 1; $j <= $height; $j++ ){
					$sum += $matrix[$j]->[$i];
				}
				$matrix[ $height + 1 ]->[$i] = $sum / $height;
			} 
		}
		
		#print Dumper(\@matrix);
		
		my $csv = new CSVMatrix();
		$csv->LoadRawMatrix( \@matrix );
		$csv->DumpMatrix( $fileName );
		
		print ".";
	}

	print " finished.\n";
}

Main();
