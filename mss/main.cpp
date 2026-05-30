#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <algorithm>
#include <unordered_map>
#include <array>
#include <unordered_set>
#include <iomanip>
#include <memory>
#include <cmath>
#include <cassert>
#include <chrono>
#include <limits>
#include <functional>
#include <bit>
#include <thread>
#include <mutex>


using namespace std;

#include "core.h"          // 数据结构
#include "basic.h"         // 基础逻辑分析算法（降低核心算法压力）
#include "struct.h"        // 建立棋盘的图论结构（连通块等）
#include "distrubution.h"  // 根据结构计算地雷分布
#include "probability.h"   // 根据地雷分布计算概率，生成随机分布等等
#include "zinialgo.h"      // 包含 Zini 算法的实现
#include "ioealgo.h"       // 包含 ioe 算法的实现
#include "analysiscache.h" // 管理分析结果的缓存，避免重复计算，一键导入上下文
#include "test.h"          // 包含测试函数

int main() {
	// 初级Zini测试();
	// 中级Zini测试();
	 ZNR算法测试();
   //Zini_test();
   return 0;
}