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
#include "core.h"          // 数据结构
#include "basic.h"         // 基础逻辑分析算法（降低核心算法压力）
#include "struct.h"        // 建立棋盘的图论结构（连通块等）
class 连通块地雷分布计算 {
   private:

   inline static unordered_map<unsigned long long, 连通块地雷分布> connect_map;
   // 计算一个连通块的哈希值，用于在 connect_map 中进行查找
   unsigned long long gen_hash(const 棋盘结构::连通块& Connect) {
      unsigned long long h = 0x19260817;
      // 哈希 box_list
      for (const auto& b : Connect.单位格们) {
         h ^= splitmix64(b.size);
         h = splitmix64(h);
      }
      // 哈希 limit_list
      for (const auto& lim : Connect.限制们) {
         h ^= splitmix64(lim.sum);
         for (int id : lim.box_id) {
            h ^= splitmix64((unsigned long long)id + 0x9e3779b9);
            h = splitmix64(h);
         }
         h = splitmix64(h);
      }
      return h;
   }
   public:
   连通块地雷分布 analysis(const 棋盘结构::连通块& Connect, bool deep) {
      连通块地雷分布 result;

      // 组合数表
      constexpr int MAXC = 9;
      constexpr array<array<long double, MAXC + 1>, MAXC + 1> C = []() {
         array<array<long double, MAXC + 1>, MAXC + 1> arr{};
         for (int i = 0; i <= MAXC; ++i) {
            arr[i][0] = arr[i][i] = 1;
            for (int j = 1; j < i; ++j)
               arr[i][j] = arr[i - 1][j - 1] + arr[i - 1][j];
         }
         return arr;
      }();

      // 缓存查找
      unsigned long long hsh = gen_hash(Connect);
      if (connect_map.count(hsh) && !deep) {
         return connect_map[hsh];
      }

      int n = (int)Connect.单位格们.size();
      int max_total = 0;
      for (auto& cell : Connect.单位格们) max_total += cell.size;

      // 结果表
      vector<long double> waystable(max_total + 5, 0);
      vector<vector<long double>> expecttable(max_total + 5, vector<long double>(n, 0));
      vector<连通块地雷分布::分布::深度结果> deeprestable;
      if (deep) deeprestable.resize(max_total + 5);

      // 预处理：每个 box 被哪些限制依赖
      vector<vector<int>> box_to_limits(n);
      for (int i = 0; i < (int)Connect.限制们.size(); ++i) {
         int max_box = 0;
         for (int box_id : Connect.限制们[i].box_id) max_box = max(max_box, box_id);
         box_to_limits[max_box].push_back(i);
      }

      // DFS
      vector<char> assignment(n);

      function<void(int, long double)> dfs = [&](int idx, long double cur_ways) {
         if (idx == n) {
            int total = 0;
            for (int i = 0; i < n; ++i) total += assignment[i];

            waystable[total] += cur_ways;
            for (int i = 0; i < n; ++i)
               expecttable[total][i] += cur_ways * assignment[i];

            if (deep) {
               vector<char> asgn(assignment.begin(), assignment.begin() + n);
               deeprestable[total].摆放方式.push_back({ asgn, cur_ways });
            }
            return;
         }

         int max_k = Connect.单位格们[idx].size;
         for (int k = 0; k <= max_k; ++k) {
            assignment[idx] = k;
            long double new_ways = cur_ways * C[max_k][k];

            // 检查涉及当前 box 的限制
            bool ok = true;
            for (int limit_id : box_to_limits[idx]) {
               int s = 0;
               for (int box_id : Connect.限制们[limit_id].box_id)
                  s += assignment[box_id];
               if (s != Connect.限制们[limit_id].sum) {
                  ok = false;
                  break;
               }
            }
            if (ok) dfs(idx + 1, new_ways);
         }
      };

      dfs(0, 1.0);

      // 构建结果
      for (int total = 0; total <= max_total; ++total) {
         if (waystable[total] == 0) continue;

         result.分布们.push_back({});
         auto& dist = result.分布们.back();
         dist.mine_count = total;
         dist.ways = waystable[total];
         dist.expect.swap(expecttable[total]);

         if (deep) {
            auto* ptr = new 连通块地雷分布::分布::深度结果;
            ptr->摆放方式.swap(deeprestable[total].摆放方式);
            // 计算前缀和
            for (int i = 1; i < (int)ptr->摆放方式.size(); ++i)
               ptr->摆放方式[i].ways_perfix += ptr->摆放方式[i - 1].ways_perfix;
            dist.deepres = shared_ptr<连通块地雷分布::分布::深度结果>(ptr);
         }
         else {
            dist.deepres = nullptr;
         }

         for (int i = 0; i < n; ++i)
            dist.expect[i] /= waystable[total];
      }

      if (!deep) connect_map[hsh] = result;
      return result;
   }
   vector<连通块地雷分布> analysis(const 棋盘结构& Structure, bool deep) {
      // 懒人式一键调用，如果没有很独特的理由的话，就用这个吧！
      vector<连通块地雷分布> result;
      for (const auto& block : Structure.board_struct)
         result.push_back(analysis(block, deep));
      return result;
   }
};