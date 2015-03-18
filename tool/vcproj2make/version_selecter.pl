#
# Visual Studio 2005/2008/2010 project file version selecter
#

use strict;

use FindBin;
use lib "$FindBin::Bin/../../lib/perl/XML-TreePP/lib";

use IO::File;
use XML::TreePP;
use Data::Dumper;

sub ParseXML($)
{
	my $input = shift;
	
	if(!(-e $input)){
		print "error: '$input' is not found.\n";
		return undef;
	}
	
	my $xml  = XML::TreePP->new();
	my $hash = $xml->parsefile( $input );
	return $hash;
}

sub main
{
	my $cfgFileName = $ARGV[0];
	my $file = new IO::File;
	my $cfg = ParseXML( $cfgFileName );

	$cfgFileName =~ /^(.+\/)([^\/]+)$/;
	my $relCfgPath = $1;

	my $root = ParseXML( $relCfgPath . $cfg->{'MakeConfiguration'}->{'-SourceFile'} );
	
	if( exists( $root->{'VisualStudioProject'} ) ){
		# VS 2005/2008
		my $cmd = "perl $FindBin::Bin/vcproj2make.pl $cfgFileName\n";
		print $cmd;
		print `$cmd`;
	}
	elsif( exists( $root->{'Project'} ) ){
		# VS 2010
		my $cmd = "perl $FindBin::Bin/vcxproj2make.pl $cfgFileName\n";
		print $cmd;
		print `$cmd`;
	}
	else{
		print "Unknown file format."
	}
}


&main;
