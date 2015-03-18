#
# Visual Studio 2005/2008 project file converter
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

#
# Returns node array reference regardless of whether argument is array or scalar.
#
sub NodeToList($)
{
	my $node = shift;
	my $ret = [];
	
	if(ref($node) eq 'ARRAY'){
		foreach my $i ( @{$node} ){
			push(@{$ret}, $i);
		}
	}
	else{
		push(@{$ret}, $node);
	}
	
	return $ret;
}

#
#
#
sub ListToNamedNode($$)
{
	my $list = shift;
	my $name = shift;
	
	if(ref($list) ne 'ARRAY'){
		return undef;
	}
	
	my $ret = undef;
	foreach my $i ( @{$list} ){
		if($i->{'-Name'} eq $name){
			$ret = $i;
		}
	}
	return $ret;
}


#
# Enumerate files from vcproj recursively.
# Returns a hash of configurations keyed by file name.
#
sub EnumerateFiles
{
	my $node = shift;
	my $list = {};
	
	if(ref($node) ne 'HASH'){
		return {};
	}
	
	if(exists( $node->{'File'} )){
		my $files = NodeToList( $node->{'File'} );
		
		foreach my $i ( @{$files} ){
			my $file = $i->{'-RelativePath'};
			$file =~ s/\\/\//g;
			
			my $elem = { $file => $i };
			%{$list} = ( %{$list}, %{$elem} );
		}
	}
	
	if(exists( $node->{'Filter'} )){
		my $filters = NodeToList( $node->{'Filter'} );
		foreach my $i ( @{$filters} ){
			%{$list} = ( %{$list}, %{ EnumerateFiles($i) } );
		}
	}

	return $list;
}

#
# Filer mathced to cpp file.
#
sub FilterCpp($)
{
	my $src = shift;
	my $des = [];
	
	foreach my $file ( @{$src} ){
		if( !($file =~ /(\.cpp$)|(\.cc$)|(\.c$)/) ){
			next;
		}
		push( @{$des}, $file );
	}
	return $des;
}

sub Dependence($)
{
	my $src = shift;
	my $des = [];
	foreach my $file ( @{$src} ){
		my $dep = `g++ -MM $file`;
		push( @{$des}, $dep );
	}
	return $des;
}

sub GetDefaultCompilerConfiguration($$)
{
	my $vcproj = shift;
	my $cnvCfg = shift;
	
	my $cfgList = NodeToList( 
		$vcproj->{'VisualStudioProject'}->{'Configurations'}->{'Configuration'} );

	my $srcCfgName = $cnvCfg->{'MakeConfiguration'}->{'-SourceConfiguration'};
	my $targetCfg  = ListToNamedNode($cfgList, $srcCfgName);
	
	if($targetCfg == undef){
		print("Source configuration name '$srcCfgName' is not found.\n");
	}
	
	my $tools = NodeToList( $targetCfg->{'Tool'} );
	my $tool  = ListToNamedNode( $tools, "VCCLCompilerTool" ); 

	if($tool == undef){
		print("Extracting default compiler configuration failed.\n");
	}
	return $tool;
}

sub GetCompilerConfiguration($$$$)
{
	my $src = shift;
	my $srcCfg = shift;
	my $defCfg = shift;
	my $cnvCfg = shift;

	my $cfgList = NodeToList( 
		$srcCfg->{$src}->{'FileConfiguration'} );

	my $srcCfgName = $cnvCfg->{'MakeConfiguration'}->{'-SourceConfiguration'};
	my $targetCfg  = ListToNamedNode($cfgList, $srcCfgName);

	my %ret = %{$defCfg};
	my $tools = NodeToList( $targetCfg->{'Tool'} );
	my $tool  = ListToNamedNode( $tools, "VCCLCompilerTool" ); 
	if($tool != undef){
		%ret = (%ret, %{$tool});
	}
	
	return \%ret;
}

