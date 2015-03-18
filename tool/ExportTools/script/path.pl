#
# Usage : perl licence.pl 'source root path' 'destination root path'
#

use strict;

use File::Path;
use IO::File;
use Data::Dumper;

use Cwd qw( realpath );
use File::Spec;


my $g_execptDirectoryPathPattern = '\.svn$';
my $g_filePattern                = '(\.c)|(\.cpp)|(\.cc)|(\.h)$';

my $g_sourceDirectory            = $ARGV[0];
my $g_destinationDirectory       = $ARGV[1];

if( $#ARGV == -1 ){
	print "Usage:\n perl path.pl 'source root path' 'output path'";
	exit 0;
}

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



sub ProcessFile($$$)
{
	my $dstFile = shift;
	my $srcFile = shift;
	my $processHandler = shift;
		
	my $file = new IO::File;
	if(!$file->open( $srcFile )){
		Throw( "Could not open the source file '$srcFile'.\n" );
	}
	my @sourceText = $file->getlines();
	$file->close();
	
	if(!$file->open( $dstFile, "w" )){
		Throw( "Could not open the destination file '$dstFile'.\n" );
	}
	
	my $dstData = 
		$processHandler->( 
			\@sourceText,
			$dstFile,
			$srcFile
		);
	
	$file->print( @{$dstData} );

	$file->close();
}

sub ProcessSourceFiles($)
{
	my $processHandler = shift; 
	my $srcPath = $g_sourceDirectory;
	my $dstPath = $g_destinationDirectory;
	
	if(!(-d $dstPath)){
		print "Make destination direcotory '$dstPath'.\n";
		mkpath($dstPath);
	}

	my @dstFiles = GetFileAndDirectoryList( $dstPath );
	if($#dstFiles != -1){
	#	Throw(
	#		"Destination directory '$dstPath' is not empty.\n".
	#		"Remove all files in destination directory and try again.\n"
	#	);
	}
	
	foreach my $i ( GetDestinationDirectoryTree( $srcPath ) ){
		mkpath($i);
	}

	foreach my $srcFile ( GetSourceFileTree( $srcPath ) ){
		my $dstFile = $srcFile;
		$dstFile =~ s/^$srcPath/$dstPath/;

		ProcessFile(
			$dstFile,
			$srcFile,
			$processHandler
		);
	}
}

sub Main($)
{
	my $processHandler = shift; 
	ProcessSourceFiles( $processHandler );
}


#
# User handler
#
my $g_processHandler = sub 
{
	my $lines   = shift;
	my $dstFile = shift;
	my $srcFile = shift;
	
	my $srcPath = $srcFile;
	$srcPath =~s/(^.+\/)[^\/]+$/$1/;
	$srcPath = realpath( $srcPath );
	
	my @dst;
	
	foreach my $line ( @{$lines} ){
		
		if( $line =~ /^#include[\s]+\"([^\"]+)\"/ ){
			my $includedFile = $1;
			
			#print "test '$includedFile' in '$srcFile' ... ";

			if( -e "$g_sourceDirectory/$includedFile" ){

			}
			else{
			
				my $includedCanoPath = realpath( "$srcPath/$includedFile" );
				my $correctPath = 
					File::Spec->abs2rel(
						 $includedCanoPath, 
						 realpath( $g_sourceDirectory ) 
					);
				#print "$correctPath\n";
				
				$line = "#include \"$correctPath\"\r\n";
			}
		}
		

		$line =~ s/\r\n$/\n/;
		$line =~ s/\n$/\r\n/;
		#$line =~ s/([^\n\r])\n$/$1\n/;

		push( @dst, $line );
	}
	
	return \@dst;
};

#
# Entry point
#
Main( $g_processHandler );

#print Dumper( GetDestinationFileTree('../../src') );

