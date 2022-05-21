#!/usr/bin/env ruby
$-w=true

require 'ownet'
c = OWNet::Connection.new
p c.dir('/')
p c.read('/10.B7B4C4000800/temperature')
