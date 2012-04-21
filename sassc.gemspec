# -*- encoding: utf-8 -*-
require File.expand_path('../lib/sassc', __FILE__)

Gem::Specification.new do |gem|
  gem.authors       = ["Hampton Catlin", "Aaron Leung"]
  gem.email         = ["hcatlin@gmail.com"]
  gem.description   = %q{A native implementation of the Sass language}
  gem.summary       = %q{Native Sass}
  gem.homepage      = "http://github.com/hcatlin/libsass"

  gem.files         = `git ls-files`.split($\)
  gem.executables   = []#gem.files.grep(%r{^bin/}).map{ |f| File.basename(f) }
  gem.test_files    = gem.files.grep(%r{^(test|spec|features)/})
  gem.name          = "sassc"
  gem.extensions    = ["src/extconf.rb"]
  gem.require_paths = ["lib", "src"]
  gem.version       = SassC::VERSION
end
