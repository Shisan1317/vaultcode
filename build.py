'''
    文件名:build.py
    作者:002321-刘广永
    创建日期:2025-06-09
    功能描述:编译脚本的统一执行入口。
'''

from scripts.helper import env_helper
from scripts.helper import path_helper
from scripts.menuconfig import mconfig
import argparse
import importlib.util
import subprocess
import os

# 注册参数,所有传参请在这里声明
parser = argparse.ArgumentParser(description='build.py的帮助信息')

parser.add_argument('--arch', 
                    type=str, 
                    help='指定架构文件的名字(不含.py扩展名)')

parser.add_argument('--update', 
                    action='store_true', 
                    help='暂未实现')

parser.add_argument('--menuconfig', 
                    action='store_true', 
                    help='打开menuconfig配置模块')

parser.add_argument('--clear', 
                    nargs='?',
                    const='code', 
                    help='不传参-只清除程序生成文件; all-程序生成文件和脚本缓存的文件一起清除')

parser.add_argument('--build', 
                    nargs='?', 
                    const='code', 
                    help='不传参-只编译代码; all-编译全部   (注:编译前先指定arch文件)')

args, unknown_args = parser.parse_known_args()

# 获取根目录
root_dir = path_helper.current_dir()
# .env文件路径
env_file_path = path_helper.path_join(root_dir, ".env")

# 这里执行清除操作
if args.clear:
    if f"{args.clear}" == "code" or f"{args.clear}" == "all":
        try:
            # 使用 Popen 实时获取输出
            process = subprocess.Popen(
                ["scons", "-c"],           # 清除命令
                stdout=subprocess.PIPE,    # 捕获标准输出
                stderr=subprocess.STDOUT,  # 将错误重定向到标准输出
                text=True,                 # 文本模式输出
                bufsize=1,                 # 行缓冲模式（确保实时输出）
                universal_newlines=True    # 兼容不同系统的换行符
            )
            # 实时读取并打印输出
            while True:
                output = process.stdout.readline()
                if output == '' and process.poll() is not None:
                    break
                if output:
                    print(output.strip(), flush=True)  # 立即刷新输出
            # 检查返回码（非0表示失败）
            return_code = process.wait()
            if return_code != 0:
                raise RuntimeError(f"scons -c 执行失败! 返回码: {return_code}")
        except KeyboardInterrupt:
            # 捕获 Ctrl+C 并终止子进程
            print("\n[中断] 检测到用户终止请求，正在停止清除操作...")
            process.terminate()  # 先尝试优雅终止
            try:
                process.wait(timeout=2)  # 等待2秒
            except subprocess.TimeoutExpired:
                process.kill()  # 强制终止
            raise RuntimeError("清除操作已被用户中断")
        except Exception as e:
            raise RuntimeError(f"scons -c 执行失败! 错误: {str(e)}")
    # 深度清理
    if f"{args.clear}" == "all":
        # 遍历整个目录删除__pycache__文件
        for root, dirs, files in os.walk(root_dir):
            # 删除 __pycache__ 文件夹
            if "__pycache__" in dirs:
                pycache_path = path_helper.path_join(root, "__pycache__")
                try:
                    path_helper.remove(pycache_path)
                except Exception as e:
                    raise RuntimeError(f"删除目录失败 [{pycache_path}]: {str(e)}")
        # 删除.env文件
        try:
            path_helper.remove(env_file_path)
        except Exception as e:
            raise RuntimeError(f"删除文件失败 [{env_file_path}]: {str(e)}")
        # 删除 .sconsign.dblite 文件
        sconsign_path = path_helper.path_join(root_dir, ".sconsign.dblite")
        try:
            path_helper.remove(sconsign_path)
        except Exception as e:
            raise RuntimeError(f"删除文件失败 [{sconsign_path}]: {str(e)}")
        # 删除 .config 文件
        config_path = path_helper.path_join(root_dir, ".config")
        try:
            path_helper.remove(config_path)
        except Exception as e:
            raise RuntimeError(f"删除文件失败 [{config_path}]: {str(e)}")
        # 删除 .config.old 文件
        config_old_path = path_helper.path_join(root_dir, ".config.old")
        try:
            path_helper.remove(config_old_path)
        except Exception as e:
            raise RuntimeError(f"删除文件失败 [{config_old_path}]: {str(e)}")
        # 删除 /scripts/menuconfig/kconfig 文件
        kconfig_path = path_helper.path_join(root_dir, "scripts", "menuconfig", "kconfig")
        try:
            path_helper.remove(kconfig_path)
        except Exception as e:
            raise RuntimeError(f"删除文件失败 [{kconfig_path}]: {str(e)}")
        # 删除 target目录
        target_path = path_helper.path_join(root_dir, "target")
        try:
            path_helper.remove(target_path)
        except Exception as e:
            raise RuntimeError(f"删除文件失败 [{target_path}]: {str(e)}")
        
