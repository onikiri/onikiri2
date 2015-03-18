use strict;
use IO::File;
use FindBin;

my $defaultFileNameIn  = "$FindBin::Bin/../../../src/DefaultParam.h";
my $defaultFileNameOut = "$FindBin::Bin/DefaultParam.xml";

sub Process( $$ )
{
	my $fileNameOut = shift;
	my $fileNameIn  = shift;
	
	print "$fileNameIn > \n$fileNameOut\n";
	
	if( (-C $fileNameIn) > (-C $fileNameOut) ){
#		print( -C $fileNameIn );
#		print( -C $fileNameOut );
		print "The XML file is newer than the C header file.\nAre you sure? [y,N] ";
		my $yseNo = <STDIN>;
		if( !($yseNo =~ /^y[\n\r]/) ){
			exit 1;
		}
	}

	my $fileIn  = new IO::File;
	my $fileOut = new IO::File;

	$fileIn->open( $fileNameIn );
	$fileOut->open( $fileNameOut, "w" );

	while( my $line = $fileIn->getline() ){

		if( $line =~ s/^[\t\s]*static[\t\s]+char[\t\s]+g_defaultParam\[\][\t\s]+=[\t\s]+"[\t\s]*\\n\\[\n\r]+$//g) {
		}
		elsif( $line =~ s/^[\t\s]*";[\t\s]*[\n\r]+$//g ){
			$fileOut->print("$line");
			last;
		}
		else{
			$line =~ s/[\t\s]*\\n\\[\n\r]+$//;
			$fileOut->print("$line\r\n");
		}
	}

	$fileOut->close();
	$fileIn->close();
}

if( $#ARGV >= 0 ){
	foreach my $i (@ARGV){
		my $in  = $i;
		my $out = $in;
		$out	=~ s/\.[^\.]+$/\.xml/;
		Process( $out, $in );
	}
}
else{
	Process( $defaultFileNameOut, $defaultFileNameIn );
}
