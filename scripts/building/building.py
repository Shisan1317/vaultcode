'''
    文件名:building.py
    作者:002321-刘广永
    创建日期:2025-06-26
    功能描述:编译根脚本。
'''

from SCons.Script import *
from scripts.helper import env_helper
from scripts.helper import path_helper
import json
import glob
import re
import os

"""
    合并两个列表并去重，保留原始顺序
    参数:
        base_list: 基础列表（保留其原始顺序）
        additional_list: 要添加的额外列表（可选）
    返回:
        合并并去重后的新列表
"""
def merge_unique_lists(base_list, additional_list=None):
    seen = set()
    result = []
    # 处理基础列表
    for item in base_list:
        if item not in seen:
            seen.add(item)
            result.append(item)
    # 处理额外列表
    if additional_list is not None:
        for item in additional_list:
            if item not in seen:
                seen.add(item)
                result.append(item)
    return result

# 定义一个builder类，所有的编译方法由这个类提供
class Builder:
    # 构造函数，支持用户传入自定义编译选项
    def __init__(self, flags_list: list = None):

        # 如果list未传参，将flags初始化为空列表
        flags_list = flags_list or []
        # C 编译器路径
        self.CC = env_helper.get_env('CC')
        # C++ 编译器路径
        self.CXX = env_helper.get_env('CXX')
        # 汇编器路径
        self.AS = env_helper.get_env('AS')
        # 静态库归档工具
        self.AR = env_helper.get_env('AR')
        # 链接器路径
        self.LD = env_helper.get_env('LD')
        # 获取工程根目录
        self.root_dir = env_helper.get_env('ROOT_DIR')
        # 获取工程源码目录
        self.src_dir = env_helper.get_env('SRC_DIR')
        # 获取目标文件存放目录
        self.target_dir = env_helper.get_env('TARGET_PATH')
        # 获取目标头文件存放目录
        self.include_dir = env_helper.get_env('INCLUDE_PATH')
        # 获取依赖库存放目录
        self.libs_dir = env_helper.get_env('LIBS_PATH')
        # 获取.o文件存放目录
        self.obj_dir = env_helper.get_env('OBJ_PATH')
        # 获取最终输出文件存放目录
        self.output_dir = env_helper.get_env('OUTPUT_PATH')
        # .config文件路径
        self.config_path = path_helper.path_join(env_helper.get_env('ROOT_DIR'), ".config")
        # 加载环境变量中的列表，处理可能的空值
        env_cflags = json.loads(env_helper.get_env('CFLAGS') or '[]')
        env_cxxflags = json.loads(env_helper.get_env('CXXFLAGS') or '[]')
        env_sys_include = json.loads(env_helper.get_env('SYS_INCLUDE_PATH') or '[]')
        # 使用封装函数处理编译选项
        self.CFLAGS = merge_unique_lists(env_cflags, flags_list)
        self.CXXFLAGS = merge_unique_lists(env_cxxflags, flags_list)
        self.sys_include_path = merge_unique_lists(env_sys_include)
        # 收集编译过程中的产物
        self.build_collection = {
            "dep_lib": [],                  # 依赖库列表，保持用户传入的依赖库顺序
            "dep_lib_path": [],             # 依赖库路径列表
            "module": {}                    # 所有模块信息的字典
        }

    """
        根据黑白名单过滤源文件路径
        参数:
            cwd: 当前目录的绝对路径
            white_list: 白名单列表（相对路径/绝对路径/通配符）
            black_list: 黑名单列表（相对路径/绝对路径/通配符）
        返回:
            过滤后的源文件绝对路径列表
    """
    def filter_src_files(self, cwd: str, 
                         white_list: list = None, 
                         black_list: list = None) -> list:
        # 初始化列表
        white_list = white_list or []
        black_list = black_list or []
        # 处理文件模式的辅助函数
        def process_patterns(patterns, base_dir):
            result = []
            seen = set()
            for pattern in patterns:
                # 处理相对路径
                if not path_helper.is_abs_path(pattern):
                    abs_pattern = path_helper.path_join(base_dir, pattern)
                else:
                    abs_pattern = pattern
                # 处理通配符
                if "**" in abs_pattern:
                    matched = glob.glob(abs_pattern, recursive=True)
                elif "*" in abs_pattern:
                    matched = glob.glob(abs_pattern)
                else:
                    matched = [abs_pattern] if path_helper.file_exists(abs_pattern) else []
                # 添加有效文件
                for file in matched:
                    # 确保文件存在且是文件类型
                    if path_helper.is_file(file) and file not in seen:
                        result.append(file)
                        seen.add(file)
            return result
        # 获取白名单和黑名单文件
        white_files = process_patterns(white_list, cwd)
        black_files = process_patterns(black_list, cwd)
        # 创建黑名单集合用于快速查找
        black_set = set(black_files)
        # 过滤白名单文件，排除黑名单文件
        result = []
        seen = set()
        for file in white_files:
            if file not in black_set and file not in seen:
                result.append(file)
                seen.add(file)
        return result

    """
        检查源文件中包含的头文件是否能在头文件列表中匹配多个，匹配多个直接抛出异常(（路径冲突检测）)
        参数:
            src_files: 源码列表
            all_include_paths: 头文件列表,里面是绝对路径
        返回:
            无，异常直接退出
    """
    def _check_header(self, src_files: list, all_include_paths: list):
        # 匹配正则表达式，找到.c文件里面include的头文件名
        include_pattern = re.compile(r'^\s*#\s*include\s*["<]([^">]+)[">]')
        # 缓存头文件查找结果，避免重复检查(该头文件前面已经检查合法了没必要再查)
        header_cache = {}
        for src_file in src_files:
            # 尝试读取文件，文件必须是utf-8编码
            try:
                with open(src_file, 'r', encoding='utf-8') as f:
                    lines = f.readlines()
            except UnicodeDecodeError:
                raise RuntimeError(f"文件读取失败:{src_file}")
            # 逐行读取文件
            for line_num, line in enumerate(lines, 1):
                # 查看是否匹配正则表达式
                match = include_pattern.match(line)
                if not match:
                    continue
                # 匹配上直接提取include里面的字段
                header_name = match.group(1)
                # 如果include里面的是个绝对路径，那没必要去查，不可能冲突
                if path_helper.is_abs_path(header_name):
                    continue 
                # 先检查这个字段是否前面已经排查过了
                if header_name in header_cache:
                    found_paths = header_cache[header_name]
                else:
                    found_paths = []
                    # 挨个匹配所有路径
                    for inc_dir in all_include_paths:
                        # 尝试进行路径拼接
                        test_path = path_helper.path_join(inc_dir, header_name)
                        # 如果文件存在
                        if path_helper.file_exists(test_path) and path_helper.is_file(test_path):
                            # 把该文件加入到列表里面
                            found_paths.append(test_path)
                    # 将结果缓存到字典里面
                    header_cache[header_name] = found_paths
                # 路径冲突检测，如果匹配结果不止一个，那就是头文件包含非法
                if len(found_paths) > 1:
                    error_msg = (
                        f"头文件冲突: '{header_name}' 出现在多个路径中:\n"
                        f"  源文件: {src_file}:{line_num}\n"
                        f"  冲突路径: {found_paths}\n"
                    )
                    raise RuntimeError(error_msg)

    """
        把单个模块隔离编译成.o文件
        参数:
            cwd: 当前目录的绝对路径
            src_files: 源码列表
            include_path_list: 头文件列表，可以是绝对路径或相对路径
            dep_lib_list: 依赖库列表
            dep_lib_path_list: 依赖库路径列表，可以是绝对路径或相对路径
            flags_list: 编译配置选项列表
            self_flag_list: 只对模块侧自己开放的编译宏
        返回:
            无
    """
    def build_module(self, cwd: str,
                    src_files: list = None, 
                    include_path_list: list = None, 
                    dep_lib_list: list = None, 
                    dep_lib_path_list: list = None, 
                    flags_list: list = None,
                    self_flag_list: list = None):
        # 初始化传参
        src_files = src_files or []
        include_path_list = include_path_list or []
        dep_lib_list = dep_lib_list or []
        dep_lib_path_list = dep_lib_path_list or []
        flags_list = flags_list or []
        self_flag_list = self_flag_list or []
        # 过滤源码，去掉不存在的文件并去重
        valid_src_files = []
        seen_src = set()
        for file_path in src_files:
            if file_path not in seen_src and path_helper.is_file(file_path):
                valid_src_files.append(file_path)
                seen_src.add(file_path)
        src_files = valid_src_files
        # 将include_path_list转换成绝对路径并去重
        abs_include_paths = []
        seen_include = set()
        for p in include_path_list:
            # 转换为绝对路径
            if path_helper.is_abs_path(p):
                abs_path = p
            else:
                abs_path = path_helper.get_abs_path(path_helper.path_join(cwd, p)) 
            # 检查是否已存在
            if abs_path not in seen_include:
                abs_include_paths.append(abs_path)
                seen_include.add(abs_path)
        # 合并用户自定义头文件路径和系统头文件路径
        all_include_paths = merge_unique_lists(abs_include_paths, self.sys_include_path)
        # 需要用正则表达式检查这个模块所有包含的.c文件头文件是否冲突
        self._check_header(src_files, all_include_paths)
        # 依赖库按顺序添加
        for lib in dep_lib_list:
            if lib not in self.build_collection["dep_lib"]:
                self.build_collection["dep_lib"].append(lib)
        # 将依赖库路径转换成绝对路径并去重
        abs_dep_lib_paths = []
        seen_lib_path = set()
        for p in dep_lib_path_list:
            # 转换为绝对路径
            if path_helper.is_abs_path(p):
                abs_path = p
            else:
                abs_path = path_helper.get_abs_path(path_helper.path_join(cwd, p))
            # 检查是否已存在
            if abs_path not in seen_lib_path:
                abs_dep_lib_paths.append(abs_path)
                seen_lib_path.add(abs_path)
        # 依赖库路径合并到全局列表（去重）
        self.build_collection["dep_lib_path"] = merge_unique_lists(
            self.build_collection["dep_lib_path"], 
            abs_dep_lib_paths
        )
        # 编译配置选项合并到全局列表（去重）
        self.CFLAGS = merge_unique_lists(self.CFLAGS, flags_list)
        self.CXXFLAGS = merge_unique_lists(self.CXXFLAGS, flags_list)
        # 模块自己的宏编译开关去重
        if not self_flag_list:
            self_flag_list = merge_unique_lists([], self_flag_list)
        # 如果源码文件为空，可以直接返回
        if not src_files:
            return
        # 只收集模块信息不编译文件
        module_info = {
            'src': src_files,                   # 模块源码
            'include_path': abs_include_paths,  # 模块头文件
            'self_flag_list': self_flag_list    # 模块自己的宏编译开关
        }
        # 添加到全局模块字典,以模块名作键
        self.build_collection["module"][cwd] = module_info

    """
        将收集的所有.o文件链接成可执行文件
        参数:
            target_name: 可执行文件名,最后会生成一个可执行文件存放于target/output目录下
        返回:
            无
    """
    def do_building(self, target_name: str):
        # 收集所有的.o文件列表（保持顺序且唯一）
        obj_files = []
        seen_objs = set()  # 用于跟踪已添加的目标文件
        # 编译所有模块，使用静态编译方法
        for module_name, module_info in self.build_collection["module"].items():
            # 打印这个模块的编译信息
            print(f"\n模块路径: {module_name}")
            print("模块源码:", module_info["src"])
            print("模块头文件路径:", module_info["include_path"], "\n")
            # 合并模块侧自己的宏
            module_cflag_list = merge_unique_lists(self.CFLAGS, module_info["self_flag_list"])
            module_cxxflag_list = merge_unique_lists(self.CXXFLAGS, module_info["self_flag_list"])
            # 给每个编译模块单独创建环境变量，实现隔离编译
            module_env = Environment(
                CC=self.CC,                                             # 指定gcc编译链
                CXX=self.CXX,                                           # 指定g++编译连
                AS=self.AS,                                             # 指定汇编编译器     
                AR=self.AR,                                             # 指定静态库归档工具
                LD=self.LD,                                             # 指定链接器
                CFLAGS=module_cflag_list,                               # 指定c编译配置选项
                CXXFLAGS=module_cxxflag_list,                           # 指定c++编译配置选项
                CPPPATH=module_info["include_path"],                    # 指定该模块编译的头文件路径   
            )
            # 基于这个环境将该模块的.c和.cpp文件编译成.o文件
            for src_file in module_info["src"]:
                # 获取源文件相对路径
                rel_path = path_helper.get_relative_path(src_file, self.root_dir)
                # 构造目标文件路径（保留目录结构）
                obj_path = path_helper.path_join(self.obj_dir, os.path.splitext(rel_path)[0] + '.o')
                # 检查是否已添加此目标文件
                if obj_path not in seen_objs:
                    # 添加到目标文件列表
                    obj_files.append(obj_path)
                    seen_objs.add(obj_path)
                    # 把单个文件编译成.o文件
                    module_env.Object(
                        target=obj_path,      # 目标文件
                        source=src_file       # 源文件
                    )
        # 单独创建环境变量，实现隔离链接整个程序
        build_env = Environment(
            CC=self.CC,                                             # 指定gcc编译链
            CXX=self.CXX,                                           # 指定g++编译连
            AS=self.AS,                                             # 指定汇编编译器     
            AR=self.AR,                                             # 指定静态库归档工具
            LD=self.LD,                                             # 指定链接器
            LIBPATH=self.build_collection["dep_lib_path"],          # 指定依赖库路径
            LIBS=self.build_collection["dep_lib"]                   # 指定依赖库
        )
        # 构造目标文件路径
        target_path = path_helper.path_join(self.output_dir, target_name)
        if obj_files:
            # 编译成可执行文件
            build_env.Program(
                target=target_path,    # 目标文件
                source=obj_files       # 源文件
            )
        else:
            raise ValueError("warning:obj_files为空,跳过编译")

    """
        将收集的所有.o文件链接成静态库
        参数:
            target_name: 静态库文件名,最后会生成一个静态库存放于target/output目录下
        返回:
            无
    """
    def build_static_library(self, target_name: str):
        # 收集所有的.o文件列表（保持顺序且唯一）
        obj_files = []
        seen_objs = set()  # 用于跟踪已添加的目标文件
        # 编译所有模块，使用静态编译方法
        for module_name, module_info in self.build_collection["module"].items():
            # 打印这个模块的编译信息
            print(f"\n模块路径: {module_name}")
            print("模块源码:", module_info["src"])
            print("模块头文件路径:", module_info["include_path"], "\n")
            # 合并模块侧自己的宏
            module_cflag_list = merge_unique_lists(self.CFLAGS, module_info["self_flag_list"])
            module_cxxflag_list = merge_unique_lists(self.CXXFLAGS, module_info["self_flag_list"])
            # 给每个编译模块单独创建环境变量，实现隔离编译
            module_env = Environment(
                CC=self.CC,                                             # 指定gcc编译链
                CXX=self.CXX,                                           # 指定g++编译连
                AS=self.AS,                                             # 指定汇编编译器     
                AR=self.AR,                                             # 指定静态库归档工具
                LD=self.LD,                                             # 指定链接器
                CFLAGS=module_cflag_list,                               # 指定c编译配置选项
                CXXFLAGS=module_cxxflag_list,                           # 指定c++编译配置选项
                CPPPATH=module_info["include_path"],                    # 指定该模块编译的头文件路径   
            )
            # 基于这个环境将该模块的.c和.cpp文件编译成.o文件
            for src_file in module_info["src"]:
                # 获取源文件相对路径
                rel_path = path_helper.get_relative_path(src_file, self.root_dir)
                # 构造目标文件路径（保留目录结构）
                obj_path = path_helper.path_join(self.obj_dir, os.path.splitext(rel_path)[0] + '.o')
                # 检查是否已添加此目标文件
                if obj_path not in seen_objs:
                    # 添加到目标文件列表
                    obj_files.append(obj_path)
                    seen_objs.add(obj_path)
                    # 把单个文件编译成.o文件
                    module_env.Object(
                        target=obj_path,      # 目标文件
                        source=src_file       # 源文件
                    )
        # 单独创建环境变量，实现隔离链接整个程序
        build_env = Environment(
            CC=self.CC,                                             # 指定gcc编译链
            CXX=self.CXX,                                           # 指定g++编译连
            AS=self.AS,                                             # 指定汇编编译器     
            AR=self.AR,                                             # 指定静态库归档工具
            LD=self.LD,                                             # 指定链接器
        )
        # 构造目标文件路径
        target_path = path_helper.path_join(self.output_dir, "static_lib", target_name)
        if obj_files:
            # 编译成静态库
            build_env.StaticLibrary(
                target = target_path,       # 目标文件
                source = obj_files          # 源文件
            )
        else:
            raise ValueError("warning:obj_files为空,跳过编译")

    """
        将收集的所有.o文件链接成动态库
        参数:
            target_name: 动态库文件名,最后会生成一个动态库存放于target/output目录下
        返回:
            无
    """
    def build_shared_library(self, target_name: str):
        # 收集所有的.o文件列表（保持顺序且唯一）
        obj_files = []
        seen_objs = set()  # 用于跟踪已添加的目标文件
        # 编译所有模块，使用动态编译方法
        for module_name, module_info in self.build_collection["module"].items():
            # 打印这个模块的编译信息
            print(f"\n模块路径: {module_name}")
            print("模块源码:", module_info["src"])
            print("模块头文件路径:", module_info["include_path"], "\n")
            # 合并模块侧自己的宏
            module_cflag_list = merge_unique_lists(self.CFLAGS, module_info["self_flag_list"])
            module_cxxflag_list = merge_unique_lists(self.CXXFLAGS, module_info["self_flag_list"])
            # 给每个编译模块单独创建环境变量，实现隔离编译
            module_env = Environment(
                CC=self.CC,                                             # 指定gcc编译链
                CXX=self.CXX,                                           # 指定g++编译连
                AS=self.AS,                                             # 指定汇编编译器     
                AR=self.AR,                                             # 指定静态库归档工具
                LD=self.LD,                                             # 指定链接器
                CFLAGS=module_cflag_list,                               # 指定c编译配置选项
                CXXFLAGS=module_cxxflag_list,                           # 指定c++编译配置选项
                CPPPATH=module_info["include_path"],                    # 指定该模块编译的头文件路径   
            )
            # 基于这个环境将该模块的.c和.cpp文件编译成.o文件
            for src_file in module_info["src"]:
                # 获取源文件相对路径
                rel_path = path_helper.get_relative_path(src_file, self.root_dir)
                # 构造目标文件路径（保留目录结构）
                obj_path = path_helper.path_join(self.obj_dir, os.path.splitext(rel_path)[0] + '.os')
                # 检查是否已添加此目标文件
                if obj_path not in seen_objs:
                    # 添加到目标文件列表
                    obj_files.append(obj_path)
                    seen_objs.add(obj_path)
                    # 把单个文件编译成.o文件
                    module_env.SharedObject(
                        target = obj_path,      # 目标文件
                        source = src_file       # 源文件
                    )
        # 单独创建环境变量，实现隔离链接整个程序
        build_env = Environment(
            CC=self.CC,                                             # 指定gcc编译链
            CXX=self.CXX,                                           # 指定g++编译连
            AS=self.AS,                                             # 指定汇编编译器     
            AR=self.AR,                                             # 指定静态库归档工具
            LD=self.LD,                                             # 指定链接器
            LIBPATH=self.build_collection["dep_lib_path"],          # 指定依赖库路径
            LIBS=self.build_collection["dep_lib"]                   # 指定依赖库
        )
        # 构造目标文件路径
        target_path = path_helper.path_join(self.output_dir, "shared_lib", target_name)
        if obj_files:
            # 编译成动态库
            build_env.SharedLibrary(
                target = target_path,    # 目标文件
                source = obj_files       # 源文件
            )
        else:
            raise ValueError("warning:obj_files为空,跳过编译")

    """
        将库用到的.h文件统一拷贝出去
        参数:
            无
        返回:
            无
    """
    def copy_header(self):
        # 把库头文件全部拷贝到target/include目录下，测试需要用
        for root, dirs, files in os.walk(self.src_dir):
            # 跳过lib目录
            if root == self.src_dir and 'lib' in dirs:
                dirs.remove('lib')
            for file in files:
                if file.endswith(".h"):
                    include_file = path_helper.path_join(root, file)
                    # 计算相对路径（保持目录结构）
                    rel_path = path_helper.get_relative_path(root, self.src_dir)
                    target_path = path_helper.path_join(self.include_dir, rel_path)
                    target_file = path_helper.path_join(target_path, file)
                    path_helper.copy(include_file, target_file)
    
    """
        递归执行子一级目录的SConscript脚本
        参数:
            cwd:当前目录
        返回:
            无
    """
    def traverse_sconscript(self, cwd: str):
        for file in path_helper.list_dir(cwd):
            # 只找子一级目录的文件
            if path_helper.is_dir(path_helper.path_join(cwd, file)) and path_helper.is_file(path_helper.path_join(cwd, file, "SConscript")):
                # 执行这个脚本
                sconscript_path = path_helper.path_join(cwd, file, "SConscript")
                # 把对象指针传递给子模块
                SConscript(sconscript_path, exports={'builder': self})

    """
        根据.config配置来使能模块
        参数:
            cwd:当前目录
        返回:
            bool:当前模块是否参与编译
    """
    def get_module_switch(self, cwd: str) -> bool:
        if not path_helper.is_file(self.config_path):
            raise ValueError(f"配置文件{self.config_path}不存在")
        # 计算相对路径并标准化
        rel_path = path_helper.get_relative_path(cwd, self.root_dir)
        # 获取该模块在.config文件里面的配置名字
        symbol = re.sub(r'[./-]', '_', rel_path).upper() 
        config_key = f"CONFIG_{symbol}"
        # 模式1：启用状态 (CONFIG_XXX=y)
        enable_pattern = re.compile(rf"^{re.escape(config_key)}\s*=\s*y\s*$") 
        # 模式2：禁用状态 (# CONFIG_XXX is not set)
        disable_pattern = re.compile(rf"^#\s*{re.escape(config_key)}\s+is not set\s*$")
        # 逐行扫描配置文件
        with open(self.config_path, 'r', encoding='utf-8') as f:
            for line in f:
                line = line.strip()
                # 匹配启用状态 
                if enable_pattern.match(line):
                    return True
                # 匹配禁用状态 
                if disable_pattern.match(line):
                    return False
        # 未找到配置项时默认禁用
        return False