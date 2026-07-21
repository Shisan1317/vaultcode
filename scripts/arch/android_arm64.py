from scripts.helper import env_helper
from scripts.helper import path_helper
import json

'''
    Android ARM64 编译环境配置 (NDK cross-compile)
    @author RenJiaqi
'''
def arch_env_init(root_dir):
    env_file_path = path_helper.path_join(root_dir, ".env")
    # NDK 编译器地址(需根据实际NDK路径调整)
    COMPILER_PATH = path_helper.get_abs_path("/usr/local/android-ndk/toolchains/llvm/prebuilt/linux-x86_64/bin")
    # C 编译器 (aarch64-linux-android21 为 API level 21, 可根据需求调整)
    CC = path_helper.path_join(COMPILER_PATH, "aarch64-linux-android21-clang")
    CXX = path_helper.path_join(COMPILER_PATH, "aarch64-linux-android21-clang++")
    AS = path_helper.path_join(COMPILER_PATH, "aarch64-linux-android21-clang")
    AR = path_helper.path_join(COMPILER_PATH, "llvm-ar")
    LD = path_helper.path_join(COMPILER_PATH, "ld.lld")
    # 通用编译选项
    COMMON_FLAGS = ['-O2', '-Wall', '-Wextra', '-fPIC', '-DANDROID', '-D__ANDROID_API__=21']
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
    env_helper.set_env('ARCH_TYPE', 'android_arm64', env_file_path)
    SYS_INCLUDE_PATH = ["/usr/local/android-ndk/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include"]
    env_helper.set_env('SYS_INCLUDE_PATH', json.dumps(SYS_INCLUDE_PATH), env_file_path)
