package XMLHelper;

use FindBin;
use lib "$FindBin::Bin/";
use lib "$FindBin::Bin/../../lib/perl/XML-TreePP/lib";
use XML::TreePP;

#
# --- Exception & terminating
#

sub Throw
{
	print shift;
	exit 1;
}

#
# --- XML helper
#
sub GetXMLString($)
{
	my $fileName = shift;
	my $file = new IO::File;
	if(!$file->open($fileName)){
		Throw "$fileName is not found\n";
	}
	

	my $xml;
	my $ret;
	while( my $line = $file->getline() ){
		if($line =~ /<\?xml[^\?]+\?>/){
			$xml = 1;
		}
				
		if(!$xml){
			next;
		}
		
		$ret .= $line;
	}
	
	$file->close();
	return $ret;	
}

sub ParseXML($)
{
	my $inputFile = shift;
	my $inputStr  = GetXMLString($inputFile);
	my $xml   = XML::TreePP->new;
	$xml->set( ignore_error => 1 );
	my $hash  = $xml->parse( $inputStr );
	return $hash;
}

#
# --- Object method
#

# XPath access
sub XPath($$)
{
	my $self = shift;
	my $path = shift;
	my $hash = $self->{'hashData'};
	
	my @step = split(/\//, $path);

	foreach my $i (@step){
		if($i eq ''){
			next;
		}
		if(!exists($hash->{$i})){
			Throw "'$path' is not found in configuration file.\n";
		}
		$hash = $hash->{$i};
	}
	return $hash;
}
sub Get($$)
{
	my $self = shift;
	if(exists($self->{shift})){
		return $self->{shift};
	}
	else{
		return undef;
	}
		
}

# hash access
sub GetHash($)
{
	my $self = shift;
	return $self->{'hashData'};
}

# constructor
sub new 
{
	my $packageName = shift;
	my $fileName = shift;
	my $self = {};
	bless $self, $packageName;
	
	$self->{'hashData'} = ParseXML( $fileName );

	return $self;
}


1;