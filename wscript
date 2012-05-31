import Options
from os import unlink, symlink, popen
from os.path import exists

srcdir = "."
blddir = "build"
VERSION = "0.0.1"

def set_options(opt):
    opt.tool_options("compiler_cxx")

def configure(conf):
    conf.check_tool("compiler_cxx")
    conf.check_tool("node_addon")
    conf.env.libsass_compiled = exists("./libsass/libsass.a")

def build(bld):
    if bld.env.libsass_compiled == False:
      bld.exec_command("cd ../libsass;make")
  
    obj = bld.new_task_gen("cxx", "shlib", "node_addon")
    obj.uselib = "sass"
    obj.target = "binding"
    obj.source = "binding.cpp "
    obj.use = ['LIB_SASS']
    obj.add_obj_file("./libsass/libsass.a")
