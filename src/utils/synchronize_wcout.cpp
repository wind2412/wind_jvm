/*
 * synchronize_wcout.cpp
 *
 *  Created on: 2017年12月8日
 *      Author: zhengxiaolin
 */

#include "utils/synchronize_wcout.hpp"

std::mutex sync_wcout::mutex{};		// TODO: 这里多模块初始化不安全吧......