sub OutputMake($$$$$)
{
	my $src      = shift;
	my $srcCfg   = shift;
	my $vcproj   = shift;
	my $cnvCfg      = shift;
	my $cfgFileName = shift;
	
	my $des;
	$des .= "#\n# This file is generated from ".
			$cnvCfg->{'MakeConfiguration'}->{'-SourceFile'}."\n#\n\n";
	
	# default pre-compiled header
	my $defaultCompilerCfg = GetDefaultCompilerConfiguration($vcproj, $cnvCfg);

	# working directory
	my $workPath = '$(WORK_DIR)';

	# detect host 
	$des .= 'HOST_TYPE = $(shell uname)'."\n\n";

	foreach my $platform ( @{$cnvCfg->{'MakeConfiguration'}->{'Platforms'}->{'Platform'}} ){

		$des .= sprintf("# Platform: %s\n", $platform->{'-Name'});
		$des .= sprintf(
			'ifneq (,$(findstring %s,$(HOST_TYPE)))'."\n",
			 $platform->{'-HostString'} );

		# Make PCH strings
		#my $pch = $platform->{'-PrecompiledHeader'};
		#if($path ne ''){
		#}
		#$des .= sprintf("WORK_DIR = %s\n", $path);

		# Make WORK_DIR strings
		my $path = $platform->{'-WorkingDirectory'};
		if($path ne ''){
			$path = $path."/";
		}
		$des .= sprintf("WORK_DIR = %s\n", $path);

		# Make CXX flag strings
		$des .= sprintf("CXX = %s\n", $platform->{'-CXX'});
		$des .= sprintf("CXXFLAGS = %s", $platform->{'-CXXFlags'});
		if(ref($platform->{'IncludeDirectories'}) eq 'HASH'){
			foreach my $i ( @{ NodeToList($platform->{'IncludeDirectories'}->{'Directory'}) }){
				if($i ne ''){
					$des .= " \\\n\t-I".$i->{'-Path'};
				}
			}
		}
		$des .= "\n";
		
		# Make LDFLAGS flag strings
		$des .= sprintf("LDFLAGS = %s", $platform->{'-LDFlags'});
		if(ref($platform->{'LibraryDirectories'}) eq 'HASH'){
			foreach my $i ( @{ NodeToList($platform->{'LibraryDirectories'}->{'Directory'}) }){
				if($i ne ''){
					$des .= " \\\n\t-L".$i->{'-Path'};
				}
			}
		}
		if(ref($platform->{'Libraries'}) eq 'HASH'){
			foreach my $i ( @{ NodeToList($platform->{'Libraries'}->{'Library'}) }){
				if($i ne ''){
					$des .= " \\\n\t-l".$i->{'-Name'};
				}
			}
		}
		if( exists($platform->{'-OutputFile'}) ){
			$des .= " \\\n\t-o ".$platform->{'-OutputFile'};
		}
		else{
			$des .= " \\\n\t-o a.out";
		}
		$des .= "\n";

		# clean
		$des .= "CLEARFILTER = ";
		foreach my $i ( split( /\s/, $platform->{'-CleanTargets'} ) ){
			$des .= $workPath.$i." ";
		}
		$des .= $platform->{'-OutputFile'};
		$des .= "\n";

		$des .= "endif\n\n";
		
	}
	
	my $usePCH = $cnvCfg->{'MakeConfiguration'}->{'-UsePrecompiledHeader'};

	# Make object file and pch name list
	my $srcObjPair = [];
	my $srcPCHPair = [];
	my $objID = 0;
	my $pchID = 0;
	my $pchMap = {};
	foreach my $i (@{$src}){
		my $obj;
		
		my $cfg    = GetCompilerConfiguration($i, $srcCfg, $defaultCompilerCfg, $cnvCfg);
		my $pchFlag = $cfg->{'-UsePrecompiledHeader'};
		my $pchSrc = $cfg->{'-PrecompiledHeaderThrough'};
		my $pch;
		
		$pchSrc =~ /([^\/]+)\.([^\/]+)$/;
		if(exists($pchMap->{$pchSrc})){
			$pch = $pchMap->{$pchSrc};
		}
		else{
			$pchSrc =~ /([^\/]+)\.([^\.]+)$/;
			$pch = "$workPath$1.$pchID.h";
			$pchMap->{$pchSrc} = $pch;
			$pchID++;
		}
		my $gch = "$pch.gch";
				
		
		if($pchFlag == 1 && $usePCH){
			# Create PCH
			$i =~ /^(.+\/)([^\/]+)$/;
			$pchSrc = $1.$pchSrc;

			push( @{$srcPCHPair}, 
				{
					'src' => $pchSrc,
					'gch' => $gch,
					'pch' => $pch,
				} );
			
			#print Dumper ($srcPCHPair);
		}
		else{
			$i =~ /([^\/]+)\.([^\.]+)$/;
			$obj = "$1.$objID.o";
			
			my $elem = 
			{
				'src' => $i,
				'obj' => $workPath.$obj,
			};
			
			if($pchFlag == 2 && $usePCH){
				#use PCH
				$elem->{'pch'} = $pch;
				$elem->{'gch'} = $gch;
			}

			push( @{$srcObjPair}, $elem );
			$objID++;
		}
	}
	
	# object list
	$des .= "\n\n";
	$des .= "OBJS = \\\n";
	foreach my $i ( @{$srcObjPair} ){
		$des .= "\t".$i->{'obj'}." \\\n";
	}
	$des .= "\n\n";

	# pch list
	$des .= "\n\n";
	$des .= "PCHS = \\\n";
	foreach my $i ( @{$srcPCHPair} ){
		$des .= "\t".$i->{'gch'}." \\\n";
	}
	$des .= "\n\n";

	# link
	$des .= 'all: $(WORK_DIR) $(OBJS)';
	$des .= "\n\t".'$(CXX) $(OBJS) $(LDFLAGS)';
	$des .= "\n\n";

	# makefile rebuilding
	$cfgFileName =~ /([^\/]+)$/;
	my $relCfgFileName = $1;
	
	my $scriptPath = $0;
	my $cfgPath = $cfgFileName;
	if( !($scriptPath =~ /^\//) ){
		while($cfgPath =~ s/(^[^\/]+\/)//){
			$scriptPath = "../".$scriptPath;
		}
	}
	
	$des .= "Makefile : ".$cnvCfg->{'MakeConfiguration'}->{'-SourceFile'}." $relCfgFileName\n";
	$des .= "\trm -rf ".'$(CLEARFILTER)'."\n";
	$des .= "\tperl $scriptPath $relCfgFileName\n\n";

	# work 
	$des .= '$(WORK_DIR)'.": \n";
	$des .= "\tmkdir $workPath \n\n";
	
	# clean
	$des .= "clean: \n";
	$des .= "\trm -rf ".'$(CLEARFILTER)'."\n\n";
	
	# makefile
	$des .= '$(OBJS): '."Makefile\n\n";

	# dependency
	$des .= 'ifneq ($(MAKECMDGOALS),clean)'."\n";
	$des .= '-include $(OBJS:.o=.d)'."\n";
	$des .= '-include $(PCHS:.gch=.d)'."\n";
	$des .= 'endif'."\n\n";

	# -- compile

	#pch
	$des .= "\n\n";
	foreach my $i ( @{$srcPCHPair} ){
		$des .= "$i->{'gch'}: $i->{'src'}\n";
		$des .= "\t".'$(CXX) $(CXXFLAGS)'." -c $i->{'src'} -o $i->{'gch'} -MMD";
		$des .= "\n\n";
	}

	#obj
	$des .= "\n\n";
	foreach my $i ( @{$srcObjPair} ){
		my $gch;
		my $pch;
		if( exists($i->{'pch'}) ){
			$gch = $i->{'gch'};
			$pch = '-include '.$i->{'pch'};
		}

		$des .= "$i->{'obj'}: $i->{'src'} $gch\n";
		$des .= "\t".'$(CXX) $(CXXFLAGS)'." -c $i->{'src'} -o $i->{'obj'} $pch -MMD";
		$des .= "\n\n";
	}
	
	return $des;
}

sub main
{
	print "Visual Studio 2005/2008 project file converter.\n";
	print "2010/05/01 Ryota Shioya\n\n";
	
	my $cfgFileName = $ARGV[0];
	my $file = new IO::File;
	my $cfg = ParseXML( $cfgFileName );

	$cfgFileName =~ /^(.+\/)([^\/]+)$/;
	my $relCfgPath = $1;

	my $root = ParseXML( $relCfgPath . $cfg->{'MakeConfiguration'}->{'-SourceFile'} );
	my $fileParam = EnumerateFiles( $root->{'VisualStudioProject'}->{'Files'} );
	my $fileList = [ keys(%{$fileParam}) ];
	my $fileCppList = FilterCpp( $fileList );


	my $outMake = OutputMake( $fileCppList, $fileParam, $root, $cfg, $cfgFileName );
	$file->open($relCfgPath . "Makefile","w");
	$file->print($outMake);
	$file->close();


}


&main;

