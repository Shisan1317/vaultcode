'''
    文件名:mconfig.py
    作者:002321-刘广永
    创建日期:2025-06-26
    功能描述:利用SConscript所在路径自动化生成kconfig文件
    注:每个SConscript的查找只找子一级目录,子一级存在SConscript就继续递归子一级
'''

from scripts.helper import env_helper
from scripts.helper import path_helper
from kconfiglib import Kconfig
from menuconfig import menuconfig
import os
import re

# 创建一个menuconfig的类
class MenuconfigMgmt:
    def __init__(self):
        # 获取工程根目录
        self.root_dir = env_helper.get_env('ROOT_DIR')
        # kconfig路径
        self.kconfig_dir = path_helper.path_join(env_helper.get_env('ROOT_DIR'), "scripts", "menuconfig", "kconfig")
        # .default_config路径(默认配置)
        self.default_config_dir = path_helper.path_join(env_helper.get_env('ROOT_DIR'), "scripts", "menuconfig", ".default_config")
        # .config文件位置
        self.config_dir = path_helper.path_join(env_helper.get_env('ROOT_DIR'), ".config")
        # 工程目录下所有的sconscript文件目录集合
        self.sconscript_dirs = set()
        # 目录树结构
        self.directory_tree = {}   
        # 节点索引字典
        self.node_index = {}     
    
    """
        递归查找整个工程的sconscript目录
        参数:
            cwd: 当前目录的绝对路径
        返回:
            无
    """
    def _find_sconscript_dirs(self, cwd: str):
        # 检查子目录中是否存在SConscript文件
        sconscript_path = path_helper.path_join(cwd, "SConscript")
        if path_helper.is_file(sconscript_path):
            # 获取当前路径相对于工程根目录路径
            rel_path = path_helper.get_relative_path(cwd, self.root_dir)
            # 当前目录存在SConscript,添加到集合
            self.sconscript_dirs.add(rel_path)
            # 获取当前目录的直接子目录（仅一级）
            for entry in path_helper.list_dir(cwd):
                entry_path = path_helper.path_join(cwd, entry)
                # 跳过非目录项
                if not path_helper.is_dir(entry_path):
                    continue
                # 递归执行子目录
                self._find_sconscript_dirs(entry_path)
    
    """
        构建SConscript目录树结构
        参数:
            无
        返回:
            无
    """
    def build_directory_tree(self):
        self.directory_tree = {
            'full_path': '',    # 目录全称
            'name': '',         # 节点名字
            'children': {}      # 孩子列表
        }
        # 根节点对应工程根目录
        self.node_index = {'': self.directory_tree}
        # 按路径深度排序（确保先处理父目录）
        sorted_paths = sorted(
            self.sconscript_dirs,
            key=lambda p: p.count(os.sep)
        )
        # 遍历所有SConscript目录
        for rel_path in sorted_paths:
            # 将路径拆分，例如a/b/c拆成[a,b,c]
            parts = rel_path.split(os.sep)
            current_path = ''
            parent_node = self.directory_tree
            # 逐层创建节点
            for part in parts:
                # 更新当前路径（相对路径）
                current_path = path_helper.path_join(current_path, part) if current_path else part
                # 若节点不存在则创建 
                if current_path not in self.node_index:
                    # 新节点
                    new_node = {
                        'full_path': current_path,
                        'name': part,
                        'children':{}
                    }
                    # 添加到父节点的children字典
                    parent_node['children'][part] = new_node
                    self.node_index[current_path] = new_node
                # 更新父节点和当前路径
                parent_node = self.node_index[current_path]
    
    """
        递归生成Kconfig节点
        参数:
            param node: 当前目录树节点
            param file_handle: 文件句柄
            param depth: 当前递归深度（用于缩进）
        返回:
            无
    """
    def _generate_kconfig_node(self, node: dict, file_handle, depth: int = 0):
        # 计算每行缩进
        indent = "    " * depth
        # 生成当前节点的配置项
        config_name = re.sub(r'[./-]', '_', node['full_path']).upper() 
        file_handle.write(f"{indent}config {config_name}\n")
        file_handle.write(f"{indent}    bool \"{node['name']}\"\n")
        file_handle.write(f"{indent}    default n\n\n")
        # 为子节点创建菜单（需依赖父节点选中）
        if node['children']:
            # 创建带依赖的菜单
            file_handle.write(f"{indent}menu \"{node['name']} Modules\"\n")
            file_handle.write(f"{indent}    depends on {config_name}\n\n")
            # 递归生成子节点
            for child in node['children'].values():
                self._generate_kconfig_node(child, file_handle, depth + 1)
            file_handle.write(f"{indent}endmenu\n\n")

    # 递归构建和目录保持一致的kconfig文件
    def generate_kconfig(self):
        with open(self.kconfig_dir, 'w', encoding='utf-8') as f:
            # 写入文件头部
            f.write('mainmenu "Project Module Configuration"\n\n')
            # 从根节点的子节点开始生成（跳过虚拟根节点）
            for child in self.directory_tree['children'].values():
                self._generate_kconfig_node(child, f)

    """
        自动化生成kconfig文件并打开menuconfig图形化界面
        参数:
            无
        返回:
            无
    """
    def run_menuconfig(self):
        # 递归收集所有的SConscript目录
        for entry in path_helper.list_dir(self.root_dir):
            entry_path = path_helper.path_join(self.root_dir, entry)
            # 跳过非目录项
            if not path_helper.is_dir(entry_path):
                continue
            # 递归执行子目录
            self._find_sconscript_dirs(entry_path)
        # 由于python没有树的实现，这里用字典把这棵模块的目录树抽象出来
        self.build_directory_tree()
        # 递归构建和目录保持一致的kconfig文件
        self.generate_kconfig()
        kconf = Kconfig(self.kconfig_dir)
        # 打开界面以前先判断是否需要加载默认配置
        if not path_helper.file_exists(self.config_dir) and path_helper.is_file(self.default_config_dir):
            kconf.load_config(self.default_config_dir, replace=False)
        # 打开menuconfig
        menuconfig(kconf)