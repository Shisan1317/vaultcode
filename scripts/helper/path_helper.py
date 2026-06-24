'''
    文件名:path_helper.py
    作者:002321-刘广永
    创建日期:2025-06-09
    功能描述:提供统一的操作文件或者目录路径的接口,屏蔽linux路径“/”以及windows路径“\”差异。
'''

import os
import shutil
from pathlib import Path
from typing import List

'''
    用于自动将字符串拼接成一个完整的路径。
    参数：
        parts(字符串指针):如 path_join("docs", "2025", "report.txt")。
    返回：
        （字符串）：完整的路径。
    异常：
        无。
'''
def path_join(*parts: str) -> str:
    return str(Path(*parts))

'''
    根据传入的路径，递归创建目录。
    参数：
        path(字符串):要创建目录路径。
    返回：
        无。
    异常：
        无。
'''
def dir_create(path: str) -> None:
    Path(path).mkdir(parents=True, exist_ok=True)

'''
    删除文件/目录。
    参数：
        path(字符串):要删除的文件或者目录。
    返回：
        无。
    异常：
        无。
'''
def remove(path: str) -> None:
    # 跨平台解析路径
    path_obj = Path(path)
    if not path_obj.exists():
        return
    # 如果是文件
    if path_obj.is_file():
        path_obj.unlink()
    # 如果是目录
    elif path_obj.is_dir():
        shutil.rmtree(path_obj)

'''
    复制文件/目录（保留元数据）。
    参数：
        src(字符串):要拷贝的目录或文件路径。
        dst(字符串):目标的目录或文件路径。
    返回：
        无。
    异常：
        无。
'''
def copy(src: str, dst: str) -> None:
    # 跨平台转换一下目录
    src_obj, dst_obj = Path(src), Path(dst)
    # 确保目标目录存在
    if not dst_obj.parent.exists():
        os.makedirs(dst_obj.parent, exist_ok=True)
    # 复制文件
    if src_obj.is_file():
        shutil.copy2(src_obj, dst_obj)
    # 复制目录，允许目录已存在
    elif src_obj.is_dir():
        shutil.copytree(src_obj, dst_obj, dirs_exist_ok=True)

'''
    移动/重命名文件或目录。
    参数：
        src(字符串):要拷贝的目录或文件路径。
        dst(字符串):目标的目录或文件路径。
    返回：
        无。
    异常：
        无。
'''
def move(src: str, dst: str) -> None:
    shutil.move(src, dst)

'''
    列出目录内容。
    参数：
        path(字符串):要显示的路径。
        pattern(字符串):过滤条件的通配符，如“*.c”,默认是“*”。
        show_hidden_files(布尔):是否显示隐藏文件，默认显示。
    返回：
        字符串列表。
    异常：
        无。
'''
def list_dir(path: str, pattern: str = "*", show_hidden_files: bool = True) -> List[str]:
    # 作路径转换
    path_obj = Path(path)
    # 根据通配符筛选文件
    items = [p.name for p in path_obj.glob(pattern)]
    # 如果不需要隐藏文件
    if not show_hidden_files:
        items = [item for item in items if not item.startswith('.')]
    return items

'''
    获取绝对路径（自动解析相对路径）。
    参数：
        path(字符串):要转换的路径。
    返回：
        （字符串）：绝对路径。
    异常：
        无。
'''
def get_abs_path(path: str) -> str:
    return str(Path(path).resolve())

'''
    检查是否为文件。
    参数：
        path(字符串):要检查的路径。
    返回：
        （布尔）：是否是文件。
    异常：
        无。
'''
def is_file(path: str) -> bool:
    return Path(path).is_file()

'''
    检查是否为目录。
    参数：
        path(字符串):要检查的路径。
    返回：
        （布尔）：是否是目录。
    异常：
        无。
'''
def is_dir(path: str) -> bool:
    return Path(path).is_dir()

'''
    切换工作目录。
    参数：
        path(字符串):要切换的工作路径。
    返回：
        无。
    异常：
        无。
'''
def change_dir(path: str) -> None:
    os.chdir(path)

'''
    获取当前目录路径。
    参数：
        无。
    返回：
        （字符串）：当前目录路径。
    异常：
        无。
'''
def current_dir() -> str:
    return os.getcwd()

'''
    判断文件是否存在。
    参数：
        path(字符串)：检查的文件路径。
    返回：
        文件是否存在。
    异常：
        无。
'''
def file_exists(path: str) -> bool:
    return Path(path).exists()

'''
    判断是否是绝对路径。
    参数：
        path(字符串)：检查的文件路径。
    返回：
        路径是否是绝对路径。
    异常：
        无。
'''
def is_abs_path(path: str) -> bool:
    return os.path.isabs(path)

'''
    计算从 base_path 到 target_path 的相对路径。
    参数：
        arget_path (str): 目标绝对路径
        base_path (str): 基准绝对路径
    返回：
        str: 相对路径字符串
    异常：
        无。
'''
def get_relative_path(target_path: str, base_path: str) -> str:
    return os.path.relpath(target_path, base_path)

'''
    获取path的基础路径
    参数：
        path:要查询的路径
    返回：
        str: 返回路径最后一个目录字符串,例如a/b/c,那么结果返回c
    异常：
        无。
'''
def get_base_path(path: str) -> str:
    return  os.path.basename(path)