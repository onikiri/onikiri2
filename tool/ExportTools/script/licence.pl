#
# Usage : perl licence.pl 'source root path' 'destination root path'
#

use strict;

use File::Path;
use IO::File;
use Data::Dumper;

my $g_execptDirectoryPathPattern = '\.svn$';
my $g_filePattern                = '(\.c)|(\.cpp)|(\.cc)|(\.h)|(\.xml)$';

my $g_sourceDirectory            = $ARGV[0];
my $g_destinationDirectory       = $ARGV[1];

my $g_licenceText                = 'licence.txt';

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

sub GetFileAndDirectoryList($)
{
	my $dir = shift;
	
	my @paths;
	opendir(DIR, $dir);
	@paths = readdir(DIR);
	closedir(DIR);

	@paths = grep(
		($_ ne '.') && ($_ ne '..'),
		@paths 
	);

	@paths = map( 
		($dir.'/'.$_),
		@paths 
	);
	
	return @paths;
}

sub GetFileAndDirectoryTree($)
{
	my $root = shift;
	if((-d $root) != 1){
		Throw("The specified root path '$root' is not directory.");
	}
	
	sub SearchFunc
	{
		my $root    = shift;
		my @paths  = GetFileAndDirectoryList( $root );
		
		@paths =
			grep( 
				!(-d $_) || ((-d $_) && !($_ =~ /$g_execptDirectoryPathPattern/)),
				@paths 
			);
		
		my @dirs = grep( (-d $_), @paths );
		
		foreach my $i (@dirs){
			push( @paths, SearchFunc($i) );
		};
		
		return @paths;
	};
	
	return SearchFunc( $root );
}


sub GetSourceFileTree($)
{
	my $root = shift;
	return 
		grep(
			!(-d $_) && ($_ =~ /$g_filePattern/), 
			GetFileAndDirectoryTree( $root )
		);
}

sub GetSourceDirectoryTree($)
{
	my $root = shift;
	return 
		grep(
			(-d $_), 
			GetFileAndDirectoryTree( $root )
		);
}

sub GetDestinationDirectoryTree($)
{
	my $root = shift;
	my @dirs = GetSourceDirectoryTree( $root );
	foreach my $i (@dirs){
		$i =~ s/^$root/$g_destinationDirectory\//;
	}
	return @dirs;
}

#
# ---
#

sub GetLicenceText()
{
	my $file = new IO::File;
	if(!$file->open( $g_licenceText )){
		Throw("The licence text '$g_licenceText' is not found.");
	}
	my @licenceText = $file->getlines();
	$file->close();
	return \@licenceText;
}

sub AddLicence($$$)
{
	my $dstFile = shift;
	my $srcFile = shift;
	my $licenceText = shift;
	
	my $file = new IO::File;
	if(!$file->open( $srcFile )){
		Throw( "Could not open the source file '$srcFile'.\n" );
	}
	my @sourceText = $file->getlines();
	$file->close();
	
	if(!$file->open( $dstFile, "w" )){
		Throw( "Could not open the destination file '$dstFile'.\n" );
	}
	
	print( "$srcFile -> $dstFile\n" );
	
	if( $srcFile =~ /\.xml$/ ){
		# If a source file is XML, the current implementation does not output
		# a license text because the XML specification does not allow to put comment 
		# sections before a XML declaration section.
		#foreach my $line ( @{$licenceText} ){
		#	$file->print( $line );
		#}
	}
	else{
		foreach my $line ( @{$licenceText} ){
			$file->print( "// $line" );
		}
		$file->print("\r\n\r\n");
	}


	foreach my $line ( @sourceText ){
		$file->print("$line");
	}

	$file->close();
}

sub ProcessSourceFiles()
{
	my $srcPath = $g_sourceDirectory;
	my $dstPath = $g_destinationDirectory;
	
	if(!(-d $dstPath)){
		print "Make destination direcotory '$dstPath'.\n";
		mkpath($dstPath);
	}

	my @dstFiles = GetFileAndDirectoryList( $dstPath );
	if($#dstFiles != -1){
		Throw(
			"Destination directory '$dstPath' is not empty.\n".
			"Remove all files in destination directory and try again.\n"
		);
	}
	
	foreach my $i ( GetDestinationDirectoryTree($srcPath) ){
		mkpath($i);
	}

	my $licenceText = GetLicenceText();
	foreach my $srcFile ( GetSourceFileTree( $srcPath ) ){
		my $dstFile = $srcFile;
		$dstFile =~ s/^$srcPath/$dstPath/;

		AddLicence(
			$dstFile,
			$srcFile,
			$licenceText
		);
	}
}

sub Main()
{
	ProcessSourceFiles();
}

Main();

#print Dumper( GetDestinationFileTree('../../src') );