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
   void highZNE_inside(const callback& cb, const GameState& state, const 基础逻辑结果& basic, const 棋盘结构& structure, const vector<连通块地雷分布>& mine_distrube, const 高级分析结果& advanced
					   , unsigned long long& seed, long double znereq, int cls, int algo_itr, int zini_itr = 10) {
			 // znereq 表示至少期待的 zne 的要求，cls 表示已经进行的点击次数，itr 表示计算 zini 时的迭代次数，迭代次数越高越准确但越慢。
	  if (advanced.candidates <= algo_itr) { // 当候选方案足够少，直接遍历
		 auto callback_wrapper = [&](const 地雷排布& dist) {
			Zini结果 zinires = ZiniAlgo().ChainZini<false>(state, dist, seed, 1);
			if ((long double)zinires.bbbv / (zinires.Zini + cls) < znereq * 0.8) return; // 如果单次迭代的 zne 过低，说明这个随机分布不具有代表性，直接丢弃。 0.8 随便写的，平时基本 0.9 都不会出岔子，但是为了保险起见，降低一点要求。
			zinires = ZiniAlgo().ChainZini<false>(state, dist, seed, zini_itr); // 计算这个分布的 zne，迭代次数较高以获得更准确的结果。
			if ((long double)zinires.bbbv / (zinires.Zini + cls) < znereq)  return;
			//cerr << (zinires.bbbv + 0.0) / (zinires.Zini + cls) * 100 << "%" << endl;
			cb(dist, zinires);
		 };
		 概率分析().all_distrubte(state, basic, structure, mine_distrube, callback_wrapper);
		 return;
	  }
	  for (int i = 1; i <= algo_itr; ++i) {
		 unsigned long long seed_used = seed;
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
	  for (int i = 1; i <= state.rows; ++i)
		 for (int j = 1; j <= state.cols; ++j) {
			if (i == 1 || i == state.rows || j == 1 || j == state.cols) {
			   playable[i][j] = true; // 谁玩效的时候不喜欢香香软软的边界格子呢？
			}
			if (state.board[i][j] != GameState::Cell::H) {
			   for (int dx = -3; dx <= 3; ++dx)
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
			for_each_adjacent(i, j, state.rows, state.cols, [&](int nx, int ny) {
			   if (state.board[nx][ny] == GameState::Cell::H && dist.dist[nx][ny] == false) good = true;
			});
			if (good == false) playable[i][j] = false; // 周围一个能开的格子都没有你玩你妈呢？
		 }
	  return playable;
   }
   public:
   ZNR计算结果 get_ZNR(const GameState& state, const 基础逻辑结果& basic, const 棋盘结构& structure, const vector<连通块地雷分布>& mine_distrube, const 高级分析结果& advanced
				   , unsigned long long& seed, long double znereq, int cls, bool 加权 = false, int algo_itr = 10000, int zini_itr = 20) {
	  ZNR计算结果 result;
	  map<ZNR计算结果::操作, pair<int, long double>> operation_prob; // 统计每个操作的概率，pair<int,longdouble> 表示 {计数, 概率}
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
				  t.first++;
				  if (加权) t.second += pow(2, ((long double)Zinires.bbbv / (Zinires.Zini + cls)) / 0.05); // 计数和累计 zne 增益
				  else t.second += zinires.Zini - Zinires.Zini;
			   }
			}
	  };
	  highZNE_inside(callback, state, basic, structure, mine_distrube, advanced, seed, znereq, cls, algo_itr, zini_itr);
	  for (auto& kv : operation_prob)
		 if (kv.second.first != 0) {
			if (加权) kv.second.second = log(kv.second.second) - log(pow(2, znereq / 0.05)); // 计算得分
			else kv.second.second /= kv.second.first; // 计算平均 ZNE 概率
			result.ZNR.push_back({ kv.first, kv.second.second });
		 }
	  for (int i = 1; i <= state.rows; ++i)
		 for (int j = 1; j <= state.cols; ++j)
			result.ZNE_result.dist.probability[i][j] /= result.ZNE_result.count; // 归一化概率
	  result.ZNE_result.total = (int)round(min((long double)algo_itr, advanced.candidates));
	  return result;
   }
   ZNR计算结果 get_ZNR_new(const GameState& state, const 基础逻辑结果& basic, const 棋盘结构& structure, const vector<连通块地雷分布>& mine_distrube, const 高级分析结果& advanced
					   , unsigned long long& seed, long double znereq, int cls, function<long double(long double)> 加权, int algo_itr = 10000, int zini_itr = 20, int 多线搜索 = 5) {

	  vector<vector<bool>> playable(state.rows + 1, vector<bool>(state.cols + 1, false));

	  for (int i = 1; i <= state.rows; ++i)
		 for (int j = 1; j <= state.cols; ++j) {
			if (i == 1 || i == state.rows || j == 1 || j == state.cols) {
			   playable[i][j] = true; // 谁玩效的时候不喜欢香香软软的边界格子呢？
			}
			if (state.board[i][j] != GameState::Cell::H) {
			   for (int dx = -3; dx <= 3; ++dx)
				  for (int dy = -3; dy <= 3; ++dy) {
					 int nx = i + dx, ny = j + dy;
					 if (1 <= nx && nx <= state.cols && 1 <= ny && ny <= state.rows)
						playable[nx][ny] = true; // 已经打开的格子周围的格子也可以玩效，毕竟有信息了。
				  }
			}
		 }
	  for(int i=1;i<=state.rows;++i)
		 for (int j = 1; j <= state.cols; ++j) {
			bool good = false;
			for_each_adjacent(i, j, state.rows, state.cols, [&](int nx, int ny) {
			   if (state.board[nx][ny] == GameState::Cell::H) good = true;
			});
			if (!good) playable[i][j] = false; // 别在家里挂机了！！！
		 }

	  ZNR计算结果 result;
	  vector<pair<地雷排布, Zini结果>> all_results; // 存储所有分布和对应的 zini 结果
	  auto callback = [&](const 地雷排布& dist, const Zini结果& zinires) {
		 all_results.push_back({ dist, zinires });
	  };
	  highZNE_inside(callback, state, basic, structure, mine_distrube, advanced, seed, znereq, cls, algo_itr, zini_itr);
	  cerr << "生成盘面结束……共 " << all_results.size() << " 个盘面满足要求。" << endl;
	  stable_sort(all_results.begin(), all_results.end(), [&](const auto& a, const auto& b) {return (a.second.bbbv * 1.0) / (a.second.Zini + cls) > (b.second.bbbv * 1.0) / (b.second.Zini + cls); });

	  vector<long double> weight_suffix(all_results.size() + 1, 0.0);

	  vector<pair<ZNR计算结果::操作, long double>> best_results(多线搜索);

	  for (int i = (int)all_results.size()-1; i >=0; --i)
		 weight_suffix[i] = weight_suffix[i + 1] + 加权((long double)all_results[i].second.bbbv / (all_results[i].second.Zini + cls));

	  for (int i = 1; i <= state.rows; ++i) {
		 for (int j = 1; j <= state.cols; ++j) {
			if (!playable[i][j]) continue; // 不可玩效的格子直接跳过
			map<array<array<bool, 3>, 3>, long double> operation_weight_sum; // 用于计算每个操作的权重总和。
			long double overall_weight = 0.0;
			for (int k = 0; k < (int)all_results.size(); ++k) {

			   if (all_results[k].first.dist[i][j]) continue; // 这个分布这个格子是雷，不需要考虑。
			   
			   array<array<bool, 3>, 3> fl = {};
			   if (state.board[i][j] != GameState::Cell::H) { // 已经打开的格子，周围八个格子的标雷情况
				  for (int dx = -1; dx <= 1; ++dx)
					 for (int dy = -1; dy <= 1; ++dy) {
						int nx = dx + i, ny = dy + j;
						if (1 <= nx && nx <= state.rows && 1 <= ny && ny <= state.cols)
						   fl[dx + 1][dy + 1] = all_results[k].first.dist[nx][ny];
						else
						   fl[dx + 1][dy + 1] = false; // 边界外视为安全
					 }
			   } // 没打开的格子，只要单击就行，不需要考虑标雷

			   if (operation_weight_sum[fl] + weight_suffix[k] < best_results[0].second) {
				  if(operation_weight_sum[fl]!=-1e100)
				  // 这个分布的权重加上后续分布的最大权重都无法超过当前最优解，剪枝。
				  operation_weight_sum[fl] = -1e100;
				  continue;
			   }
			   auto Zinires = ZiniAlgo().ChainZini<true>(state, all_results[k].first, seed, zini_itr, i, j);
			   operation_weight_sum[fl] += 加权((long double)Zinires.bbbv / (Zinires.Zini + cls)); // 累加权重
			}
			for (const auto& [key, weight] : operation_weight_sum) { // 检查每个操作
			   if (weight <= best_results[0].second && best_results.size() == 多线搜索)
				  continue;
			   best_results.push_back({ {i, j, key}, weight });
			   sort(best_results.begin(), best_results.end(),
					[](const auto& a, const auto& b) { return a.second < b.second; });
			   if(best_results.size() > 多线搜索)
				  best_results.erase(best_results.begin()); // 保持 best_results 的大小不超过 多线搜索
			}
		 }
	  }

	  result.ZNE_result.count = (int)all_results.size();
	  result.ZNE_result.total = (int)round(min((long double)algo_itr, advanced.candidates));
	  result.ZNE_result.dist = {vector<vector<long double>>(state.rows + 1, vector<long double>(state.cols + 1, 0.0))};
	  for (int i = 1; i <= state.rows; ++i)
		 for (int j = 1; j <= state.cols; ++j) {
			for (const auto& res : all_results)
			   if (res.first.dist[i][j]) result.ZNE_result.dist.probability[i][j] += 1.0; // 计数
			result.ZNE_result.dist.probability[i][j] /= result.ZNE_result.count; // 归一化概率
		 }
	  for (const auto& res : best_results) {
		 result.ZNR.push_back({ res.first, log(res.second)-znereq * 13.8629436112 }); // 这个数字有点抽象，随便写的，为了把结果限制在一个合理范围。
	  }
	  return result;
   }

   static inline long double 指数加权(long double x) { // 没有特殊理由就用这个
	  return pow(2, x / 0.05);
   }
   static inline long double 线性加权(long double x) {
	  return x;
   }
   static inline long double 无加权(long double x) {
	  return 1.0;
   }
};

