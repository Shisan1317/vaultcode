'''
    文件名:env_helper.py
    作者:002321-刘广永
    创建日期:2025-06-09
    功能描述:用于生成和读写环境变量，一个工程只认工程目录下的.env文件,禁止使用系统环境变量。
'''

from scripts.helper import path_helper
import os
from dotenv import load_dotenv, set_key

'''
    获取指定环境变量的值。
    参数：
        key (str): 要查询的环境变量名称（大小写不敏感）。
        default (str): 当键不存在时返回的默认值,默认为None。
    返回：
        str: 环境变量的值字符串,若不存在则返回default。
    异常：
        无。
'''
def get_env(key: str, default: str = None) -> str:
    return os.environ.get(key, default)

'''
    设置环境变量并持久化到工程配置文件。
    参数：
        key (str): 环境变量名称。
        value (str): 环境变量值。
        env_file (str): 目标.env文件路径。
    返回：
        无。
    异常：
        文件不存在抛出异常。
'''
def set_env(key: str, value: str, env_file: str) -> None:
    if not path_helper.file_exists(env_file):
        raise FileNotFoundError(f".env文件不存在: {env_file}")
    os.environ[key] = value   
    # 写入文件
    set_key(env_file, key, value)  

'''
    重新生成空的.env文件。
    参数：
        env_file (str): 环境文件路径。
    返回：
        无。
    异常：
        无。
'''
def init_env(env_file: str) -> None:
    if path_helper.file_exists(env_file):
        path_helper.remove(env_file)
    open(env_file, 'a').close()

'''
    从.env文件加载环境变量到当前进程。
    参数：
        env_file (str): 环境文件路径。
    返回：
        无。
    异常：
        无。
'''
def load_env(env_file: str) -> None:
    if path_helper.file_exists(env_file):
        # 覆盖式加载
        load_dotenv(env_file, override=True)  

'''
    删除.env文件。
    参数：
        env_file (str): 环境文件路径。
    返回：
        无。
    异常：
        无。
'''
def del_env(env_file: str) -> None:
    if path_helper.file_exists(env_file):
        path_helper.remove(env_file)