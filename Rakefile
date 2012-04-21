#!/usr/bin/env rake
#require "bundler/gem_tasks"
#Bundler.setup
load 'sassc.gemspec'
require 'rake/extensiontask'

Rake::ExtensionTask.new do |ext|
  ext.name = 'libsass'                # indicate the name of the extension.
  ext.ext_dir = 'src/'         # search for 'hello_world' inside it.
  ext.lib_dir = 'lib/sassc'              # put binaries into this folder.
  #ext.config_script = 'custom_extconf.rb' # use instead of the default 'extconf.rb'.
  #ext.tmp_dir = 'tmp'                     # temporary folder used during compilation.
  ext.source_pattern = "*.{c,cpp,hpp}"        # monitor file changes to allow simple rebuild.
  #ext.config_options << '--with-foo'      # supply additional options to configure script.
  ext.gem_spec = $gemspec                     # optionally indicate which gem specification
                                          # will be used.
end