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
		elsif($arg =~ /^-c([^\s]+)$/){
			$cfgFile = $1; 
		}
		else{
			Throw("Usage: perl diagnosis.pl (n|n-n|n,n,n...|all) -c(config file)");
		}
	}
	
	my $cfgObj = new Cfg( $cfgFile );	
	
	if($sessionAll || $#sessions < 0){
		@sessions = $cfgObj->GetSessions();
	}
	
	my $option = 
	{
		'sessions' => \@sessions,
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

sub GetSessionXMLFileName($$)
{
	my $cfgObj = shift;
	my $sessionNumber = shift;
	my $fileName = 
		$cfgObj->GetResultPath($sessionNumber)."session.xml";
		
	return $fileName;
}

sub LoadSessionXML($)
{
	my $file = shift;
	my $xmlTreePP = XML::TreePP->new;
	$xmlTreePP->set( force_array => [ "Job" ] );

	if( !(-e $file) ){
		Throw( "Sesion XML file '$file' is not found. Could not recover this session." );
	}

	return $xmlTreePP->parsefile( $file );
}

sub Main()
{
	# flush stdout
	$| = 1;

	my $cfgObj = Initialize();
	my @sessionNubmers = GetSessionNumbers( $cfgObj );
	
	
	foreach my $sessionNumber (@sessionNubmers){
		my $sessionXMLFile = GetSessionXMLFileName( $cfgObj, $sessionNumber );
		my $sessionXML = LoadSessionXML( $sessionXMLFile );
		printf( "Checking a session $sessionNumber '%s' ...\n", $sessionXML->{'Session'}->{'-Name'} );

		foreach my $job ( @{$sessionXML->{'Session'}->{'Job'}} ){
			my $resultFile       = $job->{'-resultFile'};
			my $enqueueScritFile = $job->{'-enqueScriptFile'};
			
			my $recovery = 0;
			if(!(-e $resultFile)){
				$recovery = 1;
			}
			
			if( $recovery == 0 ){
				my $resultXMLObj = new XMLHelper( $resultFile );
				my $resultXML = $resultXMLObj->GetHash();
				if( !exists( $resultXML->{'Session'}->{'Result'} ) ){
					$recovery = 1;
				}
			}

			if( $recovery ){
				print "\nA broken job is found.\nRe-enqueue $enqueueScritFile\n";
				`$enqueueScritFile`;
				sleep(1);
			}
			print '.';
		}
		
		print " finished.\n\n";
	}



}

Main();
