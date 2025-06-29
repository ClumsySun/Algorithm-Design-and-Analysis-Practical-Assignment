#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <limits.h>

// 物品结构体
typedef struct {
    int id;         // 物品编号
    int weight;     // 物品重量
    double value;   // 物品价值
    double ratio;   // 单位价值
} Item;

// 生成物品
void gene_item(Item *items, int n) {
    for (int i = 0; i < n; i++) {
        items[i].id = i + 1;
        items[i].weight = rand() % 100 + 1;  // 重量: 1~100
        items[i].value = (rand() % 90001 + 10000) / 100.0; // 价值: 100.00~1000.00 (保留两位小数)
        items[i].ratio = items[i].value / items[i].weight;
    }
}

// 蛮力法
double brute(int n, int C, Item *items, int *chosen, int *cnt) {
    if (n > 30) {
        printf("蛮力法跳过\n", n);
        return -1;
    }

    double max_v = 0.0;
    int final_selection = 0;
    long long total_combinations = 1LL << n;

    // 枚举所有可能组合 (2^n)
    for (long long i = 0; i < total_combinations; i++) {
        int current_weight = 0;
        double current_value = 0.0;
        for (int j = 0; j < n; j++) {
            if (i & (1LL << j)) {
                current_weight += items[j].weight;
                current_value += items[j].value;
            }
        }

        // 更新最优解
        if (current_weight <= C && current_value > max_v) {
            max_v = current_value;
            final_selection = i;
        }
    }

    // 记录选中的物品
    *cnt = 0;
    memset(chosen, 0, n * sizeof(int));
    for (int j = 0; j < n; j++) {
        if (final_selection & (1LL << j)) {
            chosen[j] = 1;
            (*cnt)++;
        }
    }

    return max_v;
}

// 动态规划法
double dynamicProgramming(int n, int C, Item *items, int *chosen, int *cnt) {
    // 检查内存需求 (C+1个long long元素)
    size_t mem = (C + 1) * sizeof(long long);
    if (mem > 2000000000) { // 2GB内存限制
        printf("动规内存分配失败！\n");
        return -1;
    }

    // 转long long减少计算时间
    long long *dp = calloc(C + 1, sizeof(long long));
    if (!dp) {
        printf("动规内存分配失败！\n");
        return -1;
    }

    // 动态规划按行填表
    for (int i = 0; i < n; i++) {
        int w = items[i].weight;
        long long v = (long long)(items[i].value * 100 + 0.5);
        for (int j = C; j >= w; j--) {
            if (dp[j - w] + v > dp[j]) {
                dp[j] = dp[j - w] + v;
            }
        }
    }

    // 记录选中的物品
    *cnt = 0;
    memset(chosen, 0, n * sizeof(int));
    for (int j = C; j >= 0; j--) {
        if (dp[j] > 0) {
            int i = 0;
            while (i < n && items[i].weight > j) i++;
            if (i < n) {
                chosen[i] = 1;
                (*cnt)++;
            }
        }
    }

    // 计算最大价值
    double max_v = dp[C] / 100.0;
    free(dp);
    *cnt = 0; // 动态规划不记录具体物品选择
    return max_v;
}


int compare(const void *a, const void *b) {
    Item *itemA = (Item *)a;
    Item *itemB = (Item *)b;
    if (itemA->ratio < itemB->ratio) return 1;
    if (itemA->ratio > itemB->ratio) return -1;
    return 0;
}

// 贪心法
double greedy(int n, int C, Item *items, int *chosen, int *cnt) {
    // 按单位价值排序
    Item *sorts = malloc(n * sizeof(Item));
    memcpy(sorts, items, n * sizeof(Item));
    qsort(sorts, n, sizeof(Item), compare);
    double total_v = 0.0;
    int curr_w = 0;
    *cnt = 0;
    memset(chosen, 0, n * sizeof(int));
    // 遍历物品，贪心选择
    for (int i = 0; i < n; i++) {
        if (curr_w + sorts[i].weight <= C) {
            curr_w += sorts[i].weight;
            total_v += sorts[i].value;
            chosen[sorts[i].id - 1] = 1;
            (*cnt)++;
        }
    }

    free(sorts);
    return total_v;
}

