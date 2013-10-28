#!/usr/bin/perl

use warnings;
use strict;

use Sendmail::Milter;

my %callbacks = (
    'connect' => \&cb_connect,
    'helo'  => \&cb_helo,
    'envfrom' => \&cb_envfrom,
    'envrcpt'   => \&cb_envrcpt,
    'header'    => \&cb_header,
    'eoh'   => \&cb_eoh,
    'body'  => \&cb_body,
    'eom'   => \&cb_eom,
    'abort' => \&cb_abort,
    'close' => \&cb_close,
);

sub cb_eom {
    my $ctx = shift;

    print "Discarded\n";
    return SMFIS_DISCARD;
}

sub cb_connect {
    my $ctx = shift;

    print "Connected\n";
    return SMFIS_CONTINUE;
}

sub cb_helo {
    my $ctx = shift;

    print "Helo\n";
    return SMFIS_CONTINUE;
}

sub cb_envfrom {
    my $ctx = shift;

    print "envfrom\n";
    return SMFIS_CONTINUE;
}

sub cb_envrcpt {
    my $ctx = shift;

    print "envrcpt\n";
    return SMFIS_CONTINUE;
}

sub cb_header {
    my $ctx = shift;

    print "Header\n";
    return SMFIS_CONTINUE;
}

sub cb_eoh {
    my $ctx = shift;

    print "eoh\n";
    return SMFIS_CONTINUE;
}

sub cb_body {
    my $ctx = shift;

    print "body\n";
    return SMFIS_CONTINUE;
}

sub cb_abort {
    my $ctx = shift;

    print "abort\n";
    return SMFIS_CONTINUE;
}

sub cb_close {
    my $ctx = shift;

    print "close\n";
    return SMFIS_CONTINUE;
}

Sendmail::Milter::setdbg(3);
Sendmail::Milter::setconn("local:/tmp/milter.sock");
Sendmail::Milter::register("test_filter", \%callbacks, SMFI_CURR_ACTS );
Sendmail::Milter::main();
