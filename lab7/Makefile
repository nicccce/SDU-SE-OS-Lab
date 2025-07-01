CXX := g++
CXXFLAGS := -std=c++20 -w -O2  # -w 忽略所有警告
TARGET := lab6

all: $(TARGET)
	@./$(TARGET) >/dev/null 2>&1  # 静默运行

$(TARGET): lab6.cpp
	@$(CXX) $(CXXFLAGS) $< -o $@ >/dev/null 2>&1  # 静默编译

clean:
	@rm -f $(TARGET) >/dev/null 2>&1  # 静默清理

.PHONY: all clean