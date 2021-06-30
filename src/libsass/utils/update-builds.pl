#!/usr/bin/perl

use strict;
use warnings;

# Installed via `cpan install File::Slurp`
# Alternative `cpanm install File::Slurp`
use File::Slurp qw(read_file write_file);

my $tmpl_msvc_head = <<EOTMPL;
    <ClInclude Include="\$(LIBSASS_HEADERS_DIR)\\%s" />
EOTMPL

my $tmpl_msvc_inc = <<EOTMPL;
    <ClInclude Include="\$(LIBSASS_INCLUDES_DIR)\\%s" />
EOTMPL

my $tmpl_msvc_src = <<EOTMPL;
    <ClCompile Include="\$(LIBSASS_SRC_DIR)\\%s" />
EOTMPL

my $tmpl_msvc_filter_head = <<EOTMPL;
    <ClInclude Include="\$(LIBSASS_INCLUDES_DIR)\\%s">
      <Filter>Library Includes</Filter>
    </ClInclude>
EOTMPL

my $tmpl_msvc_filter_inc = <<EOTMPL;
    <ClInclude Include="\$(LIBSASS_INCLUDES_DIR)\\%s">
      <Filter>LibSass Headers</Filter>
    </ClInclude>
EOTMPL

my $tmpl_msvc_filter_src = <<EOTMPL;
    <ClCompile Include="\$(LIBSASS_SRC_DIR)\\%s">
      <Filter>LibSass Sources</Filter>
    </ClCompile>
EOTMPL

# parse source files directly from libsass makefile
open(my $fh, "<", "../Makefile.conf") or
    die "../Makefile.conf not found";
my $srcfiles = join "", <$fh>; close $fh;

my (@INCFILES, @HPPFILES, @SOURCES, @CSOURCES);
# parse variable out (this is hopefully tolerant enough)
if ($srcfiles =~ /^\s*INCFILES\s*=\s*((?:.*(?:\\\r?\n))*.*)/m) {
	@INCFILES = grep { $_ } split /(?:\s|\\\r?\n)+/, $1;
} else { die "Did not find c++ INCFILES in libsass/Makefile.conf"; }
if ($srcfiles =~ /^\s*HPPFILES\s*=\s*((?:.*(?:\\\r?\n))*.*)/m) {
	@HPPFILES = grep { $_ } split /(?:\s|\\\r?\n)+/, $1;
} else { die "Did not find c++ HPPFILES in libsass/Makefile.conf"; }
if ($srcfiles =~ /^\s*SOURCES\s*=\s*((?:.*(?:\\\r?\n))*.*)/m) {
	@SOURCES = grep { $_ } split /(?:\s|\\\r?\n)+/, $1;
} else { die "Did not find c++ SOURCES in libsass/Makefile.conf"; }
if ($srcfiles =~ /^\s*CSOURCES\s*=\s*((?:.*(?:\\\r?\n))*.*)/m) {
	@CSOURCES = grep { $_ } split /(?:\s|\\\r?\n)+/, $1;
} else { die "Did not find c++ CSOURCES in libsass/Makefile.conf"; }

sub renderTemplate($@) {
    my $str = "\n";
    my $tmpl = shift;
    foreach my $inc (@_) {
        $str .= sprintf($tmpl, $inc);
    }
    $str .= "  ";
    return $str;
}

@INCFILES = map { s/\//\\/gr } @INCFILES;
@HPPFILES = map { s/\//\\/gr } @HPPFILES;
@SOURCES = map { s/\//\\/gr } @SOURCES;
@CSOURCES = map { s/\//\\/gr } @CSOURCES;

my $targets = read_file("build-skeletons/libsass.targets");
$targets =~s /\{\{includes\}\}/renderTemplate($tmpl_msvc_inc, @INCFILES)/eg;
$targets =~s /\{\{headers\}\}/renderTemplate($tmpl_msvc_head, @HPPFILES)/eg;
$targets =~s /\{\{sources\}\}/renderTemplate($tmpl_msvc_src, @SOURCES, @CSOURCES)/eg;
warn "Generating ../win/libsass.targets\n";
write_file("../win/libsass.targets", $targets);

my $filters = read_file("build-skeletons/libsass.vcxproj.filters");
$filters =~s /\{\{includes\}\}/renderTemplate($tmpl_msvc_filter_inc, @INCFILES)/eg;
$filters =~s /\{\{headers\}\}/renderTemplate($tmpl_msvc_filter_head, @HPPFILES)/eg;
$filters =~s /\{\{sources\}\}/renderTemplate($tmpl_msvc_filter_src, @SOURCES, @CSOURCES)/eg;
warn "Generating ../win/libsass.vcxproj.filters\n";
write_file("../win/libsass.vcxproj.filters", $filters);

