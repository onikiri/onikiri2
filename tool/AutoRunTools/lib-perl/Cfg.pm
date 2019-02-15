package Cfg;

use FindBin;
use lib "$FindBin::Bin/";
use lib "$FindBin::Bin/../../lib/perl/XML-TreePP/lib";
use XMLHelper;
use Data::Dumper;

sub Throw
{
	print shift;
	exit 1;
}

sub ToArray($)
{
	my $val = shift;
	if(ref($val) ne 'ARRAY'){
		$val = [ $val ];
	}
	return $val;
}

sub GetUserName()
{
	my $user = `whoami`;
	chomp($user);
	return $user;
}

#
# Replace all strings in a passed XML tree with $macroHash.
# This replacement is performed recursively.
#
sub ApplyMacroInternal($$$$$)
{
	my $root      = shift;
	my $node      = shift;
	my $pattern   = shift;
	my $str       = shift;
	my $macroHash = shift;
	

	if (ref($node) eq 'HASH'){
		foreach my $i (keys(%{$node})){
			$node->{$i} = ApplyMacroInternal($root, $node->{$i}, $pattern, $str, $macroHash);
		}
	}
	elsif (ref($node) eq 'ARRAY') {
		foreach my $i (@{$node}) {
			$i = ApplyMacroInternal($root, $i, $pattern, $str, $macroHash);
		}
	}
	else{   # SCALAR
		#if ($node =~ /\$\($pattern\)/) {
		#	print "$node\n\t$pattern\t->\t$str\n";
        #}
        
		if ($node =~ s/\$\($pattern\)/$str/g) {
            # Re-apply all macros to the replaced string.
            # This process should not be performed the whole tree.
			foreach my $pattern (keys(%{$macroHash})) {
				my $str = $macroHash->{$pattern};
				$node = ApplyMacroInternal($root, $node, $pattern, $str, $macroHash);
			}
		}
	}	

	return $node;
}
	

