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

inline unsigned long long splitmix64(unsigned long long x) {
   x += 0x9e3779b97f4a7c15;
   x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9;
   x = (x ^ (x >> 27)) * 0x94d049bb133111eb;
   return x ^ (x >> 31);
}

// ==================== 数据 ====================
struct GameState {
   enum class Cell : int {
	  N0 = 0, N1 = 1, N2 = 2, N3 = 3, N4 = 4, N5 = 5, N6 = 6, N7 = 7, N8 = 8, H = 9
   };

   vector<vector<Cell>> board;    // 当前盘面
   vector<vector<bool>> flags;     // 旗帜标记
   int rows, cols, total_mines;

   GameState(int r, int c, int m) : rows(r), cols(c), total_mines(m) {
	  board = vector<vector<Cell>>(r + 1, vector<Cell>(c + 1, Cell::H));
	  flags = vector<vector<bool>>(r + 1, vector<bool>(c + 1, false));
   }
};

inline bool isdigit(GameState::Cell c) {
   int val = static_cast<int>(c);
   return 0 <= val && val <= 8;
}

template<typename Func>
void for_each_adjacent(int x, int y, int rows, int cols, Func&& func) {
   for (int i = -1; i <= 1; ++i) {
	  for (int j = -1; j <= 1; ++j) {
		 int nx = x + i, ny = y + j;
		 if ((i != 0 || j != 0) && 1 <= nx && nx <= rows && 1 <= ny && ny <= cols) {
			func(nx, ny);
		 }
	  }
   }
}

// ==================== 推理结果 ====================
struct 基础逻辑结果 {
   enum class Mark { S, M, H, T };  // S=安全, M=危险, H=有信息未开, T=无信息未开

   vector<vector<Mark>> marks;
   int Tsum = 0, Msum = 0;

   基础逻辑结果(int rows, int cols) {
	  marks = vector<vector<Mark>>(rows + 1, vector<Mark>(cols + 1, Mark::S));
	  Tsum = 0, Msum = 0;
   }
};
struct 棋盘结构 {
   struct 连通块 {
	  struct 单位格 {
		 int size;
		 vector<pair<int, int>> position;
	  };
	  struct 限制 {
		 int sum = 0, x = 0, y = 0;
		 vector<int> box_id;
		 bool operator<(const 限制& other) const { if (sum != other.sum) return sum < other.sum; return box_id < other.box_id; }
		 bool operator==(const 限制& other) const { return sum == other.sum && box_id == other.box_id; }
	  };
	  vector<单位格> 单位格们;
	  vector<限制> 限制们;
   };
   vector<连通块> board_struct;
   vector<vector<连通块*>> cell2connect; // 每个格子对应的连通块指针列表
};
struct 连通块地雷分布 {
   struct 分布 {
	  int mine_count; // 地雷数量
	  long double ways; // 摆放方案数
	  vector<long double> expect; // 每个单位格的期望值（雷数）
	  struct 深度结果 {
		 // 用于保存某个连通块所有可能的结果。
		 struct 单个可能 {
			vector<char> assignment; // 每个 box 的雷数分配
			long double ways_perfix; // 该 assignment 的摆放方法数（组合数乘积）的前缀和
		 };
		 vector<单个可能> 摆放方式; // deepres 仅在 void build_probres 的 deep 为 true 时启用，因为会增加额外的计算量。
	  };
	  shared_ptr<深度结果> deepres = nullptr; // 深度结果指针，初始为 nullptr，表示未计算过
   };
   vector<分布> 分布们; // 每个地雷数量对应一个分布
};

struct 高级分析结果 {
   struct 连通块结果 {
	  vector<long double> distrube_probability; // 每一个分布被取到的概率。
   };
   vector<连通块结果> full_result; // 每个连通块对应一个结果
   long double Tcell_probability = 0; // 非前沿是地雷的概率
   long double candidates = 0; // 候选方案数
};

struct 地雷概率 {
   vector<vector<long double>> probability;
};

struct 地雷排布 {
   vector<vector<bool>> dist;
};

struct Zini结果 {
   int Zini, bbbv;
};

struct 高ZNE版面结果 {
   int count = 0; // 满足条件的版面数量
   地雷概率 dist;
};

struct ZNR计算结果 {
   struct 单个格子结果 {
	  int x, y; // 坐标
	  array<bool,8> fl; // 周围八个格子的标雷情况，按照从上到下从左到右的顺序排列。
	  long double probability; // **如果标雷正确**，这个操作是 Zini 的概率。需要结合 ZNE 版面结果进行判定
	  // 例如，一个操作可能拥有 100% 的 probability，但是实际上是因为这个雷排列极其稀有，所以这个操作其实是很差的。
   };
   vector<单个格子结果> ZNR; // 每个"操作"是 Zini 的概率
};