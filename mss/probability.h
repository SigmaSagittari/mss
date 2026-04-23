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
class 概率分析 {
   private:
   struct 随机生成结果 {
	  struct 连通块结果 {
		 int mine_count; // 这个连通块的地雷数量
	  };
	  vector<连通块结果> full_result; // 每个连通块对应一个结果
	  int Tcell_minecount = 0; // 非前沿是地雷的数量
   };
   inline static vector<long double> log_fact; // 阶乘对数
   static void combi_init(int n) {
	  if (log_fact.empty()) log_fact.push_back(0.0);
	  while ((int)log_fact.size() <= n + 1)
		 log_fact.push_back(log_fact.end()[-1] + log(log_fact.size()));
   }
   static long double binom(int n, int k) {
	  if (k<0 || k>n) return 0;
	  if (k == 0 || k == n) return 1;
	  combi_init(n);
	  return exp(log_fact[n] - log_fact[k] - log_fact[n - k]);
   }
   struct Polynomial { // 多项式类，用于计算概率生成函数
	  int start; // e^start 表示多项式的最低次幂，coeffs[i] 表示 x^(start+i) 的系数。
	  vector<long double> coeffs;
	  Polynomial operator*(const Polynomial& other) const {
		 // 多项式乘法
		 int start = this->start + other.start, size = (int)coeffs.size() + (int)other.coeffs.size() - 1;
		 vector<long double> res(size, 0.0);
		 for (int i = 0; i < (int)coeffs.size(); ++i)
			for (int j = 0; j < (int)other.coeffs.size(); ++j)
			   res[i + j] += coeffs[i] * other.coeffs[j];
		 return { start,res };
	  }
	  Polynomial operator/(const Polynomial& other) const {
		 // 多项式除法，返回商多项式。
		 int start = this->start - other.start, size = max(0, (int)coeffs.size() - (int)other.coeffs.size() + 1);
		 vector<long double> res(size, 0.0);
		 vector<long double> rem(coeffs);
		 long double other_leading = other.coeffs.back();
		 for (int i = (int)rem.size() - 1; i >= (int)other.coeffs.size() - 1; --i) {
			if (abs(rem[i]) < 1e-12) continue; // 如果当前余数的最高次幂系数接近于 0，则跳过
			long double factor = rem[i] / other_leading;
			int quot_idx = i - ((int)other.coeffs.size() - 1);
			res[quot_idx] = factor;
			for (int j = 0; j < (int)other.coeffs.size(); ++j) {
			   rem[i - j] -= factor * other.coeffs[other.coeffs.size() - 1 - j];
			}
		 }
		 return { start,res };
	  }
	  Polynomial(int s, const vector<long double>& c) : start(s), coeffs(c) {}
	  Polynomial() :start(0), coeffs({}) {}
	  Polynomial(const 连通块地雷分布& s) {
		 if (s.分布们.empty()) {
			*this = Polynomial(0, { 1.0 });
			return;
		 }
		 int min_exp = s.分布们[0].mine_count, max_exp = min_exp;
		 for (连通块地雷分布::分布 i : s.分布们) {
			min_exp = min(min_exp, i.mine_count);
			max_exp = max(max_exp, i.mine_count);
		 }
		 vector<long double> coeffs(max_exp - min_exp + 1, 0.0);
		 for (连通块地雷分布::分布 i : s.分布们)
			coeffs[i.mine_count - min_exp] = i.ways;
		 *this = Polynomial(min_exp, coeffs);
	  }
   };
   Polynomial full_gf_poly(const vector<连通块地雷分布>& connect_distributions) {
	  // 计算所有连通块的联合概率生成函数
	  if (connect_distributions.empty()) return Polynomial(0, { 1.0 });
	  Polynomial result(connect_distributions[0]);
	  for (int i = 1; i < (int)connect_distributions.size(); ++i)
		 result = result * Polynomial(connect_distributions[i]);
	  return result;
   }
   long double denominator(const Polynomial& gf, int total_mines, int Tsum) {
	  // 计算候选方案数
	  long double result = 0.0;
	  for (int i = 0; i < (int)gf.coeffs.size(); ++i) {
		 int heavy_mines = gf.start + i, light_mines = total_mines - heavy_mines;
		 if (light_mines >= 0 && light_mines <= Tsum)
			result += gf.coeffs[i] * binom(Tsum, light_mines);
	  }
	  return result;
   }
   高级分析结果 analysis(const vector<连通块地雷分布>& connect_distributions, int total_mines, int Tsum) {
	  // 根据每个连通块的地雷分布，以及未被基础逻辑发现的的地雷数量和无信息格子数量，计算出每个连通块的最终结果（每个分布被取到的概率）和非前沿格子的雷概率。
	  // 这个是给我内部用的，除非你知道你在干什么，否则请不要调用这个函数。
	  高级分析结果 result;
	  Polynomial P_H = full_gf_poly(connect_distributions);
	  long double Denominator = denominator(P_H, total_mines, Tsum);
	  long double light_prob = 0.0;
	  for (int i = 0; i < (int)P_H.coeffs.size(); ++i) {
		 int heavy_mines = P_H.start + i;
		 int light_mines = total_mines - 1 - heavy_mines;
		 if (light_mines >= 0 && light_mines <= Tsum - 1)
			light_prob += P_H.coeffs[i] * binom(Tsum - 1, light_mines);
	  }
	  light_prob /= Denominator;
	  for (int i = 0; i < (int)connect_distributions.size(); ++i) {
		 Polynomial P_i(connect_distributions[i]);
		 Polynomial T_i = P_H / P_i;
		 result.full_result.push_back({});
		 for (const 连通块地雷分布::分布& value : connect_distributions[i].分布们) {
			int v = value.mine_count;
			long double w = value.ways, numerator = 0;
			for (int j = 0; j < (int)T_i.coeffs.size(); ++j) {
			   int t_mines = T_i.start + j, light_mines = total_mines - v - t_mines;
			   if (light_mines >= 0 && light_mines <= Tsum)
				  numerator += w * T_i.coeffs[j] * binom(Tsum, light_mines);
			}
			long double prob = numerator / Denominator;
			result.full_result.back().distrube_probability.push_back(prob);
		 }
	  }
	  result.candidates = Denominator;
	  result.Tcell_probability = light_prob;
	  return result;
   }
   随机生成结果 randomAnalysis(const vector<连通块地雷分布>& connect_distributions, int total_mines, int Tsum, unsigned long long& seed) {
	  // 均匀，随机地生成一个符合条件的分布，要求分布要有深度结果。
	  随机生成结果 result;

	  Polynomial P_H = full_gf_poly(connect_distributions);
	  for (int i = 0; i < (int)connect_distributions.size(); ++i) {
		 if (!connect_distributions[i].分布们.empty())
			if (connect_distributions[i].分布们[0].deepres == nullptr) {
			   cerr << "调用 randomAlalysis 需要连通块分布的深度结果，但当前分布没有，请先使用 analysis 函数计算出深度结果。" << endl;
			   assert(false);
			}
		 long double denominator = 0.0;
		 for (int i = 0; i < (int)P_H.coeffs.size(); ++i) {
			int heavy_mines = P_H.start + i, light_mines = total_mines - heavy_mines;
			if (light_mines >= 0 && light_mines <= Tsum) {
			   denominator += P_H.coeffs[i] * binom(Tsum, light_mines);
			}
		 }
		 P_H = P_H / Polynomial(connect_distributions[i]);
		 seed = splitmix64(seed);
		 long double r = (seed & 0xFFFFFFFFFFFFF) * 1.0 / (1LL << 52);
		 bool flag = false;
		 for (const 连通块地雷分布::分布& value : connect_distributions[i].分布们) {
			int v = value.mine_count;
			long double w = value.ways, numerator = 0;
			for (int j = 0; j < (int)P_H.coeffs.size(); ++j) {
			   int t_mines = P_H.start + j, light_mines = total_mines - v - t_mines;
			   if (light_mines >= 0 && light_mines <= Tsum)
				  numerator += w * P_H.coeffs[j] * binom(Tsum, light_mines);
			}
			long double prob = numerator / denominator;
			r -= prob;
			if (r < 1e-7) {
			   total_mines -= v;
			   result.full_result.push_back({ v });
			   flag = true;
			   break;
			}
		 }
		 if (flag == false) {
			cerr << "在尝试生成局面时发生意外错误，程序终止。" << endl;
			assert(false);
		 }
	  }
	  result.Tcell_minecount = total_mines;
	  return result;
   }
   template<typename Callback>
   void all_distrubte_inside(const vector<连通块地雷分布>& connect_distributions, int total_mines, int Tsum, Callback callback) {
	  // 产生所有分布，注意只是分布，而不是具体的局面。想要获得具体的局面还需要根据深度结果进行进一步的生成。
	  vector<随机生成结果::连通块结果> current_counts;
	  current_counts.reserve(connect_distributions.size());
	  auto dfs = [&](int block_index, int remaining_mines, auto&& dfs_ref) -> void {
		 if (block_index == (int)connect_distributions.size()) {
			if (remaining_mines >= 0 && remaining_mines <= Tsum) {
			   随机生成结果 result;
			   result.full_result = current_counts;
			   result.Tcell_minecount = remaining_mines;
			   callback(result);
			}
			return;
		 }

		 const auto& block = connect_distributions[block_index];

		 for (const 连通块地雷分布::分布& v : block.分布们) {
			if (v.mine_count <= remaining_mines) {
			   current_counts.push_back(随机生成结果::连通块结果{ v.mine_count });
			   dfs_ref(block_index + 1, remaining_mines - v.mine_count, dfs_ref);
			   current_counts.pop_back();
			}
		 }
	  };

	  dfs(0, total_mines, dfs);
   }
   public:
   高级分析结果 analysis(const GameState& state, const 基础逻辑结果& basic, const 棋盘结构& structure, const vector<连通块地雷分布>& mine_distrube) {
	  // 给出每个分布的概率结果的接口，这个接口是公用的。
	  int mines = state.total_mines, Tsum = 0;
	  for (int i = 1; i <= state.rows; ++i)
		 for (int j = 1; j <= state.cols; ++j) {
			if (basic.marks[i][j] == 基础逻辑结果::Mark::M)
			   mines--;
			if (basic.marks[i][j] == 基础逻辑结果::Mark::T)
			   Tsum++;
		 }
	  return analysis(mine_distrube, mines, Tsum);
   }
   地雷概率 transfer(const GameState& state, const 基础逻辑结果& basic, const 棋盘结构& structure, const vector<连通块地雷分布>& mine_distrube, const 高级分析结果& advancedresult) {
	  地雷概率 result;
	  result.probability = vector<vector<long double>>(state.rows + 1, vector<long double>(state.cols + 1, 0.0));
	  for (int i = 1; i <= state.rows; ++i)
		 for (int j = 1; j <= state.cols; ++j) {
			if (basic.marks[i][j] == 基础逻辑结果::Mark::S) continue;
			if (basic.marks[i][j] == 基础逻辑结果::Mark::M) result.probability[i][j] = 1.0;
			if (basic.marks[i][j] == 基础逻辑结果::Mark::T) result.probability[i][j] = advancedresult.Tcell_probability;
		 }
	  for (int i = 0; i < (int)advancedresult.full_result.size(); ++i) {
		 for (int j = 0; j < (int)advancedresult.full_result[i].distrube_probability.size(); ++j) {
			long double prob = advancedresult.full_result[i].distrube_probability[j];
			const auto& distribution = mine_distrube[i].分布们[j];
			for (int k = 0; k < (int)distribution.expect.size(); ++k) {
			   long double expect = distribution.expect[k];
			   for (pair<int, int> pos : structure.board_struct[i].单位格们[k].position) {
				  result.probability[pos.first][pos.second] += expect * prob / structure.board_struct[i].单位格们[k].size;
			   }
			}
		 }
	  }
	  return result;
   }
   地雷排布 gen_random(const GameState& state, const 基础逻辑结果& basic, const 棋盘结构& structure, const vector<连通块地雷分布>& mine_distrube, unsigned long long& seed) {
	  // 均匀随机生成一个符合当前棋盘状态的雷分布，并返回每个格子是雷的概率（0或1）。
	  unsigned long long local_seed = seed;
	  地雷排布 result;
	  result.dist = vector<vector<bool>>(state.rows + 1, vector<bool>(state.cols + 1, false));
	  for (int i = 1; i <= state.rows; ++i)
		 for (int j = 1; j <= state.cols; ++j) {
			if (basic.marks[i][j] == 基础逻辑结果::Mark::S) continue;
			if (basic.marks[i][j] == 基础逻辑结果::Mark::M) result.dist[i][j] = true;
		 }
	  int mines = state.total_mines, Tsum = 0;
	  for (int i = 1; i <= state.rows; ++i)
		 for (int j = 1; j <= state.cols; ++j) {
			if (basic.marks[i][j] == 基础逻辑结果::Mark::M)
			   mines--;
			if (basic.marks[i][j] == 基础逻辑结果::Mark::T)
			   Tsum++;
		 }
	  随机生成结果 res = randomAnalysis(mine_distrube, mines, Tsum, seed);
	  for (int i = 0; i < (int)structure.board_struct.size(); ++i) {
		 int choosed_idx = -1;
		 for (int idx = 0; idx < (int)(mine_distrube[i].分布们.size()); ++idx)
			if (mine_distrube[i].分布们[idx].mine_count == res.full_result[i].mine_count) {
			   choosed_idx = idx;
			   break;
			}
		 if (choosed_idx == -1) {
			cerr << "发生未知错误，程序终止。" << endl;
			cerr << "[LOG] " << endl;
			cerr << "SEED : " << local_seed << endl;
			cerr << "局面:" << endl;
			for (int x = 1; x <= state.rows; ++x) {
			   for (int y = 1; y <= state.cols; ++y) {
				  if (state.flags[x][y]) cerr << 'F';
				  else if (state.board[x][y] == GameState::Cell::H) cerr << 'H';
				  else cerr << static_cast<int> (state.board[x][y]);

			   }
			   cerr << endl;
			}
			cerr << "分布：" << i << endl;
			cerr << "期望地雷数: " << res.full_result[i].mine_count << "但是没有找到" << endl;
			assert(false);
		 }
		 const 连通块地雷分布::分布& chosen = mine_distrube[i].分布们[choosed_idx];
		 if (chosen.deepres == nullptr) {
			cerr << "调用 gen_random 需要连通块分布的深度结果，请先使用 analysis 函数计算出深度结果。" << endl;
			assert(false);
		 }
		 long double totalWays = chosen.ways;
		 seed = splitmix64(seed);
		 long double rnd = (seed & 0xFFFFFFFFFFFFF) * 1.0L / (1ULL << 52) * totalWays;
		 size_t pick = (size_t)(lower_bound(chosen.deepres->摆放方式.begin(), chosen.deepres->摆放方式.end(), rnd,
								[](const 连通块地雷分布::分布::深度结果::单个可能& elem, long double value) {return elem.ways_perfix < value; })
								- chosen.deepres->摆放方式.begin());
		 if (pick >= chosen.deepres->摆放方式.size()) pick = chosen.deepres->摆放方式.size() - 1;
		 const vector<char>& assignment = chosen.deepres->摆放方式[pick].assignment; // 每个元素为对应 box 的地雷数
		 for (int b = 0; b < (int)structure.board_struct[i].单位格们.size(); ++b) { // 终于定位到，每个单元格里应该放多少个，然后根据随机结果放。
			int k = static_cast<int>(assignment[b]);
			if (k <= 0) continue;
			const vector<pair<int, int>>& positions = structure.board_struct[i].单位格们[b].position;
			int n = (int)positions.size();
			for (int i = 0; i < n && k > 0; ++i) {
			   seed = splitmix64(seed);
			   if ((seed % (n - i)) < static_cast<unsigned long long>(k)) {
				   // 接受当前格子
				  result.dist[positions[i].first][positions[i].second] = true;
				  k--;
			   }
			}
		 }
	  }
	  if (res.Tcell_minecount > 0) {
		 int k = res.Tcell_minecount;
		 int remaining_T = Tsum;  // 剩余的T格子总数

		 // 遍历所有格子，遇到T格子时以概率k/remaining_T决定是否选择
		 for (int i = 1; i <= state.rows && k > 0; ++i) {
			for (int j = 1; j <= state.cols && k > 0; ++j) {
			   if (basic.marks[i][j] == 基础逻辑结果::Mark::T) {
				   // 当前T格子，以k/remaining_T的概率选择
				  seed = splitmix64(seed);
				  if ((seed % remaining_T) < static_cast<unsigned long long>(k)) {
					  // 选择这个T格子
					 result.dist[i][j] = true;
					 k--;
				  }
				  remaining_T--;  // 无论是否选择，剩余T格子数减少
			   }
			}
		 }
	  }
	  return result;
   }
   void all_distrubte(const GameState& state, const 基础逻辑结果& basic, const 棋盘结构& structure, const vector<连通块地雷分布>& connect_distributions, void (*callback)(const 地雷概率&)) {
	  // 此函数会枚举所有可能的局面，并对每个局面调用一次 callback。会指数爆炸而且我没有判断输入是否合法，所以用的时候记得判一下，免得整个程序卡死。
	  // Determine remaining mines and Tsum (non-frontier hidden cells)
	  int mines = state.total_mines;
	  int Tsum = 0;
	  for (int i = 1; i <= state.rows; ++i) {
		 for (int j = 1; j <= state.cols; ++j) {
			if (basic.marks[i][j] == 基础逻辑结果::Mark::M) mines--;
			if (basic.marks[i][j] == 基础逻辑结果::Mark::T) Tsum++;
		 }
	  }

	  // collect list of T cell positions for later enumeration
	  vector<pair<int, int>> Tcells;
	  for (int i = 1; i <= state.rows; ++i)
		 for (int j = 1; j <= state.cols; ++j)
			if (basic.marks[i][j] == 基础逻辑结果::Mark::T)
			   Tcells.emplace_back(i, j);

	  // helper to enumerate combinations of k positions out of given positions and mark them in prob
	  auto enum_choose_positions = [&](auto&& self, const vector<pair<int, int>>& positions, int start, int need, 地雷概率& prob, auto&& on_complete) -> void {
		 if (need == 0) {
			on_complete();
			return;
		 }
		 int n = (int)positions.size();
		 for (int i = start; i <= n - need; ++i) {
			auto p = positions[i];
			prob.probability[p.first][p.second] = 1.0L;
			self(self, positions, i + 1, need - 1, prob, on_complete);
			prob.probability[p.first][p.second] = 0.0L;
		 }
	  };

	  // main callback from distribution-level enumeration: r contains per-block mine counts and Tcell_minecount
	  auto tmp_function = [&](const 随机生成结果& rse) {
		 // For each block, we need to iterate all deep assignments and then iterate per-box inner placements.
		 int blocks = (int)connect_distributions.size();

		 // For each block, prepare pointer to chosen 分布
		 vector<const 连通块地雷分布::分布*> chosen_dist(blocks, nullptr);
		 for (int i = 0; i < blocks; ++i) {
			int want = rse.full_result[i].mine_count;
			const auto& block = connect_distributions[i];
			int choose_idx = -1;
			for (int j = 0; j < (int)block.分布们.size(); ++j) {
			   if (block.分布们[j].mine_count == want) { choose_idx = j; break; }
			}
			if (choose_idx == -1) return; // inconsistent, skip
			chosen_dist[i] = &block.分布们[choose_idx];
			if (chosen_dist[i]->deepres == nullptr) return; // deep results required
		 }

		 // recursive over blocks
		 地雷概率 prob_template;
		 prob_template.probability = vector<vector<long double>>(state.rows + 1, vector<long double>(state.cols + 1, 0.0L));
		 // mark basic M as 1.0 and S as 0.0 (optional); we'll leave revealed numbers as 0
		 for (int i = 1; i <= state.rows; ++i)
			for (int j = 1; j <= state.cols; ++j)
			   if (basic.marks[i][j] == 基础逻辑结果::Mark::M) prob_template.probability[i][j] = 1.0L;

		 // recursive over blocks and their deep assignments
		 function<void(int, 地雷概率&)> dfs_block = [&](int bi, 地雷概率& cur_prob) {
			if (bi == blocks) {
			   // handle T cells selection: choose rse.Tcell_minecount among Tcells
			   int k = rse.Tcell_minecount;
			   if (k == 0) {
				  callback(cur_prob);
				  return;
			   }
			   // enumerate combinations of Tcells
			   enum_choose_positions(enum_choose_positions, Tcells, 0, k, cur_prob, [&]() { callback(cur_prob); });
			   return;
			}

			const auto& deep = chosen_dist[bi]->deepres->摆放方式;
			// for each possible assignment (unit box counts) in this block
			for (const auto& poss : deep) {
			   const vector<char>& assignment = poss.assignment;
			   // enumerate placements inside boxes of this block
			   地雷概率 copy_prob = cur_prob; // copy current state
			   const auto& boxes = structure.board_struct[bi].单位格们;

			   function<void(int)> dfs_box = [&](int boxi) {
				  if (boxi == (int)boxes.size()) {
					 // finished this block, go to next
					 dfs_block(bi + 1, copy_prob);
					 return;
				  }
				  int need = static_cast<int>(assignment[boxi]);
				  const auto& positions = boxes[boxi].position;
				  if (need == 0) {
					 dfs_box(boxi + 1);
					 return;
				  }
				  // choose `need` positions out of positions
				  enum_choose_positions(enum_choose_positions, positions, 0, need, copy_prob, [&]() { dfs_box(boxi + 1); });
			   };

			   dfs_box(0);
			}
		 };

		 地雷概率 start_prob = prob_template;
		 dfs_block(0, start_prob);
	  };

	  // call distribution enumerator with computed mines and Tsum
	  all_distrubte_inside(connect_distributions, mines, Tsum, tmp_function);
   }
};