# arch文件所在目录
arch_dir = path_helper.get_abs_path(path_helper.path_join(root_dir, "scripts/arch"))
# 传入了架构名字
if args.arch:
    # arch文件路径
    arch_file_path = path_helper.path_join(arch_dir, f"{args.arch}.py")
    if not path_helper.file_exists(arch_file_path):
        raise FileNotFoundError(f"arch文件不存在: {arch_file_path}")
    # 重新加载.env文件
    env_helper.init_env(env_file_path)
    try:
        # 动态加载模块
        module_name = f"arch_{args.arch}"   
        # 加载指定路径下的配置文件
        spec = importlib.util.spec_from_file_location(module_name, arch_file_path)
        arch_module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(arch_module)
        # 加载这个模块里面的arch_env_init函数
        arch_env_init_func = getattr(arch_module, "arch_env_init")
        arch_env_init_func(root_dir)
    except Exception as e:
        raise RuntimeError(f"架构初始化失败: {str(e)}")
    print("arch init success!!!")

if path_helper.file_exists(env_file_path):
    # 从.env文件中加载环境变量
    env_helper.load_env(env_file_path)

# menuconfig 指令
if args.menuconfig:
    # 判断环境变量中的root_dir是否对，不对证明环境配置有错，不允许进行编译打包操作
    if env_helper.get_env('ROOT_DIR') != root_dir:
        raise RuntimeError(f"编译前请先重新加载arch文件!")
    # 实例化一个menuconfig类
    menuconfig_mgmt = mconfig.MenuconfigMgmt()
    # 打开menuconfig界面
    menuconfig_mgmt.run_menuconfig()

# 这里开始编译操作
if args.build:
    # 判断环境变量中的root_dir是否对，不对证明环境配置有错，不允许进行编译打包操作
    if env_helper.get_env('ROOT_DIR') != root_dir:
        raise RuntimeError(f"编译前请先重新加载arch文件!")
    if f"{args.build}" == "code" or f"{args.build}" == "all":
        try:
            # 使用Popen实时获取输出
            process = subprocess.Popen(
                ["scons"],
                stdout=subprocess.PIPE,      # 捕获标准输出
                stderr=subprocess.STDOUT,    # 将错误重定向到标准输出
                text=True,                   # 文本模式输出
                bufsize=1,                   # 行缓冲模式
                universal_newlines=True      # 确保文本模式兼容性
            )
            # 实时读取并打印输出
            while True:
                output = process.stdout.readline()
                if output == '' and process.poll() is not None:
                    break
                if output:
                    print(output.strip())
            # 等待进程结束并检查返回码
            return_code = process.wait()
            if return_code != 0:
                raise RuntimeError(f"scons 构建失败! 返回码: {return_code}")
        except KeyboardInterrupt:
            # 捕获Ctrl+C并终止子进程
            print("\n检测到Ctrl+C,正在终止scons进程...")
            process.terminate()  # 先尝试优雅终止
            try:
                process.wait(timeout=2)  # 等待2秒
            except subprocess.TimeoutExpired:
                process.kill()  # 强制终止
            raise RuntimeError("构建过程已被用户中断")
        except Exception as e:
            raise RuntimeError(f"scons 构建失败! 错误信息: {str(e)}")