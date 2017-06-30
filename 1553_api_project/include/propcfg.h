/**
 * \file propcfg.h
 * property格式的配置文件解析.
 * 因VxWorks没有xml解析，所以主要使用property格式(即KEY=VALUE格式)的配置文件，
 * 这里为这种格式的配置解析提供统一的方法，方便并简化各Adapter的开发。
 *
 *  \date 2011-03-09
 *  \author anhu.xie
 */
#ifndef PROPERTY_CONFIG_H_
#define PROPERTY_CONFIG_H_

#include "common_qiu.h"
/*
 * 读取配置项的值.
 * 从配置字符串中读取指定配置项的设置值。如果在配置字符串中未找到指定的配置项，配置值不变。
 * 配置字符串(配置文件)的格式定义如下：
 *   -# 每个配置项由配置项名称(key)和配置项值(value)两部分组成，表述为
 * key = value
 * 这样的格式，其中，等号(=)前后可以有空白，这些空白被忽略；
 *   -# 配置项之间，由分号(;)、换行(\n)、回车(\r)符号分隔，因此key和value部分都不能出现这些特殊符号；
 *   -# 所有其它内容被认为是key或者value的一部分，包括空白符号。 
 * \param cfg 待解析的配置字符串
 * \param key 要读取配置项名称
 * \param return_value 返回找到的配置项的值；如果在配置字符串中未找到指定的配置项，不会改变此参数的值
 * \tparam T 配置项值的类型.
 * 整个读取过程被设计成C++的模板函数，便于对数值类型事先进行转换，方便实用。
 */
template <typename T> void get_config_value(const std::string &cfg, const char *key, T &return_value);

//============以下为实现部分，只是为了C++程序使用方便放在这里，但是它们不是接口的一部分======================

/// std::string类型配置项读取的实现，也是其它类型实现的基础。
template <> inline void get_config_value(const std::string &cfg, const char *key, std::string &return_value) {
	std::string::size_type start_pos = cfg.find(key);
	if ( start_pos != std::string::npos ) {
		start_pos += strlen(key);
		// skip leading white spaces
		while ( start_pos != std::string::npos && isspace(cfg[start_pos]) )
			++start_pos;
		if ( start_pos != std::string::npos && cfg[start_pos] == '=' ) {
			++start_pos;
			// skip trailing white spaces
			while ( start_pos != std::string::npos && isspace(cfg[start_pos]) )
				++start_pos;
			std::string::size_type end_pos = cfg.find_first_of(";\n\r", start_pos);
			if ( end_pos != std::string::npos ) {
				while ( isspace(cfg[--end_pos]) )
					;
			}
			return_value = cfg.substr(start_pos, end_pos != std::string::npos ? end_pos - start_pos + 1 : end_pos);
		}
	}
}

/// 浮点数类型配置项读取的实现。
template <> inline void get_config_value(const std::string &cfg, const char *key, double &return_value) {
	std::string val;
	get_config_value(cfg, key, val);
	if ( !val.empty() )
		return_value = atof(val.c_str());
}

/// 布尔型配置项读取的实现。
template <> inline void get_config_value(const std::string &cfg, const char *key, bool &return_value) {
	std::string val;
	get_config_value(cfg, key, val);
	if ( !val.empty() )
		return_value = val == "YES" || val == "yes" || val == "Yes" || val == "TRUE" || val == "true" || val == "True" || val == "1";
}

/// 各整数类型配置项读取的实现。
template <typename T> inline void get_config_value(const std::string &cfg, const char *key, T &return_value) {
	std::string val;
	get_config_value(cfg, key, val);
	if ( !val.empty() )
		return_value = atoi(val.c_str());
}

#endif /* PROPERTY_CONFIG_H_ */