sub InitializeConfig($)
{
	my $cfgFile = shift;
	my $cfgXML = new XMLHelper( $cfgFile );
	my $cfg = {};

	# statistics
	$cfg->{'resultFilePattern'}    = $cfgXML->XPath( '/Configuration/Result/-FileNamePattern' );
	$cfg->{'resultXMLNodePattern'} = $cfgXML->XPath( '/Configuration/Result/-NodeNamePattern' );
	$cfg->{'rowHeaderPattern'}     = $cfgXML->XPath( '/Configuration/Result/-RowHeaderPattern' );
	$cfg->{'columnHeaderPattern'}  = $cfgXML->XPath( '/Configuration/Result/-ColumnHeaderPattern' );
	$cfg->{'outputAverage'}        = $cfgXML->XPath( '/Configuration/Result/-OutputAverage' );
	
	# enqueue
	$cfg->{'binaryFile'}     = $cfgXML->XPath('/Configuration/Input/-BinaryFile');
	$cfg->{'session'}        = ToArray( $cfgXML->XPath('/Configuration/Sessions/Session') );

	my $xmlProcesses = ToArray( $cfgXML->XPath('/Configuration/Input/Processes') );

	foreach my $i ( @{$xmlProcesses} ){
		
		my $xmlProcess = ToArray( $i->{'Process'} );
		my @process;
		

		foreach my $j ( @{$xmlProcess} ){
			my $cmd = {};
			$cmd->{'command'} = ToArray( $j->{'Command'} );
			push( @process, $cmd );
		}
		
		my $combination = 'AllToAll';
		if( exists( $i->{'-Combination'} ) ){
			$combination = $i->{'-Combination'};
			if( $combination ne 'AllToAll' && $combination ne 'OneToOne' ){
				Throw( "'$combination' is not supported in 'Processes/@Combination'." );
			}
		}
		
		push( 
			@{$cfg->{'processes'}}, 
			{
				'process' => \@process,
				'combination' => $combination
			}
		);
	}


	# both
	$cfg->{'basePath'}             = $cfgXML->XPath('/Configuration/-BasePath');
	$cfg->{'resultBasePath'}       = $cfgXML->XPath('/Configuration/Result/-BasePath');

	
	# Expand macro
	my $macroDefs = ToArray( $cfgXML->XPath( '/Configuration/Macros/Macro' ) );
	my $macroHash = {};
	$macroHash->{'USER'} = GetUserName();
	foreach my $i ( @{$macroDefs} ){
		$macroHash->{ $i->{'-Name'} } = $i->{'-Value'};
	}
	$cfg->{'macro'} = $macroHash;	# Final $cfg result has replaced macro values.

	foreach my $pattern ( keys(%{$macroHash}) ){
		my $str = $macroHash->{$pattern};
		ApplyMacroInternal( $cfg, $cfg, $pattern, $str, $macroHash );
	}


	# Validate & post process
	$cfg->{'basePath'} .= '/';
	if(!($cfg->{'basePath'} =~ /^\//)){
		Throw "/Configuration/\@BasePath must be absolute path.";
	}
	
	$cfg->{'resultBasePath'} .= '/';
	if($cfg->{'resultBasePath'} =~ /^\//){
		Throw "/Configuration/Result/\@BasePath must be relative path.\n";
	}
	
	if($cfg->{'rowHeaderPattern'} eq ''){
		$cfg->{'rowHeaderPattern'} = '.*';
	}
	
	if($cfg->{'columnHeaderPattern'} eq ''){
		$cfg->{'columnHeaderPattern'} = '.*';
	}
	
	return $cfg;
}

#
# --- File helper
#

sub GetFileList($)
{
	my $dir = shift;
	opendir(DIR, $dir);
	my @files = readdir(DIR);
	closedir(DIR);
	return sort(@files);
}

#
# --- Object method
#

sub GetResultFiles($$)
{
	my $self  = shift;
	my $session = shift;
	my $cfg = $self->GetConfig();
	my $resultPath = $self->GetResultPath( $session );
	my @files      = GetFileList($resultPath);
	my @ret;

	foreach my $i (@files){
		my $ptn = $cfg->{'resultFilePattern'};
		if($i =~ /$ptn/){
			push(@ret, $resultPath.$i);
		}
	}
	return @ret;
}

sub GetResultBasePath($)
{
	my $self = shift;
	my $cfg = $self->GetConfig();
	return $cfg->{'basePath'}.$cfg->{'resultBasePath'};
}

sub GetResultPath($$)
{
	my $self = shift;
	my $session = shift;
	return $self->GetResultBasePath().sprintf("%03d/", $session);
}

# Make session result directory and return its path.
sub MakeNewResultPath($)
{
	my $self = shift;
	my $cfg = $self->GetConfig();
	my $result = $cfg->{'basePath'}.$cfg->{'resultBasePath'};
	my $i = 0;
	
	if(!(-e $result)){
		mkdir( $result );
	}
	
	my $make = sub($)
	{
		return sprintf("%s%03d/", $result, shift); 
	};
	
#	while( -e $make->($i) ){
#		$i++;
#	}
#	
#	my $path = $make->($i);


	my @dirList = GetFileList($result);
	my @resultList;
	my $lastSession = 0;
	foreach my $i ( @dirList ){
		if( $i =~ /([0-9]+)/ ){
			if( $lastSession < $1  ){
				$lastSession = $1;
			}
		}
	}
	
	my $path = $make->( $lastSession + 1 );


	mkdir( $path );
	mkdir( $path.'work' );
	mkdir( $path.'sh' );
	mkdir( $path.'sh/exec' );
	mkdir( $path.'sh/enqueue' );
	mkdir( $path.'param' );
	return $path;
}

sub GetSessions()
{
	my $self = shift;
	my $cfg = $self->GetConfig();
	my $resultPath = $cfg->{'basePath'}.$cfg->{'resultBasePath'};
	my @ret;
	foreach my $i ( sort(GetFileList($resultPath)) ){
		if($i =~ /^[0-9]{3}$/){
			push(@ret, $i);
		}
	}
	if($#ret < 0){
		Throw "Any session output is not found in '$resultPath'.\n";
	}
	return @ret;
}

# Constructor
sub new 
{
	my $packageName = shift;
	my $cfgFileName = shift;
	my $self = {};
	bless $self, $packageName;
	
	$self->{'configData'} = InitializeConfig( $cfgFileName );
	$self->{'userData'} = {};

	return $self;
}

# accessor
sub GetConfig($)
{
	my $self = shift;
	return $self->{'configData'};
}

# User data
sub SetUserData($$)
{
	my $self = shift;
	my $userData = shift;
	$self->{'userData'} = $userData;
}

sub GetUserData($)
{
	my $self = shift;
	return $self->{'userData'};
}


1;