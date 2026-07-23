from scripts.helper import env_helper
from scripts.helper import path_helper
from SCons.Script import *

# 获取根目录
root_dir = path_helper.current_dir()
# .env文件路径
env_file_path = path_helper.path_join(root_dir, ".env")
# 从.env文件中加载环境变量(整个编译进程使用自定义环境变量,不使用系统环境变量)
env_helper.load_env(env_file_path)

# 这里是编译每个进程程序的脚本(相对路径)
script_files = [
    'src/SConscript',
    'app/SConscript',   
]

# 测试脚本只在显式请求 test target 时加载，避免普通构建运行测试。
# scons -c test 同样会包含 test，因此可以清理测试及其构建依赖。
if 'test' in COMMAND_LINE_TARGETS:
    script_files.append('tests/SConscript')

# 按顺序执行编译脚本
SConscript(script_files)
