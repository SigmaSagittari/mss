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
// ==================== 算法 ====================
class 基础逻辑分析 {
   public:
   基础逻辑结果 analyze(const GameState& state) {
      基础逻辑结果 result(state.rows, state.cols);
      const auto& board = state.board;
      int n = state.rows, m = state.cols;

      // 1. 初始化：H格子设为T
      for (int i = 1; i <= n; ++i)
         for (int j = 1; j <= m; ++j)
            if (board[i][j] == GameState::Cell::H)
               result.marks[i][j] = 基础逻辑结果::Mark::T;

   // 2. 数字周围的H格子设为H
      for (int i = 1; i <= n; ++i)
         for (int j = 1; j <= m; ++j)
            if (isdigit(board[i][j]))
               for_each_adjacent(i, j, n, m, [&](int nx, int ny) {
               if (board[nx][ny] == GameState::Cell::H)
                  result.marks[nx][ny] = 基础逻辑结果::Mark::H;
            });

// 3. 雷的确定：数字 = 周围H数 → 设为M
      for (int i = 1; i <= n; ++i)
         for (int j = 1; j <= m; ++j)
            if (isdigit(board[i][j])) {
               int Hcnt = 0;
               for_each_adjacent(i, j, n, m, [&](int nx, int ny) {
                  if (board[nx][ny] == GameState::Cell::H) Hcnt++;
               });
               if (Hcnt == static_cast<int>(board[i][j]))
                  for_each_adjacent(i, j, n, m, [&](int nx, int ny) {
                  if (board[nx][ny] == GameState::Cell::H)
                     result.marks[nx][ny] = 基础逻辑结果::Mark::M;
               });
            }

    // 4. 安全的确定：数字 = 周围M数 → 剩余H设为S
      for (int i = 1; i <= n; ++i)
         for (int j = 1; j <= m; ++j)
            if (isdigit(board[i][j])) {
               int Mcnt = 0;
               for_each_adjacent(i, j, n, m, [&](int nx, int ny) {
                  if (result.marks[nx][ny] == 基础逻辑结果::Mark::M) Mcnt++;
               });
               if (Mcnt == static_cast<int>(board[i][j]))
                  for_each_adjacent(i, j, n, m, [&](int nx, int ny) {
                  if (board[nx][ny] == GameState::Cell::H &&
                      result.marks[nx][ny] != 基础逻辑结果::Mark::M) {
                     result.marks[nx][ny] = 基础逻辑结果::Mark::S;
                  }
               });
            }

    // 5. 统计
      result.Tsum = result.Msum = 0;
      for (int i = 1; i <= n; ++i)
         for (int j = 1; j <= m; ++j) {
            if (result.marks[i][j] == 基础逻辑结果::Mark::M) result.Msum++;
            if (result.marks[i][j] == 基础逻辑结果::Mark::T) result.Tsum++;
         }

      return result;
   }
};