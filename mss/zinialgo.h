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

class ZiniAlgo {
   private:
   struct zero_tile_information {
	  // 保存空格子的信息，避免反复 dfs 带来的性能损失。
	  vector<pair<int, int>> tiles; // 空格子（和临空位）的列表
	  vector<pair<int, int>> opening_interval; // oi[x] 表示 x 号 opening 对应 tiles[oi[x].first ~ oi[x].second] 这段区间的格子（含两端）
	  vector<vector<int>> opening_id; // opening_id[i][j] 表示这个格子属于哪个 opening，0 表示不属于任何 opening，空边不会被赋值，为了防止一个空边同时属于多个空，所以确保访问空的中心。
	  int openings = 0 , tilestop = 0; // 空的数量

	  // 这个类暂时不启用，会在下一个更新被用到。
   };
   struct player {
	  const 地雷排布& mines;
	  thread_local inline static GameState board;
	  thread_local inline static vector<vector<int>> hide_val, priority;
	  thread_local inline static vector<vector<bool>> bbv, vis;
	  thread_local inline static zero_tile_information zt_info;
	  // hide_val 表示打开这个格子之后，会显示什么数字？（不要打开地雷）
	  // opening_id 表示这个格子的归属地
	  // bbv 表示这个格子是否是油水（空不是 bbbv）
	  // priority 表示格子的优先级
	  // 优先级计算方法：打开之后 chord 能赚多少个油水（或者空），优先级就是多少
	  struct check_info {
		 vector<pair<int, int>> check_priority[10];
		 int maximum = 0, stack_top[10] = {};
		 void insert(int x, int y, int p, unsigned long long& seed) {
			if (p >= 0) {
			   check_priority[p][stack_top[p]] = { x, y };
			   maximum = max(maximum, p);
			   swap(check_priority[p][stack_top[p]], check_priority[p][seed % (stack_top[p] + 1)]);
			   seed = splitmix64(seed);
			   stack_top[p]++;
			}
		 }
		 pair<int, int> pop_best(unsigned long long& seed) {
			for (int i = maximum; i >= 0; --i) {
			   while (stack_top[i] > 0) {
				  pair<int, int> res = check_priority[i][--stack_top[i]];
				  if (priority[res.first][res.second] == i)
					 return res;
				  insert(res.first, res.second, priority[res.first][res.second], seed);

			   }
			   maximum = i - 1;
			}
			return { -1, -1 };
		 }
	  };
	  thread_local inline static check_info check;
	  void opening_tile_dfs(int start_x, int start_y, zero_tile_information& zt_info) { // 对 zero_tile_info 的初始化，虽然这里有些在 player 初始化没有用到，但是在 chainZini 会用到。
		 zt_info.openings++;
		 int itr = zt_info.openings == 1 ? 0 : zt_info.opening_interval[zt_info.openings - 2].second + 1;
		 // 新 opening 的起始位置，如果是第一个 opening 就从 0 开始，否则从上一个 opening 的结束位置的下一个位置开始

		 vis[start_x][start_y] = true;
		 zt_info.tiles[zt_info.tilestop] = { start_x, start_y };
		 zt_info.tilestop++;
		 zt_info.opening_id[start_x][start_y] = zt_info.openings - 1;

		 while (itr < zt_info.tilestop) {
			auto [x, y] = zt_info.tiles[itr];
			itr++;
			if (hide_val[x][y] == 0) { // 只对 0 进行扩张，空的边缘的会加入 opening 但是不进行扩张。
			   zt_info.opening_id[x][y] = zt_info.openings - 1;
			   for_each_adjacent(x, y, board.rows, board.cols, [&](int nx, int ny) {
				  if (!vis[nx][ny]) {
					 vis[nx][ny] = true;
					 zt_info.tiles[zt_info.tilestop] = { nx, ny };
					 zt_info.tilestop++;
				  }
			   });
			}
		 }

		 zt_info.opening_interval[zt_info.openings - 1] = {zt_info.openings == 1 ? 0 : zt_info.opening_interval[zt_info.openings - 2].second + 1 , zt_info.tilestop - 1};
		 for(int i= zt_info.opening_interval[zt_info.openings - 1].first; i<= zt_info.opening_interval[zt_info.openings - 1].second;++i)
			vis[zt_info.tiles[i].first][zt_info.tiles[i].second] = false;
	  }
	  player(const GameState& state, const 地雷排布& mines, unsigned long long& seed):
		 mines(mines) {
		 check.maximum = 0;
		 zt_info.openings = 0;
		 zt_info.tilestop = 0;
		 if (hide_val.size() != state.rows + 1 || hide_val[0].size() != state.cols + 1) {
			hide_val = vector<vector<int>>(state.rows + 1, vector<int>(state.cols + 1, 0));
			priority = vector<vector<int>>(state.rows + 1, vector<int>(state.cols + 1, 0));
			bbv = vector<vector<bool>>(state.rows + 1, vector<bool>(state.cols + 1, true));
			vis = vector<vector<bool>>(state.rows + 1, vector<bool>(state.cols + 1, false));
			zt_info.opening_id = vector<vector<int>>(state.rows + 1, vector<int>(state.cols + 1, -1));
			zt_info.opening_interval = vector<pair<int, int>>(state.rows * state.cols + 1, pair<int, int>{0, 0});
			zt_info.tiles = vector<pair<int, int>>(state.rows * state.cols * 3 + 1, pair<int, int>{0, 0});
			for (int i = 0; i <= 9; ++i) {
			   check.check_priority[i] = vector<pair<int, int>>(state.rows * state.cols * 8, pair<int, int>{0, 0});
			   check.stack_top[i] = 0;
			}
			board = state;
		 }
		 else {
			board.total_mines = state.total_mines;
			for (int i = 1; i <= state.rows; ++i)
			   for (int j = 1; j <= state.cols; ++j) {
				  hide_val[i][j] = 0;
				  priority[i][j] = 0;
				  bbv[i][j] = true;
				  vis[i][j] = false;
				  zt_info.opening_id[i][j] = -1;
				  board.board[i][j] = state.board[i][j];
				  board.flags[i][j] = state.flags[i][j];
			   }
			for (int i = 0; i <= state.rows * state.cols; ++i)
			   zt_info.opening_interval[i] = pair<int, int>{ 0,0 };
			for(int i=0;i<=state.rows*state.cols*3;++i)
			   zt_info.tiles[i] = pair<int, int>{ 0,0 };
			for (int i = 0; i <= 9; ++i)
			   check.stack_top[i] = 0;
		 }
		 for (int i = 1; i <= state.rows; ++i)
			for (int j = 1; j <= state.cols; ++j)
			   if (mines.dist[i][j] == true) {
				  for_each_adjacent(i, j, state.rows, state.cols, [&](int nx, int ny) {
					 hide_val[nx][ny]++;
				  });
				  bbv[i][j] = false;
			   }
		 for (int i = 1; i <= state.rows; ++i)	
			for (int j = 1; j <= state.cols; ++j) {
			   if (mines.dist[i][j] == true) continue;
			   if (hide_val[i][j] == 0 && zt_info.opening_id[i][j] == -1) {
				  opening_tile_dfs(i, j, zt_info);
			   }
			   if (hide_val[i][j] == 0) {
				  bbv[i][j] = false;
				  for_each_adjacent(i, j, state.rows, state.cols, [&](int nx, int ny) {
					 bbv[nx][ny] = false;
				  });
			   }
			}
		 for (int i = 1; i <= state.rows; ++i)
			for (int j = 1; j <= state.cols; ++j)
			   assert(vis[i][j] == false);
		 for (int i = 1; i <= state.rows; ++i)
			for (int j = 1; j <= state.cols; ++j) {
			   if (mines.dist[i][j] == true)
				  priority[i][j] = -10000;
			   else
				  if (hide_val[i][j] == 0) {
					 priority[i][j] = board.board[i][j] == GameState::Cell::H ? 0 : -10000;
					 continue;
				  }
			   priority[i][j]--;
			   if (mines.dist[i][j] == true && state.flags[i][j] == false) {
				  priority[i][j] = -10000;
				  for_each_adjacent(i, j, state.rows, state.cols, [&](int nx, int ny) {
					 priority[nx][ny]--;
				  });
			   }
			   if (bbv[i][j] && state.board[i][j] == GameState::Cell::H) {
				  for_each_adjacent(i, j, state.rows, state.cols, [&](int nx, int ny) {
					 priority[nx][ny]++;
				  });
			   }
			}
		 for (int i = 1; i <= state.rows; ++i)
			for (int j = 1; j <= state.cols; ++j)
			   check.insert(i, j, priority[i][j], seed);
	  }
	  void open_dfs_new(int start_x, int start_y, const zero_tile_information& zt_info) {
		 assert(hide_val[start_x][start_y] == 0);


		 auto [l, r] = zt_info.opening_interval[zt_info.opening_id[start_x][start_y]];

		 for (int i = l; i <= r; i++) {
			auto [x, y] = zt_info.tiles[i];
			if (hide_val[x][y] == 0) {
			   priority[x][y] = -10000;
			   board.board[x][y] = GameState::Cell::N0;
			}
			else {
			   if (board.board[x][y] == GameState::Cell::H) {
				  board.board[x][y] = static_cast<GameState::Cell>(hide_val[x][y]);
			   }
			   else {
				  priority[x][y]--;
			   }
			}

			vis[x][y] = true;
		 }
	  }
	  void open(int x, int y, unsigned long long& seed) {
		 if (board.board[x][y] != GameState::Cell::H) return;
		 board.board[x][y] = static_cast<GameState::Cell>(hide_val[x][y]);
		 if (hide_val[x][y] == 0) { // 打开了一个空格子
			// 对于已经被打开的空边，优先级会降低，因为打开它们的时候额外给了一个加成，而此时再 chord 空边则不会带空（见下面）
			open_dfs_new(x, y, zt_info);
		 }	
		 else {
			if (bbv[x][y]) { // 打开了一个油水格子
			   for_each_adjacent(x, y, board.rows, board.cols, [&](int nx, int ny) {
				  priority[nx][ny]--;
			   });
			}
			else {
			   priority[x][y]++; // 打开了一个空边
			   check.insert(x, y, priority[x][y], seed);
			}
		 }
	  }
	  void flag(int x, int y, unsigned long long& seed) {
		 // Fl 会使得周围的格子优先级变高，因为免费标记了一个地雷
		 board.flags[x][y] = true;
		 for_each_adjacent(x, y, board.rows, board.cols, [&](int nx, int ny) {
			priority[nx][ny]++;
			check.insert(nx, ny, priority[nx][ny], seed);
		 });
	  }
	  void chord(int x, int y, unsigned long long& seed) {
		 // 需要确保这个格子已经被打开了，并且周围的旗帜数量等于这个格子显示的数字
		 for_each_adjacent(x, y, board.rows, board.cols, [&](int nx, int ny) {
			if (mines.dist[nx][ny] == false)
			   open(nx, ny, seed);
		 });
		 priority[x][y] = -10000; // 人不能两次 chord 同一个格子
	  }
	  pair<int, int> pop_best(unsigned long long& seed) {
		 return check.pop_best(seed);
	  }
   };
   int chaincount_new(const GameState& state, const vector<pair<int,int>>& chorded ,const zero_tile_information& zt_info, pair<int,int> fixedplay={0,0}) {
	  thread_local static vector<vector<pair<int,int>>> fa;
	  if (fa.empty() || (int)fa.size() != state.rows+1 || (int)fa[0].size() != state.cols+1) {
		 fa = vector<vector<pair<int,int>>>(state.rows+1, vector<pair<int,int>>(state.cols+1, {-1, -1}));
	  }
	  for (int i = 1; i <= state.rows; ++i)
		 for (int j = 1; j <= state.cols; ++j)
			assert(fa[i][j] == make_pair(-1, -1));
	  function<pair<int,int>(pair<int,int>)> getfa = [&](pair<int,int> x) {
		 return fa[x.first][x.second] == x ? x : fa[x.first][x.second] = getfa(fa[x.first][x.second]);
	  };
	  auto uni = [&](pair<int,int> a, pair<int,int> b) {
		 a= getfa(a);
		 b= getfa(b);
		 fa[a.first][a.second] = b;
	  };
	  for (pair<int, int> i : chorded)
		 fa[i.first][i.second] = i; // 初始化并查集，每个 chorded 的格子自成一个集合
	  for (int i = 0; i < zt_info.openings; ++i) {
		 auto [l, r] = zt_info.opening_interval[i];
		 pair<int, int> main_tile = { -1,-1 };
		 for (int j = l; j <= r; ++j) {
			auto [x, y] = zt_info.tiles[j];
			if (fa[x][y] != pair<int, int>(-1, -1)) { // 不是 -1 说明是 chord。 
			   if (main_tile == pair<int, int> {-1, -1})
				  main_tile = getfa(fa[x][y]);
			   else
				  uni(main_tile, getfa(fa[x][y]));
			}
		 }
	  }
	  for (const pair<int, int> &i : chorded) {
		 for_each_adjacent(i.first, i.second, state.rows, state.cols, [&](int nx, int ny) {
			if (fa[nx][ny] != pair<int, int>(-1, -1))
			   uni(getfa(i), getfa(fa[nx][ny]));
		 });
	  }
	  unordered_map<long long, int> result_map;
	  for (const pair<int, int>& i : chorded) {
		 pair<int, int> root = getfa(i);
		 long long root_key = root.first * (state.cols + 1LL) + root.second; // 将 root 转换为一个 long long 以便用作 unordered_map 的 key
		 if (i == fixedplay || state.board[i.first][i.second] != GameState::Cell::H)
			result_map[root_key] = -1; // 这个连通块永远暴毙
		 else if(result_map[root_key]!=-1)
			result_map[root_key] = 1; // 初始化为 1，表示这个连通块还可以存活
	  }
	  for (const pair<int, int>& i : chorded) {
		 fa[i.first][i.second] = {-1,-1}; // 重置并查集，准备下一次使用
	  }
	  int cnt = 0;
	  for (auto i : result_map) {
		 if (i.second == 1) cnt++;
	  }
	  return cnt;
   }
   public:
   template<bool fixedplay>
   Zini结果 ChainZini(const GameState& state, const 地雷排布& mines, unsigned long long& seed, int itr = 1, int x = 0, int y = 0) {
	  int global_cls = 2147483647, bbv = 0;
	  thread_local static vector<pair<int, int>> chord_new;
	  chord_new.reserve(state.cols * state.rows + 1);
	  for (int i = 1; i <= itr; ++i) {
		 player pl(state, mines, seed);
		 int cls = 0;
		 chord_new.clear();
		 bool firstmove_open = false;
		 if constexpr (fixedplay)
			if (state.board[x][y] == GameState::Cell::H && pl.hide_val[x][y] != 0) { // 强迫性地做第一步，如果是需要打开的格子那就打开
			   pl.open(x, y, seed);
			   cls++;
			   firstmove_open = true;
			}
		 for (int loop = 1;; ++loop) {
			pair<int, int> best = pl.pop_best(seed);
			if constexpr (fixedplay)
			   if (state.board[x][y] != GameState::Cell::H && loop == 1) { // 强迫第一步，如果不需要打开，说明应该被 chord
				  best = pair<int, int>{ x, y }; // 狸猫换太子，强迫性地 chord 这个格子
			   }
			if (best == pair<int, int> {-1, -1}) break;
			if (pl.board.board[best.first][best.second] == GameState::Cell::H) { // 没打开就打开
			   pl.open(best.first, best.second, seed);
			   // 这里原本是需要修改 cls 的，但是接下来有一些复杂的图论处理，最后再根据连通块数量来计算 cls。
			}
			for_each_adjacent(best.first, best.second, pl.board.rows, pl.board.cols, [&](int nx, int ny) { // 标一圈旗
			   if (mines.dist[nx][ny] == true && pl.board.flags[nx][ny] == false) {
				  pl.flag(nx, ny, seed);
				  cls++;
			   }
			});
			chord_new.push_back(best);
			if (pl.board.board[best.first][best.second] != GameState::Cell::N0) { // 如果打开的非空，就 chord
			   pl.chord(best.first, best.second, seed);
			   cls++;
			}
		 }
		 if (firstmove_open) {
			int new_val = chaincount_new(state, chord_new, pl.zt_info, { x,y });
			cls += new_val;
		 }
		 else {
			int new_val = chaincount_new(state, chord_new, pl.zt_info);
			cls += new_val;
		 }
		 for (int i = 1; i <= state.rows; ++i)
			for (int j = 1; j <= state.cols; ++j)
			   if (pl.board.board[i][j] == GameState::Cell::H && mines.dist[i][j] == false)
				  cls++;
		 global_cls = min(global_cls, cls);
		 if (i == 1) { // 只计算第一次的 bbbv。
			for (int i = 1; i <= state.rows; ++i)
			   for (int j = 1; j <= state.cols; ++j)
				  if (pl.bbv[i][j]) bbv++;
			bbv += pl.zt_info.openings;
		 }
	  }
	  return { global_cls, bbv };
   }
   int ZiniDelta(const GameState& state, const 地雷排布& mines, unsigned long long& seed, int x, int y, int itr = 20) {
	  // 这东西很慢，不推荐使用，仅仅作为样例展示应该如何使用 ChainZini 来计算某一步的 zne 增量（如果强迫性地在这个格子上做出正确的操作的话）。
	  return ChainZini<true>(state, mines, seed, itr, x, y).Zini - ChainZini<false>(state, mines, seed, itr).Zini;
   }
}; 