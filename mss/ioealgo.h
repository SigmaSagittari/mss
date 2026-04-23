#pragma once
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

using namespace std;

#include "core.h"
#include "basic.h"
#include "struct.h"
#include "distrubution.h"
#include "probability.h"
#include "zinialgo.h"

class ioealgo {
   private:
   template<typename callback>
   void highZNE_inside(const callback&cb, const GameState& state, const 基础逻辑结果& basic, const 棋盘结构& structure, const vector<连通块地雷分布>& mine_distrube
				, unsigned long long& seed, long double znereq, int cls, int algo_itr, int zini_itr = 10) {
	  // znereq 表示至少期待的 zne 的要求，cls 表示已经进行的点击次数，itr 表示计算 zini 时的迭代次数，迭代次数越高越准确但越慢。
	  for (int i = 1; i <= algo_itr; ++i) {
		 地雷排布 dist = 概率分析().gen_random(state, basic, structure, mine_distrube, seed); // 生成一个随机分布，作为 zini 的输入。
		 Zini结果 zinires = ZiniAlgo().ChainZini(state, dist, seed, 1);
		 if ((long double)zinires.bbbv / (zinires.Zini + cls) < znereq * 0.8) continue; // 如果单次迭代的 zne 过低，说明这个随机分布不具有代表性，直接丢弃。 0.8 随便写的，平时基本 0.9 都不会出岔子，但是为了保险起见，降低一点要求。
		 zinires = ZiniAlgo().ChainZini(state, dist, seed, zini_itr); // 计算这个分布的 zne，迭代次数较高以获得更准确的结果。
		 if ((long double)zinires.bbbv / (zinires.Zini + cls) < znereq)  continue; 
		 cb(dist);
	  }
   }
   public:
   高ZNE版面结果 get_highZNE(const GameState& state, const 基础逻辑结果& basic, const 棋盘结构& structure, const vector<连通块地雷分布>& mine_distrube
					, unsigned long long& seed, long double znereq, int cls, int algo_itr, int zini_itr = 10) {
	  高ZNE版面结果 result;
	  result.dist.probability = vector<vector<long double>>(state.rows + 1, vector<long double>(state.cols + 1, 0.0));
	  auto callback = [&](const 地雷排布& dist) {
		 result.count++;
		 for (int i = 1; i <= state.rows; ++i)
			for (int j = 1; j <= state.cols; ++j)
			   if (dist.dist[i][j]) result.dist.probability[i][j] += 1.0; // 计数
	  };
	  highZNE_inside(callback, state, basic, structure, mine_distrube, seed, znereq, cls, algo_itr, zini_itr);
	  for(int i=1;i<=state.rows;++i)
		 for (int j = 1; j <= state.cols; ++j)
			result.dist.probability[i][j] /= result.count; // 归一化概率
	  return result;
   }
};