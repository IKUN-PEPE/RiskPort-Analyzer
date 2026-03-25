# ═══════════════════════════════════════════════════════════
#  Makefile  —  MinGW-w64 构建脚本
#  用法:
#    mingw32-make          → 构建 GUI 版（无控制台窗口）
#    mingw32-make console  → 构建调试版（带控制台输出）
#    mingw32-make clean    → 清理编译产物
# ═══════════════════════════════════════════════════════════

CXX      = g++
TARGET   = PortScanner.exe

SRCS     = main.cpp \
           data.cpp \
           scanner.cpp \
           actions.cpp \
           ui_theme.cpp \
           ui_window.cpp

OBJS     = $(SRCS:.cpp=.o)

CXXFLAGS = -std=c++17 -O2 -Wall -Wextra \
           -DUNICODE -D_UNICODE \
           -municode

LIBS     = -lws2_32 -liphlpapi -lole32 \
           -lcomctl32 -lshell32 -lmsimg32

# ── GUI 版（默认）──────────────────────────────────────────
all: LDFLAGS = -mwindows -static
all: $(TARGET)

# ── 控制台调试版 ───────────────────────────────────────────
console: LDFLAGS = -static
console: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)
	@echo.
	@echo  构建成功: $(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	del /Q *.o $(TARGET) 2>nul || true

.PHONY: all console clean
