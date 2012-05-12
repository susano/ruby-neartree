
Gem::Specification.new do |s|
	s.name        = 'neartree'
	s.date        = '2010-12-19'
	s.version     = '0.2'
	s.summary     = 'Ruby NearTree bindings.'
	s.description = 'neartree is a Ruby library binding to the NearTree C library for storing point/value
pairs in an R-Tree structure and searching it for the nearest neighbour for any given point.'
	s.authors     = ['Jean Krohn']
	s.email       = 'jb.krohn@gmail.com'
	s.homepage    = 'http://github.com/susano/ruby-neartree'
	s.extensions  = 'ext/neartree/extconf.rb'
  s.rubyforge_project = 'neartree'
	s.files = %w[
README.rdoc
neartree.gemspec
ext/neartree/extconf.rb
ext/neartree/NearTree.c
]
end

