// 实验六：内存置换算法实验

#include <vector>
#include <ranges>
#include <list>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <cstdio>
#include <random>

using std::vector, 
    std::views::iota,
    std::find,
    std::find_if,
    std::ranges::generate,
    std::list,
    std::ranges::end,
    std::multimap,
    std::unordered_map;
template<typename Range, typename T>
bool contains(const Range& range, const T& value) {
    return std::find(range.begin(), range.end(), value) != range.end();
}
using page = int;
constexpr page EMPTY_PAGE = 0;

class Status {
    page cur;
    int n_access, n_fault;
    vector<page> eliminateds;
public:
    void access(page to) {
        n_access += 1;
        cur = to;
        printf("accessing %d\n", to);
    }
    void fault(page victim = EMPTY_PAGE) {
        n_fault += 1;
        printf("%d -> %d\n", victim, cur);
        if (victim) eliminateds.push_back(victim);
    }
    void report() {
        printf("=== page fault report ==\n");
        printf("Eliminate pages:");
        for (page p : eliminateds) printf(" %d", p);
        printf("\n");
        printf("Number of page faults: %d\n", n_fault);
        if (n_access > 0)
            printf("Rate of page faults: %.1f%%\n", 100.0 * n_fault / n_access);
        printf("=== report end ==\n\n");

        // 清理
        n_access = 0;
        n_fault = 0;
        eliminateds.clear();
    }
};

struct PageReplacer {
    Status status; 
    int nframe;
    vector<page> access_seq;
    void fifo();
    void lru();
    void lfu();
    void clock();
    void enchanced_clock();
};

// FIFO算法
void PageReplacer::fifo() {
    vector<page> pages(nframe, EMPTY_PAGE);
    int i = 0;
    for (page target : access_seq) {
        status.access(target);
        if (contains(pages, target)) continue;
        status.fault(pages[i]);
        pages[i] = target;
        i = (i + 1) % pages.size();   
    }
    status.report();
}

// LRU算法
// 使用双向链表实现，哈希表优化查找时间
void PageReplacer::lru() {
    list<page> pages;
    unordered_map<page, list<page>::iterator> iters;
    for (page target : access_seq) {
        status.access(target);

        // 命中，将命中页前置
        if (iters.contains(target)) {
            pages.splice(pages.begin(), pages, iters[target]);
            continue;
        };

        // 未命中，加入target
        pages.push_front(target);
        iters[target] = pages.begin();
        
        if (pages.size() > nframe) {
            // 如果超出frame限制，则去掉最后的
            page victim = pages.back();
            iters.erase(victim);
            pages.pop_back();
            status.fault(victim);
        } else {
            status.fault();
        }
    }
    status.report();
}

// LFU算法，替换使用频率最低的页
void PageReplacer::lfu() {
    multimap<int, page> pages;
    unordered_map<page, int> counts;

    for (page target : access_seq) {
        status.access(target);  
        int& cnt = counts[target];
        auto [bg, ed] = pages.equal_range(cnt);
        auto it = find_if(bg, ed, [&target](const auto& pair) {
            return pair.second == target;
        });
        if (it != ed) {
            // 命中了，只需要删除旧的
            pages.erase(it);
        } else if (pages.size() < nframe) {
            // 尚有空页
            status.fault();
        } else {
            // 未命中，替换掉最少的
            auto victim = pages.begin();
            status.fault(victim->second);
            pages.erase(victim);
        }
        // 插入新的
        pages.insert({ cnt += 1, target });
    }

    status.report();
}

// 二次机会法，FIFO的增强版本
void PageReplacer::clock() {
    struct page_t {
        page pid = EMPTY_PAGE;
        bool r = false;
    };
    vector<page_t> pages(nframe);
    int i = 0;
    for (page p : access_seq) {
        status.access(p);
        // 先判断是否命中
        auto it = std::ranges::find_if(pages, [&p](page_t& page) {
            return page.pid == p;
        });
        // 命中，更改那个页的r标记
        if (it != end(pages)) {
            it->r = true;
            continue;
        }
        // 未命中，开始考虑换掉的页
        choose_victim:
        auto& [old, r] = pages[i];
        i = (i + 1) % pages.size();
        if (r ^= true) goto choose_victim;
        // 替换掉老页面
        status.fault(old);
        old = p;
    }
    status.report();
}

// 增强二次机会法
void PageReplacer::enchanced_clock() {
    struct page_t {
        page pid = EMPTY_PAGE;
        bool r = false;
        bool d = false;
    };
    vector<page_t> pages(nframe);

    int i = 0;
    for (page p : access_seq) {
        bool modified = random() % 2;
        status.access(p);
        if (modified) printf("this access will modify page %d\n", p);
        // 先判断是否命中
        auto it = std::ranges::find_if(pages, [&p](page_t& page) {
            return page.pid == p;
        });
        // 命中，更改那个页的r/d标记
        if (it != end(pages)) {
            it->r = true;
            it->d |= modified;
            continue;
        }
        // 未命中，开始考虑换掉的页
        int i_begin = i;

    choose_victim_by_r_d: // 先扫描(r,d)均为0的
        page_t& page = pages[i];
        if (!page.r & !page.d) goto replace;
        page.r = false;
        if ((i = (i + 1) % pages.size()) != i_begin) goto choose_victim_by_r_d;
        
    choose_victim_by_r: // 退而求其次，扫描r为0。一定能找到
        page = pages[i];
        if (!page.r) goto replace;
        if ((i = (i + 1) % pages.size()) != i_begin) goto choose_victim_by_r;

    replace:
        auto& [pid, r, d] = page;
        status.fault(pid);
        pid = p;
        r = true;
        d = modified;

        i = (i + 1) % pages.size();
    }

    status.report();   
}

int main(int argc, char const *argv[]) {
    std::mt19937 rng;
    std::uniform_int_distribution dist(1, 10);
    vector<int> seq(30);

    // 五次模拟
    for (int i : iota(0, 5)) {
        printf("\n\ntest %d\n", i + 1);

        // 生成序列
        generate(seq, [&]() { return dist(rng); });
        
        PageReplacer vmpr {
            .nframe = 6,
            .access_seq = seq
        };
        vmpr.fifo();
        vmpr.lru();
        vmpr.lfu();
        vmpr.clock();
        vmpr.enchanced_clock();
    }

    return 0;
}