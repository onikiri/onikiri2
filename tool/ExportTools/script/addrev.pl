use strict;

sub Throw
{
	print shift;
	exit 1;
}

sub Main()
{
	my $svnURL   = $ARGV[0];
	my $fileName = $ARGV[1];
	
	my $svnStatusStr = `LANG=C; svn info $svnURL`;
	
	my $rev = -1;
	foreach my $line ( split(/\n/, $svnStatusStr) ){
		if($line =~ /Revision:[^0-9]*([0-9]+)/){
			$rev = $1;
		}
	}
	
	if($rev == -1){
		Throw( "Could not get the revision of the repository '$svnURL'");
	}
	
	my $dstFileName = $fileName;
	$dstFileName =~ s/([^\/\.\-]+)\-([^\/\.]+)([^\/]*$)/$1\-rev$rev\-$2$3/; 
	`mv $fileName $dstFileName`;
}

Main();
