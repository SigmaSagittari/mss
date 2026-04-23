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
   public:
   struct player {
	  GameState board;
	  地雷排布 mines;
	  vector<vector<int>> hide_val, priority;
	  vector<vector<bool>> bbv, vis;
	  // hide_val 表示打开这个格子之后，会显示什么数字？（不要打开地雷）
	  // opening_id 表示这个格子的归属地
	  // bbv 表示这个格子是否是油水（空不是 bbbv）
	  // priority 表示格子的优先级
	  // 优先级计算方法：打开之后 chord 能赚多少个油水（或者空），优先级就是多少
	  struct check_info {
		 vector<pair<int, int>> check_priority[10];
		 int maximum = 0;
		 void insert(int x, int y, int p, unsigned long long& seed) {
			if (p >= 0) {
			   check_priority[p].push_back({ x, y });
			   maximum = max(maximum, p);
			   swap(check_priority[p].back(), check_priority[p][seed % check_priority[p].size()]);
			   seed = splitmix64(seed);
			}
		 }
	  } check;
	  int openings;
	  void opening_tile_dfs(int x, int y, int id) {
		 vis[x][y] = true;
		 for_each_adjacent(x, y, board.rows, board.cols, [&](int nx, int ny) {
			if (vis[nx][ny] == false && hide_val[nx][ny] == 0) opening_tile_dfs(nx, ny, id);
		 });
	  }
	  player(const GameState& state, const 地雷排布& mines, unsigned long long& seed)
		 : board(state)
		 , mines(mines)
		 , hide_val(state.rows + 1, vector<int>(state.cols + 1, 0))
		 , priority(state.rows + 1, vector<int>(state.cols + 1, 0))
		 , bbv(state.rows + 1, vector<bool>(state.cols + 1, true))
		 , openings(0) {
		 for (int i = 0; i <= 9; ++i) {
			check.check_priority[i].clear();
			check.check_priority[i].reserve((long long)state.rows * state.cols);
		 }
		 for (int i = 1; i <= state.rows; ++i)
			for (int j = 1; j <= state.cols; ++j)
			   if (mines.dist[i][j] == true) {
				  for_each_adjacent(i, j, state.rows, state.cols, [&](int nx, int ny) {
					 hide_val[nx][ny]++;
				  });
				  bbv[i][j] = false;
			   }
		 vis = vector<vector<bool>>(state.rows + 1, vector<bool>(state.cols + 1, false));
		 for (int i = 1; i <= state.rows; ++i)
			for (int j = 1; j <= state.cols; ++j) {
			   if (mines.dist[i][j] == true) continue;
			   if (hide_val[i][j] == 0 && vis[i][j] == false) {
				  openings++;
				  opening_tile_dfs(i, j, openings);
			   }
			   if (hide_val[i][j] == 0) {
				  bbv[i][j] = false;
				  for_each_adjacent(i, j, state.rows, state.cols, [&](int nx, int ny) {
					 bbv[nx][ny] = false;
				  });
			   }
			}
		 vis = vector<vector<bool>>(state.rows + 1, vector<bool>(state.cols + 1, false));
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
	  void open_dfs(int x, int y) {
		 if (vis[x][y]) return;
		 vis[x][y] = true;
		 if (hide_val[x][y] != 0) {
			if (board.board[x][y] == GameState::Cell::H)
			   board.board[x][y] = static_cast<GameState::Cell>(hide_val[x][y]);
			else
			   priority[x][y]--;
		 }
		 else {
			priority[x][y] = -10000;
			board.board[x][y] = GameState::Cell::N0;
			for_each_adjacent(x, y, board.rows, board.cols, [&](int nx, int ny) {
			   open_dfs(nx, ny);
			});
		 }
	  }
	  void open(int x, int y, unsigned long long& seed) {
		 if (board.board[x][y] != GameState::Cell::H) return;
		 board.board[x][y] = static_cast<GameState::Cell>(hide_val[x][y]);
		 if (hide_val[x][y] == 0) { // 打开了一个空格子
			// 对于已经被打开的空边，优先级会降低，因为打开它们的时候额外给了一个加成，而此时再 chord 空边则不会带空（见下面）
			open_dfs(x, y);
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
		 for (int i = check.maximum; i >= 0; --i) {
			while (!check.check_priority[i].empty()) {
			   pair<int, int> res = check.check_priority[i].back();
			   check.check_priority[i].pop_back();
			   if (priority[res.first][res.second] == i)
				  return res;
			   check.insert(res.first, res.second, priority[res.first][res.second], seed);

			}
			check.maximum = i - 1;
		 }
		 return { -1, -1 };
	  }
   };
   int chaincount(const GameState& state, const 地雷排布& mines, const vector<vector<int>>& hide_val, const vector<vector<bool>>& chorded, pair<int,int> fixedplay = {0,0}) {
	  int R = state.rows, C = state.cols;
	  vector<vector<int>> id(R + 1, vector<int>(C + 1, -1));
	  int nid = 0;
	  for (int i = 1; i <= R; ++i)
		 for (int j = 1; j <= C; ++j)
			if (chorded[i][j]) id[i][j] = nid++;
	  if (nid == 0) return 0;
	  vector<int> parent(nid), rnk(nid, 0);
	  for (int i = 0; i < nid; ++i) parent[i] = i;
	  function<int(int)> find = [&](int x) {
		 return parent[x] == x ? x : parent[x] = find(parent[x]);
	  };
	  auto unite = [&](int a, int b) {
		 int pa = find(a), pb = find(b);
		 if (pa == pb) return;
		 if (rnk[pa] < rnk[pb]) parent[pa] = pb;
		 else if (rnk[pb] < rnk[pa]) parent[pb] = pa;
		 else { parent[pb] = pa; rnk[pa]++; }
	  };
	  for (int i = 1; i <= R; ++i) {
		 for (int j = 1; j <= C; ++j) {
			if (id[i][j] == -1) continue;
			for (int dx = -1; dx <= 1; ++dx) for (int dy = -1; dy <= 1; ++dy) {
			   if (dx == 0 && dy == 0) continue;
			   int ni = i + dx, nj = j + dy;
			   if (ni >= 1 && ni <= R && nj >= 1 && nj <= C && id[ni][nj] != -1) {
				  unite(id[i][j], id[ni][nj]);
			   }
			}
		 }
	  }
	  vector<vector<char>> vis(R + 1, vector<char>(C + 1, 0));
	  auto dfs_zero = [&](int si, int sj) {
		 vector<pair<int, int>> st;
		 st.reserve(64);
		 st.push_back({ si, sj });
		 vis[si][sj] = 1;
		 int fa_id = -1;
		 for (size_t p = 0; p < st.size(); ++p) {
			int x = st[p].first, y = st[p].second;
			// for this zero cell, connect any chorded neighbor to fa_id
			for (int dx = -1; dx <= 1; ++dx) for (int dy = -1; dy <= 1; ++dy) {
			   int nx = x + dx, ny = y + dy;
			   if (nx >= 1 && nx <= R && ny >= 1 && ny <= C) {
				  int cid = id[nx][ny];
				  if (cid != -1) {
					 if (fa_id == -1) {
						fa_id = cid;
					 }
					 else {
						unite(fa_id, cid);
					 }
				  }
			   }
			}
			// expand region to neighboring zero cells
			for (int dx = -1; dx <= 1; ++dx) for (int dy = -1; dy <= 1; ++dy) {
			   if (dx == 0 && dy == 0) continue;
			   int nx = x + dx, ny = y + dy;
			   if (nx >= 1 && nx <= R && ny >= 1 && ny <= C && !vis[nx][ny] && hide_val[nx][ny] == 0 && mines.dist[nx][ny] == false) {
				  vis[nx][ny] = 1;
				  st.push_back({ nx, ny });
			   }
			}
		 }
	  };

	  for (int i = 1; i <= R; ++i) {
		 for (int j = 1; j <= C; ++j) {
			if (hide_val[i][j] == 0 && mines.dist[i][j] == false && !vis[i][j]) dfs_zero(i, j);
		 }
	  }

	  unordered_set<int> roots;
	  roots.reserve(nid);
	  unordered_map<int, bool> valid; // root -> whether this component should be counted
	  // collect cells per component for debugging
	  unordered_map<int, vector<pair<int, int>>> compCells;
	  for (int i = 1; i <= R; ++i) {
		 for (int j = 1; j <= C; ++j) {
			if (id[i][j] != -1) {
			   int r = find(id[i][j]);
			   roots.insert(r);
			   compCells[r].push_back({ i,j });
			}
		 }
	  }

	  int cnt = 0;
	  for (auto& kv : compCells) {
		 int r = kv.first;
		 auto& cells = kv.second;
		 bool is_valid = true;
		 for (auto& p : cells) {
			int i = p.first, j = p.second;
			if (state.board[i][j] != GameState::Cell::H || pair<int,int> {i,j} == fixedplay) {
			   is_valid = false;
			   break; // one is enough
			}
		 }
		 if (is_valid) {
			++cnt;
		 }
	  }
	  return cnt;
   }
   public:
   template<bool fixedplay>
   Zini结果 ChainZini(const GameState& state, const 地雷排布& mines, unsigned long long& seed, int itr = 1, int x = 0, int y = 0) {
	  int global_cls = 2147483647, bbv = 0;
	  for (int i = 1; i <= itr; ++i) {
		 player pl(state, mines, seed);
		 int cls = 0;
		 bool firstmove_open = false;
		 if constexpr (fixedplay)
			if (state.board[x][y] == GameState::Cell::H && pl.hide_val[x][y] != 0) { // 强迫性地做第一步，如果是需要打开的格子那就打开
			   pl.open(x, y, seed);
			   cls++;
			   firstmove_open = true;
			}
		 vector<vector<bool>> chorded(state.rows + 1, vector<bool>(state.cols + 1, false));
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
			chorded[best.first][best.second] = true; // 标记这个格子处理过了
			if (pl.board.board[best.first][best.second] != GameState::Cell::N0) { // 如果打开的非空，就 chord
			   pl.chord(best.first, best.second, seed);
			   cls++;
			}
		 }
		 if(firstmove_open)
			cls += chaincount(state, mines, pl.hide_val, chorded, {x, y});
		 else
			cls += chaincount(state, mines, pl.hide_val, chorded);
		 for (int i = 1; i <= state.rows; ++i)
			for (int j = 1; j <= state.cols; ++j)
			   if (pl.board.board[i][j] == GameState::Cell::H && mines.dist[i][j] == false)
				  cls++;
		 global_cls = min(global_cls, cls);
		 if (i == 1) { // 只计算第一次的 bbbv。
			for (int i = 1; i <= state.rows; ++i)
			   for (int j = 1; j <= state.cols; ++j)
				  if (pl.bbv[i][j]) bbv++;
			bbv += pl.openings;
		 }
	  }
	  return { global_cls, bbv };
   }
   int ZiniDelta(const GameState& state, const 地雷排布& mines, unsigned long long& seed, int x, int y, int itr = 10) {
	  // 这东西很慢，不推荐使用，仅仅作为样例展示应该如何使用 ChainZini 来计算某一步的 zne 增量（如果强迫性地在这个格子上做出正确的操作的话）。
	  return ChainZini<true>(state, mines, seed, itr, x, y).Zini - ChainZini<false>(state, mines, seed, itr).Zini;
   }
};
