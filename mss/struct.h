#pragma once

class 连通块构造 {
   private:
   void cell_list(int x, int y, const GameState& state, const 基础逻辑结果& basicresult, vector2D<char>& vis, vector<pair<int,int>>& cell) {
      // 以一个 "H" 开始，搜索当前连通块，返回当前连通块的所有格子列表，结果放入 cell 中
      auto dfs = [&](auto&& self, int cx, int cy) -> void {
         if (vis[cx][cy]) return;
         vis[cx][cy] = true;
         cell.push_back({ cx, cy });

         if (isdigit(state.board[cx][cy])) {
            for_each_adjacent(cx, cy, state.rows, state.cols, [&](int nx, int ny) {
               if (basicresult.marks[nx][ny] == 基础逻辑结果::Mark::H)
                  self(self, nx, ny);
            });
         }

         if (basicresult.marks[cx][cy] == 基础逻辑结果::Mark::H) {
            for_each_adjacent(cx, cy, state.rows, state.cols, [&](int nx, int ny) {
               if (isdigit(state.board[nx][ny]))
                  self(self, nx, ny);
            });
         }
      };
      dfs(dfs, x, y);
   }
   棋盘结构::连通块 build_connect(vector<pair<int, int>>& cell_list, const GameState& state, const 基础逻辑结果& basicresult, vector2D<unsigned long long>& cell_hash) { // 根据一个连通块的格子列表，构造出这个连通块的结构
      棋盘结构::连通块 result;

      vector<unsigned long long> hash_list;
      for (pair<int, int> i : cell_list)
         hash_list.push_back(cell_hash[i.first][i.second]);
      sort(hash_list.begin(), hash_list.end());
      hash_list.resize(unique(hash_list.begin(), hash_list.end()) - hash_list.begin());

      vector<int> hash_used(hash_list.size(), -1);

      for (pair<int, int> i : cell_list)
         if (basicresult.marks[i.first][i.second] == 基础逻辑结果::Mark::H) {
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
         if (isdigit(state.board[i.first][i.second])) {
            result.限制们.push_back({ static_cast<int>(state.board[i.first][i.second]) ,i.first,i.second,{} });
            for_each_adjacent(i.first, i.second, state.rows, state.cols, [&](int nx, int ny) {
               if (basicresult.marks[nx][ny] == 基础逻辑结果::Mark::M)
                  result.限制们.back().sum--;
               if (basicresult.marks[nx][ny] == 基础逻辑结果::Mark::H) {
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
      static thread_local vector2D<char> vis;
      static thread_local vector2D<unsigned long long> cell_hash; // 每个格子的哈希值，用于区分是否处在同一单位格
      static thread_local vector<pair<int, int>> cell;

      if (vis.rows != State.rows+1 || vis.cols != State.cols+1) {
         vis.resize(State.rows+1, State.cols+1, false);
         cell_hash.resize(State.rows + 1, State.cols + 1, 0);
         cell.reserve(State.rows*State.cols/2);
      }
      else {
         vis.fill_all(false);
         cell_hash.fill_all(0);
      }
      棋盘结构 result;
      result.cell2connect.resize(State.rows+1,State.cols+1,nullptr);
      result.board_struct.clear();

      for (int i = 1; i <= State.rows; ++i)
         for (int j = 1; j <= State.cols; ++j)
            if (isdigit(State.board[i][j])) {
               unsigned long long seed = splitmix64(i * (State.cols + State.rows + 3) + j);
               for_each_adjacent(i, j, State.rows, State.cols, [&](int nx, int ny) {
                  cell_hash[nx][ny] += seed;
               });
            }
      for (int i = 1; i <= State.rows; ++i)
         for (int j = 1; j <= State.cols; ++j)
            if (Basicresult.marks[i][j] == 基础逻辑结果::Mark::H && !vis[i][j]) {

               cell.clear();

               cell_list(i, j, State, Basicresult, vis, cell);
               result.board_struct.push_back(build_connect(cell, State, Basicresult, cell_hash));
               for (auto i : cell)
                  result.cell2connect[i.first][i.second] = reinterpret_cast<棋盘结构::连通块*>(result.board_struct.size());
               // 这里看着存的是指针，但是实际上存的是下标，防止 vector 重新分配内存导致指针失效，后续会统一转换为真正的指针
            }
      for (int i = 1; i <= State.rows; ++i)
         for (int j = 1; j <= State.cols; ++j)
            if (result.cell2connect[i][j] != nullptr)
               result.cell2connect[i][j] = &result.board_struct.begin()[reinterpret_cast<size_t>(result.cell2connect[i][j]) - 1];
                  // 将 cell2connect 中的索引转换为指针
      return result;
   }
   struct 更新列表 {
      struct 更新 {
         int x, y;
         GameState::Cell newCell;
      };
      vector<更新> 更新们;
   };
   void update(const GameState& 棋盘状态, const 更新列表&列表, 棋盘结构& 构造) {

   }
}; 
