use strict;
use IO::File;
use FindBin;

my $tab = 4;
my $formattedLineLength = 80;

my $defaultFileNameIn  = "$FindBin::Bin/DefaultParam.xml";
my $defaultFileNameOut = "$FindBin::Bin/../../src/src/DefaultParam.h";

sub Process($$)
{
	my $fileNameOut = shift;
	my $fileNameIn  = shift;

	print "$fileNameIn > \n$fileNameOut\n";

	if( (-C $fileNameIn) > (-C $fileNameOut) ){
		print "The C header file is newer than the XML file.\nAre you sure? [y,N] ";
		my $yseNo = <STDIN>;
		if( !($yseNo =~ /^y[\n\r]/) ){
			exit 1;
		}
	}

	my $fileIn  = new IO::File;
	my $fileOut = new IO::File;

	$fileIn->open( $fileNameIn );
	$fileOut->open( $fileNameOut, "w" );

	$fileOut->print( "static char g_defaultParam[] = \"\\n\\\r\n" );

	while( my $line = $fileIn->getline() ){

		my $spaced = $line;
		$spaced =~ s/\t/  /g;
		$line =~ s/[\n\r]//g;
		my $length = $formattedLineLength - length( $spaced ) + 1;
		
		my $formatted = $line;
		$formatted =~ s/\t/  /g;
		$fileOut->print( $formatted );
		
		my $padding = $tab - $length % $tab;
		if($padding != 0){
			$fileOut->print( "\t" );
		}
		if($length / $tab > 0){
			foreach(0..($length / $tab)){
				$fileOut->print( "\t" );
			}
		}

		$fileOut->print( "\\n\\\r\n" );
		
	}

	$fileOut->print( "\";\r\n\r\n" );

	$fileOut->close();
	$fileIn->close();
}


if( $#ARGV >= 0 ){
	foreach my $i (@ARGV){
		my $in  = $i;
		my $out = $in;
		$out	=~ s/\.[^\.]+$/\.h/;
		Process( $out, $in );
	}
}
else{
	Process( $defaultFileNameOut, $defaultFileNameIn );
}
