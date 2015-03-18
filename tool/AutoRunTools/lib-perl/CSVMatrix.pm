package CSVMatrix;

use strict;
use IO::File;
use Data::Dumper;
use Text::CSV_PP;

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
sub SplitQuatedCSVLine($$)
{
	my $self = shift;
	my $line = shift;
	#return split(/,/, $line);
	my $csv = $self->{'csv'};
	$csv->parse( $line );
	return $csv->fields();
}

#
# Load and parse CSV file 
#
sub LoadCSVFile($$)
{
	my $self     = shift;
	my $fileName = shift;

	my $file = new IO::File;

	if( !$file->open($fileName) ){
		Throw("Could not open '$fileName'\n");
	}

	# Parse csv
	my @rawMatrix;
	while(my $line = $file->getline()){
		$line =~ s/[\n\r]$//g;
		my @col = $self->SplitQuatedCSVLine( $line );
		push( @rawMatrix, [@col] );
	}
	
	$self->LoadRawMatrix( \@rawMatrix );
	
}

sub LoadRawMatrix($$)
{
	my $self      = shift;
	my @rawMatrix = @{shift()};

	# Split header and matrix
	my @matrix = @rawMatrix;	
	my $colHeader     = shift( @matrix );
	my $topLeftHeader = shift( @{$colHeader} );

	my @rowHeader;
	foreach my $row (@matrix){
		push(@rowHeader, shift(@{$row}));
	}

	# Construct header map
	$self->ConstructIndexMap( \@rowHeader, $colHeader );

	$self->{'topLeftHeader'} = $topLeftHeader;
	$self->{'colHeader'} = $colHeader;
	$self->{'rowHeader'} = \@rowHeader;
	$self->{'matrix'}    = \@matrix;
	$self->{'rawMatrix'} = \@rawMatrix;
}

sub ConstructIndexMap()
{
	my $self          = shift;
	my $rowHeader     = shift;
	my $colHeader     = shift;

	# Construct header map
	my %colIndexMap;
	my %rowIndexMap;
	foreach my $i ( 0..$#{$colHeader} ){
		$colIndexMap{ $colHeader->[$i] } = $i;
	}
	foreach my $i ( 0..$#{$rowHeader} ){
		$rowIndexMap{ $rowHeader->[$i] } = $i;
	}


	$self->{'colIndexMap'} = \%colIndexMap;
	$self->{'rowIndexMap'} = \%rowIndexMap;
}

sub DumpMatrix($$)
{
	my $self = shift;
	my $fileName = shift;

	my $file = new IO::File;
	if(!$file->open( $fileName, "w") ){
		Throw("Could not open '$fileName'");
	}
	
	# header
	$file->print( '"'. $self->GetTopLeftHeader() .'",' );
	foreach my $col ( $self->GetColHeader() ){
		$file->print( '"'. $col . '",');
	}
	$file->print("\n");

	my $rowIndex = 0;	
	foreach my $row ( $self->GetRowHeader() ){
		$file->print( '"'. $row . '",' );
		
		foreach my $col ( @{ $self->{'matrix'}->[$rowIndex] } ){
			$file->print( '"'. $col . '",');
		}
		
		$file->print("\n");
		$rowIndex++;
	}
	
	$file->close();
}

#
# Accessor
#

sub GetColHeader($$)
{
	my $self = shift;
	return @{ $self->{'colHeader'} };
}

sub GetRowHeader($$)
{
	my $self = shift;
	return @{ $self->{'rowHeader'} };
}
	
sub GetTopLeftHeader($$)
{
	my $self = shift;
	return $self->{'topLeftHeader'};
}

sub GetMatrix($$)
{
	my $self = shift;
	return $self->{'matrix'};
}

sub GetRawMatrix($$)
{
	my $self = shift;
	return $self->{'rawMatrix'};
}

sub GetDataFromName($$$)
{
	my $self    = shift;
	my $rowName = shift;
	my $colName = shift;
	
	if( !exists( $self->{'rowIndexMap'}->{$rowName} )  ||
		!exists( $self->{'colIndexMap'}->{$colName} ) 
	){
		return undef;
	}
		
		
	my $rowIndex = $self->{'rowIndexMap'}->{$rowName};
	my $colIndex = $self->{'colIndexMap'}->{$colName};
	return $self->GetMatrix()->[$rowIndex]->[$colIndex];
}

#
# Constructor
#
sub new 
{
	my $packageName = shift;
	my $fileName    = shift;

	my $self = 
	{
		'fileName' => '$fileName',
		'csv' => new Text::CSV_PP( { binary => 1 } ),
		
		'colHeader' => [],
		'rowHeader' => [],
		'matrix'    => []
	};
	
	bless $self, $packageName;

	
	if( $fileName ne ''){
		$self->LoadCSVFile( $fileName );
	}
	
	return $self;
}


1;

