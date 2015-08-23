#!/bin/bash

TEST_DIR=smoke_test

STANDARD=DDR3 # DDR4 LPDDR3 LPDDR4 GDDR5 WideIO WideIO2 HBM
CONFIG=$TEST_DIR/smoketest_$STANDARD.cfg
INPUT=$TEST_DIR/cpu1.trace
TEST_OUTPUT=$TEST_DIR/smoketest-DDR3-chan-0-rank-0.cmdtrace
TEST_RESULT=$TEST_DIR/smoketest-DDR3-chan-0-rank-0.result

./ramulator $CONFIG $INPUT
diff -q $TEST_OUTPUT $TEST_RESULT
