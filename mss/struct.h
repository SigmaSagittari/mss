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
#include "core.h"
#include "basic.h"



class 连通块构造 {
   private:
   const GameState* state = nullptr;
   const 基础逻辑结果* basicresult = nullptr;
   vector<vector<bool>> vis;
   vector<vector<unsigned long long>> cell_hash; // 每个格子的哈希值，用于区分是否处在同一单位格
   vector<pair<int, int>> cell_list(int x, int y) { // 以一个 "H" 开始，搜索当前连通块，返回当前连通块的所有格子列表
      vector<pair<int, int>> result;
      auto dfs = [&](auto&& self, int cx, int cy) -> void {
         if (vis[cx][cy]) return;
         vis[cx][cy] = true;
         result.push_back({ cx, cy });

         if (isdigit(state->board[cx][cy])) {
            for_each_adjacent(cx, cy, state->rows, state->cols, [&](int nx, int ny) {
               if (basicresult->marks[nx][ny] == 基础逻辑结果::Mark::H)
                  self(self, nx, ny);
            });
         }

         if (basicresult->marks[cx][cy] == 基础逻辑结果::Mark::H) {
            for_each_adjacent(cx, cy, state->rows, state->cols, [&](int nx, int ny) {
               if (isdigit(state->board[nx][ny]))
                  self(self, nx, ny);
            });
         }
      };
      dfs(dfs, x, y);
      return result;
   }
   棋盘结构::连通块 build_connect(vector<pair<int, int>>& cell_list) { // 根据一个连通块的格子列表，构造出这个连通块的结构
      棋盘结构::连通块 result;

      vector<unsigned long long> hash_list;
      for (pair<int, int> i : cell_list)
         hash_list.push_back(cell_hash[i.first][i.second]);
      sort(hash_list.begin(), hash_list.end());
      hash_list.resize(unique(hash_list.begin(), hash_list.end()) - hash_list.begin());

      vector<int> hash_used(hash_list.size(), -1);

      for (pair<int, int> i : cell_list)
         if (basicresult->marks[i.first][i.second] == 基础逻辑结果::Mark::H) {
            unsigned long long hash_value = lower_bound(hash_list.begin(), hash_list.end(), cell_hash[i.first][i.second]) - hash_list.begin();
            if (hash_used[hash_value] == -1) {
               hash_used[hash_value] = (int)result.单位格们.size();
               result.单位格们.push_back({ 1,{{i.first,i.second}} });
            }
            else {
               result.单位格们[hash_used[hash_value]].size++;
               result.单位格们[hash_used[hash_value]].position.push_back({ i.first, i.second });
            }
            cell_hash[i.first][i.second] = hash_used[hash_value]; // 复用变量 cell_hash 用于保存 pos -> 单位格id 的映射
         }

      vector<bool> box_used(result.单位格们.size(), false);

      for (pair<int, int> i : cell_list)
         if (isdigit(state->board[i.first][i.second])) {
            result.限制们.push_back({ static_cast<int>(state->board[i.first][i.second]) ,i.first,i.second,{} });
            for_each_adjacent(i.first, i.second, state->rows, state->cols, [&](int nx, int ny) {
               if (basicresult->marks[nx][ny] == 基础逻辑结果::Mark::M)
                  result.限制们.back().sum--;
               if (basicresult->marks[nx][ny] == 基础逻辑结果::Mark::H) {
                  int box_id = (int)cell_hash[nx][ny];
                  if (box_used[box_id] == false) {
                     box_used[box_id] = true;
                     result.限制们.back().box_id.push_back(box_id);
                  }
               }
            });
            for (int j : result.限制们.back().box_id)
               box_used[j] = false;
         }
      return result;
   }
   public:
   棋盘结构 brute_build_struct(const GameState& State, const 基础逻辑结果& Basicresult) {
      // 根据当前棋盘状态和基础逻辑分析结果，构造出棋盘结构
      // 其实基础逻辑结果是不必要的，但是可以提高效率。
      vis = vector<vector<bool>>(State.rows + 1, vector<bool>(State.cols + 1, false));
      cell_hash = vector<vector<unsigned long long>>(State.rows + 1, vector<unsigned long long>(State.cols + 1, 0ull));
      state = &State;
      basicresult = &Basicresult;
      棋盘结构 result;
      result.cell2connect = vector<vector<棋盘结构::连通块*>>(State.rows + 1, vector<棋盘结构::连通块*>(State.cols + 1, nullptr));
      result.board_struct.clear();

      for (int i = 1; i <= state->rows; ++i)
         for (int j = 1; j <= state->cols; ++j)
            if (isdigit(state->board[i][j])) {
               unsigned long long seed = splitmix64(i * (state->cols + state->rows + 3) + j);
               for_each_adjacent(i, j, state->rows, state->cols, [&](int nx, int ny) {
                  cell_hash[nx][ny] += seed;
               });
            }
      for (int i = 1; i <= state->rows; ++i)
         for (int j = 1; j <= state->cols; ++j)
            if (basicresult->marks[i][j] == 基础逻辑结果::Mark::H && !vis[i][j]) {
               auto cells = cell_list(i, j);
               result.board_struct.push_back(build_connect(cells));
               for (auto i : cells)
                  result.cell2connect[i.first][i.second] = reinterpret_cast<棋盘结构::连通块*>(result.board_struct.size());
               // 这里看着存的是指针，但是实际上存的是下标，防止 vector 重新分配内存导致指针失效，后续会统一转换为真正的指针
            }
      for (int i = 1; i <= state->rows; ++i)
         for (int j = 1; j <= state->cols; ++j)
            if (result.cell2connect[i][j] != nullptr)
               result.cell2connect[i][j] = &result.board_struct.begin()[reinterpret_cast<size_t>(result.cell2connect[i][j]) - 1];
                  // 将 cell2connect 中的索引转换为指针
      return result;
   }
}; 