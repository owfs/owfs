#!/usr/bin/env ruby
$-w = true

require 'ownet'

def usage
    puts 'Usage: temperatures.rb [server [port]]'
end
  
if ARGV.size > 2
  usage
  exit(1)
end

opts = {}
opts[:server] = ARGV[0] if ARGV.size > 0
begin
  opts[:port] = Integer(ARGV[1]) if ARGV.size > 1
rescue ArgumentError
  puts "Port must be an integer"
  usage
  exit(1)
end

root = OWNet::Sensor.new('/', opts)
puts "root: #{root}"

puts "root.entries"
root.each_entry {|e| puts " - #{e}"}

puts "root.sensors"
root.each_sensor {|s| puts " - #{s}"}

root.each_sensor do |s|
  if s.has_attr?('temperature')
    puts "temperature: #{s.temperature}"
    puts "templow: #{s.templow}"
    puts "Setting templow to 25"
    s.templow = 25
    puts "templow: #{s.templow}"
  end
end
        
