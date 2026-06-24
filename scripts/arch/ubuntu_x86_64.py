from scripts.helper import env_helper
from scripts.helper import path_helper
import json

'''
    该板子的环境配置，最后会写入根目录下的.env文件
    参数：
        root_dir (str): 根文件路径。
    返回：
        无。
    异常：
        无。
'''
def arch_env_init(root_dir):
    env_file_path = path_helper.path_join(root_dir, ".env")
    # 编译器地址
    COMPILER_PATH = path_helper.get_abs_path("/usr/bin")
    # C 编译器路径
    CC = path_helper.path_join(COMPILER_PATH, "gcc")
    # C++ 编译器路径
    CXX = path_helper.path_join(COMPILER_PATH, "g++")
    # 汇编器路径
    AS = path_helper.path_join(COMPILER_PATH, "as")
    # 静态库归档工具
    AR = path_helper.path_join(COMPILER_PATH, "ar")
    # 链接器路径
    LD = path_helper.path_join(COMPILER_PATH, "ld")
    # 通用编译选项
    COMMON_FLAGS = ['-g', '-O0', '-Wall', '-Wextra', '-rdynamic']
    # C 编译选项
    CFLAGS = COMMON_FLAGS + ['-std=c11']
    # C++ 编译选项
    CXXFLAGS = COMMON_FLAGS + ['-std=c++11']
    # 集体写入到环境变量里面去
    env_helper.set_env('CC', CC, env_file_path)
    env_helper.set_env('CXX', CXX, env_file_path)
    env_helper.set_env('AS', AS, env_file_path)
    env_helper.set_env('AR', AR, env_file_path)
    env_helper.set_env('LD', LD, env_file_path)
    # 环境变量只能存字符串，先将列表转成json字符串
    env_helper.set_env('CFLAGS', json.dumps(CFLAGS), env_file_path)
    env_helper.set_env('CXXFLAGS', json.dumps(CXXFLAGS), env_file_path)

    # 根目录
    env_helper.set_env('ROOT_DIR', root_dir, env_file_path)
    # src目录
    env_helper.set_env('SRC_DIR', path_helper.path_join(root_dir, 'src'), env_file_path)
    # target目录写入环境变量，用于指定生成文件存放位置
    target_path = path_helper.path_join(root_dir, 'target')
    include_path = path_helper.path_join(target_path, 'include')
    libs_path = path_helper.path_join(target_path, 'libs')
    obj_path = path_helper.path_join(target_path, 'obj')
    output_path = path_helper.path_join(target_path, 'output')
    env_helper.set_env('TARGET_PATH', target_path, env_file_path)
    env_helper.set_env('INCLUDE_PATH', include_path, env_file_path)
    env_helper.set_env('LIBS_PATH', libs_path, env_file_path)
    env_helper.set_env('OBJ_PATH', obj_path, env_file_path)
    env_helper.set_env('OUTPUT_PATH', output_path, env_file_path)
    # 目标架构类型
    arch_type = "ubuntu_x86_64"
    env_helper.set_env('ARCH_TYPE', arch_type, env_file_path)

    # 系统头文件搜索路径，可以用指令查找出来，用于头文件使用规约
    SYS_INCLUDE_PATH = ["/usr/local/include", "/usr/include"]
    env_helper.set_env('SYS_INCLUDE_PATH', json.dumps(SYS_INCLUDE_PATH), env_file_path)