// 节点结构体
typedef struct {
    int level;      // 当前决策层级
    int weight;     // 当前总重量
    double value;   // 当前总价值
    double bound;   // 价值上界
} Node;

// 优先队列
typedef struct {
    Node *nodes;
    int capacity;
    int size;
} PriorityQueue;

PriorityQueue *create_queue(int capacity) {
    PriorityQueue *pq = malloc(sizeof(PriorityQueue));
    pq->nodes = malloc(capacity * sizeof(Node));
    pq->capacity = capacity;
    pq->size = 0;
    return pq;
}

void free_queue(PriorityQueue *pq) {
    if (pq) {
        free(pq->nodes);
        free(pq);
    }
}

void push(PriorityQueue *pq, Node node) {
    if (pq->size >= pq->capacity) {
        int new_capacity = pq->capacity * 2;
        Node *new_nodes = realloc(pq->nodes, new_capacity * sizeof(Node));
        if (!new_nodes) return;
        pq->nodes = new_nodes;
        pq->capacity = new_capacity;
    }
    
    pq->nodes[pq->size] = node;
    int i = pq->size;
    while (i > 0 && pq->nodes[(i-1)/2].bound < pq->nodes[i].bound) {
        Node temp = pq->nodes[i];
        pq->nodes[i] = pq->nodes[(i-1)/2];
        pq->nodes[(i-1)/2] = temp;
        i = (i-1)/2;
    }
    pq->size++;
}

Node pop(PriorityQueue *pq) {
    Node result = pq->nodes[0];
    pq->size--;
    pq->nodes[0] = pq->nodes[pq->size];
    int i = 0;
    while (1) {
        int left = 2*i+1;
        int right = 2*i+2;
        int largest = i;
        if (left < pq->size && pq->nodes[left].bound > pq->nodes[largest].bound) 
            largest = left;
        if (right < pq->size && pq->nodes[right].bound > pq->nodes[largest].bound) 
            largest = right;
        if (largest == i) break;
        Node temp = pq->nodes[i];
        pq->nodes[i] = pq->nodes[largest];
        pq->nodes[largest] = temp;
        i = largest;
    }
    return result;
}




// 计算价值上界
double bound(Node u, int n, int C, Item *items) {
    if (u.weight >= C) return 0;
    double bou = u.value;
    int j = u.level + 1;
    int total_w = u.weight;
    
    // 计算剩余容量
    while (j < n && total_w + items[j].weight <= C) {
        total_w += items[j].weight;
        bou += items[j].value;
        j++;
    }
    if (j < n) {
        bou += (C - total_w) * items[j].ratio;
    }
    return bou;
}

// 回溯法
double backtrack(int n, int C, Item *items, int *chosen, int *cnt) {
    if (n > 100) {
        printf("回溯法跳过");
        return -1;
    }

    // 按单位价值降序排序
    Item *sorts = malloc(n * sizeof(Item));
    memcpy(sorts, items, n * sizeof(Item));
    qsort(sorts, n, sizeof(Item), compare);

    // 初始化优先队列
    PriorityQueue *pq = create_queue(1000000);
    Node tmp_n, curr_n;
    double max_v = 0.0;
    int *best_chosen = calloc(n, sizeof(int));
    int *curr_chosen = calloc(n, sizeof(int));

    // 初始化根节点
    curr_n.level = -1;
    curr_n.weight = 0;
    curr_n.value = 0;
    curr_n.bound = bound(curr_n, n, C, sorts);
    push(pq, curr_n);

    // 回溯搜索
    while (pq->size > 0) {
        tmp_n = pop(pq);
        if (tmp_n.bound > max_v) {
            // 选择下一个物品
            curr_n.level = tmp_n.level + 1;
            
            if (curr_n.level >= n) continue;

            // 标记选择
            curr_n.weight = tmp_n.weight + sorts[curr_n.level].weight;
            curr_n.value = tmp_n.value + sorts[curr_n.level].value;
            curr_chosen[curr_n.level] = 1;
            if (curr_n.weight <= C && curr_n.value > max_v) {
                max_v = curr_n.value;
                memcpy(best_chosen, curr_chosen, n * sizeof(int));
            }
            curr_n.bound = bound(curr_n, n, C, sorts);
            if (curr_n.bound > max_v && curr_n.level < n - 1) {
                push(pq, curr_n);
            }
            
            // 不选下一个物品
            // 标记不选择
            curr_n.weight = tmp_n.weight;
            curr_n.value = tmp_n.value;
            curr_chosen[curr_n.level] = 0;
            curr_n.bound = bound(curr_n, n, C, sorts);
            if (curr_n.bound > max_v && curr_n.level < n - 1) {
                push(pq, curr_n);
            }
        }
    }

    // 记录选中的物品
    *cnt = 0;
    memset(chosen, 0, n * sizeof(int));
    for (int i = 0; i < n; i++) {
        if (best_chosen[i]) {
            chosen[sorts[i].id - 1] = 1;
            (*cnt)++;
        }
    }
    free(curr_chosen);
    free(sorts);
    free_queue(pq);
    free(best_chosen);
    
    return max_v;
}

