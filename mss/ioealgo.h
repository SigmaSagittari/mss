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
#include <map>

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
   void highZNE_inside(const callback&cb, const GameState& state, const 基础逻辑结果& basic, const 棋盘结构& structure, const vector<连通块地雷分布>& mine_distrube, const 高级分析结果& advanced
				, unsigned long long& seed, long double znereq, int cls, int algo_itr, int zini_itr = 10) {
	  // znereq 表示至少期待的 zne 的要求，cls 表示已经进行的点击次数，itr 表示计算 zini 时的迭代次数，迭代次数越高越准确但越慢。
	  if (advanced.candidates <= algo_itr) { // 当候选方案足够少，直接遍历
		 auto callback_wrapper = [&](const 地雷排布& dist) {
			Zini结果 zinires = ZiniAlgo().ChainZini<false>(state, dist, seed, 1);
			if ((long double)zinires.bbbv / (zinires.Zini + cls) < znereq * 0.8) return; // 如果单次迭代的 zne 过低，说明这个随机分布不具有代表性，直接丢弃。 0.8 随便写的，平时基本 0.9 都不会出岔子，但是为了保险起见，降低一点要求。
			zinires = ZiniAlgo().ChainZini<false>(state, dist, seed, zini_itr); // 计算这个分布的 zne，迭代次数较高以获得更准确的结果。
			if ((long double)zinires.bbbv / (zinires.Zini + cls) < znereq)  return;
			cb(dist, zinires);
		 };
		 概率分析().all_distrubte(state, basic, structure, mine_distrube, callback_wrapper);
		 // 如果是遍历所有分布的话，不可以根据 ZNR 来丢弃某个候选操作，因为有偏，大部分雷有可能会聚在一起导致出现问题，所以如果枚举的话，一定要全算一遍。
		 // 你都能遍历了，就没有必要吝啬那点时间分析操作了吧，直接全算一遍就好了。
		 return;
	  }
	  for (int i = 1; i <= algo_itr; ++i) {
		 地雷排布 dist = 概率分析().gen_random(state, basic, structure, mine_distrube, seed); // 生成一个随机分布，作为 zini 的输入。
		 Zini结果 zinires = ZiniAlgo().ChainZini<false>(state, dist, seed, 1);
		 if ((long double)zinires.bbbv / (zinires.Zini + cls) < znereq * 0.8) continue; // 如果单次迭代的 zne 过低，说明这个随机分布不具有代表性，直接丢弃。 0.8 随便写的，平时基本 0.9 都不会出岔子，但是为了保险起见，降低一点要求。
		 zinires = ZiniAlgo().ChainZini<false>(state, dist, seed, zini_itr); // 计算这个分布的 zne，迭代次数较高以获得更准确的结果。
		 if ((long double)zinires.bbbv / (zinires.Zini + cls) < znereq)  continue; 
		 cb(dist, zinires);
	  }
   }
   vector<vector<bool>> potential_playable(const GameState& state, const 地雷排布& dist) {
	  vector<vector<bool>> playable(state.rows + 1, vector<bool>(state.cols + 1, false));
	  for(int i=1;i<=state.rows;++i)
		 for (int j = 1; j <= state.cols; ++j) {
			if (i == 1 || i == state.rows || j == 1 || j == state.cols) {
			   playable[i][j] = true; // 谁玩效的时候不喜欢香香软软的边界格子呢？
			}
			if (state.board[i][j] != GameState::Cell::H) {
			   for(int dx=-3;dx<=3;++dx)
				  for (int dy = -3; dy <= 3; ++dy) {
					 int nx = i + dx, ny = j + dy;
					 if (1 <= nx && nx <= state.cols && 1 <= ny && ny <= state.rows)
						playable[nx][ny] = true; // 已经打开的格子周围的格子也可以玩效，毕竟有信息了。
				  }
			}
		 }
	  for (int i = 1; i <= state.rows; ++i)
		 for (int j = 1; j <= state.cols; ++j) {
			if (dist.dist[i][j]) playable[i][j] = false; // 你居然是地雷！！！！我看错你了，没想到你居然是这样的格子。
			bool good = false;
			for_each_adjacent(i,j,state.rows,state.cols,[&](int nx,int ny){
			   if (state.board[nx][ny] == GameState::Cell::H && dist.dist[nx][ny] == false) good = true;
			});
			if (good == false) playable[i][j] = false; // 周围一个能开的格子都没有你玩你妈呢？
		 }
	  return playable;
   }
   public:
   高ZNE版面结果 get_highZNE(const GameState& state, const 基础逻辑结果& basic, const 棋盘结构& structure, const vector<连通块地雷分布>& mine_distrube, const 高级分析结果& advanced
					, unsigned long long& seed, long double znereq, int cls, int algo_itr, int zini_itr = 20) {
	  高ZNE版面结果 result;
	  result.dist.probability = vector<vector<long double>>(state.rows + 1, vector<long double>(state.cols + 1, 0.0));
	  auto callback = [&](const 地雷排布& dist, const Zini结果& zinires) {
		 result.count++;
		 for (int i = 1; i <= state.rows; ++i)
			for (int j = 1; j <= state.cols; ++j)
			   if (dist.dist[i][j]) result.dist.probability[i][j] += 1.0; // 计数
	  };
	  highZNE_inside(callback, state, basic, structure, mine_distrube, advanced, seed, znereq, cls, algo_itr, zini_itr);
	  for(int i=1;i<=state.rows;++i)
		 for (int j = 1; j <= state.cols; ++j)
			result.dist.probability[i][j] /= result.count; // 归一化概率
	  result.total = (int) round(min((long double)algo_itr, advanced.candidates));
	  return result;
   }
   ZNR计算结果 get_ZNR(const GameState& state, const 基础逻辑结果& basic, const 棋盘结构& structure, const vector<连通块地雷分布>& mine_distrube, const 高级分析结果& advanced
				   , unsigned long long& seed, long double znereq, int cls, bool 加权 = false, int algo_itr = 10000, int zini_itr = 20) {
	  ZNR计算结果 result;
	  map<ZNR计算结果::操作, pair<int,long double>> operation_prob; // 统计每个操作的概率，pair<int,longdouble> 表示 {计数, 概率}
	  result.ZNE_result.dist.probability = vector<vector<long double>>(state.rows + 1, vector<long double>(state.cols + 1, 0.0));
	  auto callback = [&](const 地雷排布& dist, const Zini结果& zinires) {
		 result.ZNE_result.count++;
		 vector<vector<bool>> playable = potential_playable(state, dist);
		 for (int i = 1; i <= state.rows; ++i)
			for (int j = 1; j <= state.cols; ++j)
			   if (dist.dist[i][j]) result.ZNE_result.dist.probability[i][j] += 1.0; // 计数
		 for (int i = 1; i <= state.rows; ++i)
			for (int j = 1; j <= state.cols; ++j) {
			   if (playable[i][j]) {
				  auto Zinires = ZiniAlgo().ChainZini<true>(state, dist, seed, zini_itr, i, j);
				  array<array<bool, 3>, 3> fl = {};
				  if (state.board[i][j] != GameState::Cell::H) { // 已经打开的格子，周围八个格子的标雷情况
					 for (int dx = -1; dx <= 1; ++dx)
						for (int dy = -1; dy <= 1; ++dy) {
						   int nx = dx + i, ny = dy + j;
						   if (1 <= nx && nx <= state.rows && 1 <= ny && ny <= state.cols)
							  fl[dx + 1][dy + 1] = dist.dist[nx][ny];
						   else
							  fl[dx + 1][dy + 1] = false; // 边界外视为安全
						}
				  } // 没打开的格子，只要单击就行，不需要考虑标雷
				  auto& t = operation_prob[{i, j, fl}];
				  t.first ++;
				  if (加权) t.second += pow(2, ((long double)Zinires.bbbv / (Zinires.Zini + cls)) / 0.05); // 计数和累计 zne 增益
				  else t.second += zinires.Zini - Zinires.Zini;
			   }
			}
	  };
	  highZNE_inside(callback, state, basic, structure, mine_distrube, advanced, seed, znereq, cls, algo_itr, zini_itr);
	  for (auto& kv : operation_prob)
		 if (kv.second.first != 0) {
			if(加权) kv.second.second = log(kv.second.second) - log(pow(2, znereq / 0.05)); // 计算得分
			else kv.second.second /= kv.second.first; // 计算平均 ZNE 概率
			result.ZNR.push_back({ kv.first, kv.second.second, kv.second.first });
		 }
	  for (int i = 1; i <= state.rows; ++i)
		 for (int j = 1; j <= state.cols; ++j)
			result.ZNE_result.dist.probability[i][j] /= result.ZNE_result.count; // 归一化概率
	  result.ZNE_result.total = (int) round(min((long double)algo_itr, advanced.candidates));
	  return result;
   }
};