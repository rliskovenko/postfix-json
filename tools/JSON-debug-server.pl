#!/usr/bin/perl

use warnings;
use strict;

use Mojolicious::Lite;
use Mojo::Log;
use JSON;

app->hook( 'before_dispatch' => sub {
    my $c = shift;
    $c->app->log->info("url:".$c->req->url." body: ".$c->req->body);
    $c->render( json => { code => 200, url => $c->req->url, method => $c->req->method, body => $c->req->body } )
});

get '/' => sub {
    my $self = shift;
    $self->render( text => 'Oops!' );
};

app->start;