// 打印结果
void print_results(int n, int C, Item *items, int *chosen, double max_v, int cnt, 
                   const char *algorithm, double exec_time) {
    printf("\n===== %s算法结果 =====\n", algorithm);
    printf("物品数量: %d, 背包容量: %d\n", n, C);
    printf("最大价值: %.2f\n", max_v);
    printf("执行时间: %.2f 毫秒\n", exec_time);
    printf("选择物品数量: %d\n", cnt);
    
    if (n <= 20) { // 在小规模时打印详细选择
        printf("选择的物品:\n");
        printf("编号\t重量\t价值\n");
        double total_v = 0.0;
        int total_w = 0;
        for (int i = 0; i < n; i++) {
            if (chosen[i]) {
                printf("%d\t%d\t%.2f\n", items[i].id, items[i].weight, items[i].value);
                total_v += items[i].value;
                total_w += items[i].weight;
            }
        }
        printf("总计: 重量=%d, 价值=%.2f\n", total_w, total_v);
    }
}

void save_items(int n, Item *items, const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("无法打开文件");
        return;
    }
    
    fprintf(fp, "物品编号,物品重量,物品价值\n");
    for (int i = 0; i < n; i++) {
        fprintf(fp, "%d,%d,%.2f\n", items[i].id, items[i].weight, items[i].value);
    }
    
    fclose(fp);
    printf("物品信息已保存到 %s\n", filename);
}

