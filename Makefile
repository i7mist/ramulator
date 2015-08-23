SRCDIR := src
OBJDIR := obj
MAIN := $(SRCDIR)/Main.cpp
SRCS := $(filter-out $(MAIN) $(SRCDIR)/Gem5Wrapper.cpp, $(wildcard $(SRCDIR)/*.cpp))
OBJS := $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SRCS))

# Ramulator currently supports g++ 5.1+ or clang++ 3.4+.  It will NOT work with
#   g++ 4.x due to an internal compiler error when processing lambda functions.
# CXX := g++-5
CXX := clang++
CXXFLAGS := -O3 -std=c++11 -g -Wall


.PHONY: all clean depend test

all: depend ramulator

clean:
	rm -f ramulator-dramtrace ramulator-cputrace $(SRCDIR)/*.o
	rm -rf $(OBJDIR)

depend: $(OBJDIR)/.depend


$(OBJDIR)/.depend: $(SRCS)
	@mkdir -p $(OBJDIR)
	@rm -f $(OBJDIR)/.depend
	@$(foreach SRC, $(SRCS), $(CXX) $(CXXFLAGS) -DRAMULATOR -MM -MT $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SRC)) $(SRC) >> $(OBJDIR)/.depend ;)

ifneq ($(MAKECMDGOALS),clean)
-include $(OBJDIR)/.depend
endif


ramulator: $(MAIN) $(OBJS) $(SRCDIR)/*.h | depend
	$(CXX) $(CXXFLAGS) -DRAMULATOR -o $@ $(MAIN) $(OBJS)

$(OBJS): | $(OBJDIR)

$(OBJDIR): 
	@mkdir -p $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -DRAMULATOR -c -o $@ $<

test:
	./run_test.sh
