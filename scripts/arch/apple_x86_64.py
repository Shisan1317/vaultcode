from scripts.helper import env_helper
from scripts.helper import path_helper
import json

'''
    Apple macOS x86_64 / iOS ARM64 编译环境配置
    通过 APPLE_TARGET 环境变量区分 macOS 和 iOS
    @author RenJiaqi
'''
def arch_env_init(root_dir):
    env_file_path = path_helper.path_join(root_dir, ".env")
    # macOS 使用系统 clang
    COMPILER_PATH = path_helper.get_abs_path("/usr/bin")
    CC = path_helper.path_join(COMPILER_PATH, "clang")
    CXX = path_helper.path_join(COMPILER_PATH, "clang++")
    AS = path_helper.path_join(COMPILER_PATH, "clang")
    AR = path_helper.path_join(COMPILER_PATH, "ar")
    LD = path_helper.path_join(COMPILER_PATH, "ld")
    # 通用编译选项 (-D__APPLE__ 触发 Apple 平台代码路径)
    COMMON_FLAGS = ['-O2', '-Wall', '-Wextra', '-D__APPLE__']
    CFLAGS = COMMON_FLAGS + ['-std=c11']
    CXXFLAGS = COMMON_FLAGS + ['-std=c++11']
    # 写入环境变量
    env_helper.set_env('CC', CC, env_file_path)
    env_helper.set_env('CXX', CXX, env_file_path)
    env_helper.set_env('AS', AS, env_file_path)
    env_helper.set_env('AR', AR, env_file_path)
    env_helper.set_env('LD', LD, env_file_path)
    env_helper.set_env('CFLAGS', json.dumps(CFLAGS), env_file_path)
    env_helper.set_env('CXXFLAGS', json.dumps(CXXFLAGS), env_file_path)
    # 路径
    env_helper.set_env('ROOT_DIR', root_dir, env_file_path)
    env_helper.set_env('SRC_DIR', path_helper.path_join(root_dir, 'src'), env_file_path)
    target_path = path_helper.path_join(root_dir, 'target')
    env_helper.set_env('TARGET_PATH', target_path, env_file_path)
    env_helper.set_env('INCLUDE_PATH', path_helper.path_join(target_path, 'include'), env_file_path)
    env_helper.set_env('LIBS_PATH', path_helper.path_join(target_path, 'libs'), env_file_path)
    env_helper.set_env('OBJ_PATH', path_helper.path_join(target_path, 'obj'), env_file_path)
    env_helper.set_env('OUTPUT_PATH', path_helper.path_join(target_path, 'output'), env_file_path)
    env_helper.set_env('ARCH_TYPE', 'apple_x86_64', env_file_path)
    SYS_INCLUDE_PATH = ["/usr/local/include", "/usr/include"]
    env_helper.set_env('SYS_INCLUDE_PATH', json.dumps(SYS_INCLUDE_PATH), env_file_path)
