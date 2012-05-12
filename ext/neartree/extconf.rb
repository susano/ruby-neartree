require 'mkmf'

extension_name = 'neartree'

abort 'need CNearTree.h' unless have_header('CNearTree.h')

$CFLAGS = '-std=c99 -g -Wall ' + $CFLAGS

dir_config(extension_name + '/src')
have_library('CNearTree')
create_makefile(extension_name)