int main() {
    srand(time(NULL));
    printf("0-1背包问题求解程序\n");


    int num_list[] = {10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95, 100, 1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000, 
                      20000, 40000, 80000, 160000, 320000};
    int C_list[] = {100, 10000, 100000, 1000000};
    int num_len = sizeof(num_list) / sizeof(num_list[0]);
    int C_len = sizeof(C_list) / sizeof(C_list[0]);

    // 打开结果文件
    FILE *fp = fopen("results.csv", "w");
    if (!fp) {
        perror("无法打开文件");
        return 1;
    }
    fprintf(fp, "n,C,算法,最大价值,执行时间(ms)\n");
    printf("结果保存地 results.csv\n");
    
    int saved = 0;
    clock_t total_start = clock();    

    for (int i = 0; i < num_len; i++) {
        int n = num_list[i];
        printf("\n========= 测试规模: %d =========\n", n);
        
        Item *items = malloc(n * sizeof(Item));
        if (!items) {
            fprintf(stderr, "内存分配失败: n=%d\n", n);
            continue;
        }
        gene_item(items, n);

        // 保存1000个物品
        if (n == 1000 && !saved) {
            save_items(n, items, "1000_items.csv");
            saved = 1;
        }


        for (int j = 0; j < C_len; j++) {
            int C = C_list[j];
            
            // 跳过一些规模和容量组合
            if (n <= 100 && C > 1000
                || n > 100 && C < 1000)
                continue;

            printf("\n");
            printf("<<背包容量>>: %d\n", C);
            
            
            // 蛮力法
            printf("蛮力法 (n=%d, C=%d)...\n", n, C);
            int *chosen_brute = calloc(n, sizeof(int));
            int count_brute = 0;
            clock_t start_brute = clock();
            double value_brute = brute(n, C, items, chosen_brute, &count_brute);
            clock_t end_brute = clock();
            double time_brute = (double)(end_brute - start_brute) * 1000 / CLOCKS_PER_SEC;
            
            if (value_brute >= 0) {
                fprintf(fp, "%d,%d,Brute,%.2f,%.2f\n", n, C, value_brute, time_brute);
                if (n <= 20) {
                    print_results(n, C, items, chosen_brute, value_brute, count_brute, "蛮力", time_brute);
                }
                else {
                    printf("最大价值: %.2f\t执行时间: %.2f 毫秒\t选择物品数量: %d\n", value_brute, time_brute, count_brute);
                }
            }
            free(chosen_brute);
            
            // 动态规划法
            printf("动态规划 (n=%d, C=%d)...\n", n, C);
            int *chosen_dp = calloc(n, sizeof(int));
            int cnt_dp = 0;
            clock_t start_dp = clock();
            double value_dp = dynamicProgramming(n, C, items, chosen_dp, &cnt_dp);
            clock_t end_dp = clock();
            double time_dp = (double)(end_dp - start_dp) * 1000 / CLOCKS_PER_SEC;
            
            if (value_dp >= 0) {
                fprintf(fp, "%d,%d,DP,%.2f,%.2f\n", n, C, value_dp, time_dp);
                if (n <= 20) {
                    print_results(n, C, items, chosen_dp, value_dp, cnt_dp, "动态规划", time_dp);
                }
                else {
                    printf("最大价值: %.2f\t执行时间: %.2f 毫秒\t选择物品数量: %d\n", value_dp, time_dp, cnt_dp);
                }
            }
            free(chosen_dp);
            
            // 贪心法
            printf("贪心法 (n=%d, C=%d)...\n", n, C);
            int *chosen_greedy = calloc(n, sizeof(int));
            int cnt_greedy = 0;
            clock_t start_greedy = clock();
            double value_greedy = greedy(n, C, items, chosen_greedy, &cnt_greedy);
            clock_t end_greedy = clock();
            double time_greedy = (double)(end_greedy - start_greedy) * 1000 / CLOCKS_PER_SEC;
            fprintf(fp, "%d,%d,Greedy,%.2f,%.2f\n", n, C, value_greedy, time_greedy);
            if (n <= 20) {
                print_results(n, C, items, chosen_greedy, value_greedy, cnt_greedy, "贪心", time_greedy);
            }
            else{
                printf("最大价值: %.2f\t执行时间: %.2f 毫秒\t选择物品数量: %d\n", value_greedy, time_greedy, cnt_greedy);
            }
            free(chosen_greedy);

            // 回溯法
            printf("回溯法 (n=%d, C=%d)...\n", n, C);
            int *chosen_bt = calloc(n, sizeof(int));
            int cnt_bt = 0;
            clock_t start_bt = clock();
            double value_bt = backtrack(n, C, items, chosen_bt, &cnt_bt);
            clock_t end_bt = clock();
            double time_bt = (double)(end_bt - start_bt) * 1000 / CLOCKS_PER_SEC;
            
            if (value_bt >= 0) {
                fprintf(fp, "%d,%d,Backtrack,%.2f,%.2f\n", n, C, value_bt, time_bt);
                if (n <= 20) {
                    print_results(n, C, items, chosen_bt, value_bt, cnt_bt, "回溯", time_bt);
                }
                else {
                    printf("最大价值: %.2f\t执行时间: %.2f 毫秒\t选择物品数量: %d\n", value_bt, time_bt, cnt_bt);
                }
            }
            free(chosen_bt);
            
            
        }
        free(items);
    }

    fclose(fp);
    
    clock_t total_end = clock();
    double total_t = (double)(total_end - total_start) * 1000 / CLOCKS_PER_SEC;
    
    printf("\n总执行时间: %.2f 秒\n", total_t/1000);
    printf("结果已保存到 results.csv\n");
    
    return 0;
}