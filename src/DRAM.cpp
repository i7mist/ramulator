#include "DRAM.h"
#include "GDDR5.h"
#include "HBM.h"
#include "LPDDR3.h"
#include "LPDDR4.h"
#include "WideIO2.h"
#include "DSARP.h"

namespace ramulator {

template <>
void DRAM<GDDR5>::regSpecStats() {
    spec->tCK
      .name("tCK")
      .desc("tCK(ns)")
      .precision(6)
      ;
    spec->tCK = spec->speed_entry.tCK;

    spec->nRCDR
      .name("nRCDR")
      .desc("nRCDR(cycle)")
      .precision(6)
      ;
    spec->nRCDR = spec->speed_entry.nRCDR;

    spec->nRCDW
      .name("nRCDW")
      .desc("nRCDW(cycle)")
      .precision(6)
      ;
    spec->nRCDW = spec->speed_entry.nRCDW;

    spec->nRP
      .name("nRP")
      .desc("nRP(cycle)")
      .precision(6)
      ;
    spec->nRP = spec->speed_entry.nRP;

    spec->nBL
      .name("nBL")
      .desc("nBL(cycle)")
      .precision(6)
      ;
    spec->nBL = spec->speed_entry.nBL;

    spec->nCL
      .name("nCL")
      .desc("nCL(cycle)")
      .precision(6)
      ;
    spec->nCL = spec->speed_entry.nCL;

    spec->read_latency_stat
      .name("read_latency")
      .desc("read_latency (ns)")
      .precision(6)
      ;
    spec->read_latency_stat = spec->read_latency * spec->tCK.value();

    spec->frequency
      .name("frequency")
      .desc("memory spec->frequency (MHz)")
      .precision(6)
      ;
    spec->frequency = spec->speed_entry.freq;
}

template <>
void DRAM<HBM>::regSpecStats() {
    spec->tCK
      .name("tCK")
      .desc("tCK(ns)")
      .precision(6)
      ;
    spec->tCK = spec->speed_entry.tCK;

    spec->nRCDR
      .name("nRCDR")
      .desc("nRCDR(cycle)")
      .precision(6)
      ;
    spec->nRCDR = spec->speed_entry.nRCDR;

    spec->nRCDW
      .name("nRCDW")
      .desc("nRCDW(cycle)")
      .precision(6)
      ;
    spec->nRCDW = spec->speed_entry.nRCDW;

    spec->nRP
      .name("nRP")
      .desc("nRP(cycle)")
      .precision(6)
      ;
    spec->nRP = spec->speed_entry.nRP;

    spec->nBL
      .name("nBL")
      .desc("nBL(cycle)")
      .precision(6)
      ;
    spec->nBL = spec->speed_entry.nBL;

    spec->nCL
      .name("nCL")
      .desc("nCL(cycle)")
      .precision(6)
      ;
    spec->nCL = spec->speed_entry.nCL;

    spec->read_latency_stat
      .name("read_latency")
      .desc("read_latency (ns)")
      .precision(6)
      ;
    spec->read_latency_stat = spec->read_latency * spec->tCK.value();

    spec->frequency
      .name("frequency")
      .desc("memory spec->frequency (MHz)")
      .precision(6)
      ;
    spec->frequency = spec->speed_entry.freq;
}

template <>
void DRAM<LPDDR3>::regSpecStats() {
    spec->tCK
      .name("tCK")
      .desc("tCK(ns)")
      .precision(6)
      ;
    spec->tCK = spec->speed_entry.tCK;

    spec->nRCDR
      .name("nRCDR")
      .desc("nRCDR(cycle)")
      .precision(6)
      ;
    spec->nRCDR = spec->speed_entry.nRCD;

    spec->nRCDW
      .name("nRCDW")
      .desc("nRCDW(cycle)")
      .precision(6)
      ;
    spec->nRCDW = spec->speed_entry.nRCD;

    spec->nRP
      .name("nRP")
      .desc("nRP(cycle)")
      .precision(6)
      ;
    spec->nRP = spec->speed_entry.nRPpb;

    spec->nBL
      .name("nBL")
      .desc("nBL(cycle)")
      .precision(6)
      ;
    spec->nBL = spec->speed_entry.nBL;

    spec->nCL
      .name("nCL")
      .desc("nCL(cycle)")
      .precision(6)
      ;
    spec->nCL = spec->speed_entry.nCL;

    spec->read_latency_stat
      .name("read_latency")
      .desc("read_latency (ns)")
      .precision(6)
      ;
    spec->read_latency_stat = spec->read_latency * spec->tCK.value();

    spec->frequency
      .name("frequency")
      .desc("memory spec->frequency (MHz)")
      .precision(6)
      ;
    spec->frequency = spec->speed_entry.freq;
}

template <>
void DRAM<LPDDR4>::regSpecStats() {
    spec->tCK
      .name("tCK")
      .desc("tCK(ns)")
      .precision(6)
      ;
    spec->tCK = spec->speed_entry.tCK;

    spec->nRCDR
      .name("nRCDR")
      .desc("nRCDR(cycle)")
      .precision(6)
      ;
    spec->nRCDR = spec->speed_entry.nRCD;

    spec->nRCDW
      .name("nRCDW")
      .desc("nRCDW(cycle)")
      .precision(6)
      ;
    spec->nRCDW = spec->speed_entry.nRCD;

    spec->nRP
      .name("nRP")
      .desc("nRP(cycle)")
      .precision(6)
      ;
    spec->nRP = spec->speed_entry.nRPpb;

    spec->nBL
      .name("nBL")
      .desc("nBL(cycle)")
      .precision(6)
      ;
    spec->nBL = spec->speed_entry.nBL;

    spec->nCL
      .name("nCL")
      .desc("nCL(cycle)")
      .precision(6)
      ;
    spec->nCL = spec->speed_entry.nCL;

    spec->read_latency_stat
      .name("read_latency")
      .desc("read_latency (ns)")
      .precision(6)
      ;
    spec->read_latency_stat = spec->read_latency * spec->tCK.value();

    spec->frequency
      .name("frequency")
      .desc("memory spec->frequency (MHz)")
      .precision(6)
      ;
    spec->frequency = spec->speed_entry.freq;
}

template <>
void DRAM<DSARP>::regSpecStats() {
    spec->tCK
      .name("tCK")
      .desc("tCK(ns)")
      .precision(6)
      ;
    spec->tCK = spec->speed_entry.tCK;

    spec->nRCDR
      .name("nRCDR")
      .desc("nRCDR(cycle)")
      .precision(6)
      ;
    spec->nRCDR = spec->speed_entry.nRCD;

    spec->nRCDW
      .name("nRCDW")
      .desc("nRCDW(cycle)")
      .precision(6)
      ;
    spec->nRCDW = spec->speed_entry.nRCD;

    spec->nRP
      .name("nRP")
      .desc("nRP(cycle)")
      .precision(6)
      ;
    spec->nRP = spec->speed_entry.nRPpb;

    spec->nBL
      .name("nBL")
      .desc("nBL(cycle)")
      .precision(6)
      ;
    spec->nBL = spec->speed_entry.nBL;

    spec->nCL
      .name("nCL")
      .desc("nCL(cycle)")
      .precision(6)
      ;
    spec->nCL = spec->speed_entry.nCL;

    spec->read_latency_stat
      .name("read_latency")
      .desc("read_latency (ns)")
      .precision(6)
      ;
    spec->read_latency_stat = spec->read_latency * spec->tCK.value();

    spec->frequency
      .name("frequency")
      .desc("memory spec->frequency (MHz)")
      .precision(6)
      ;
    spec->frequency = spec->speed_entry.freq;
}

template <>
void DRAM<ramulator::WideIO2>::regSpecStats() {
    spec->tCK
      .name("tCK")
      .desc("tCK(ns)")
      .precision(6)
      ;
    spec->tCK = spec->speed_entry.tCK;

    spec->nRCDR
      .name("nRCDR")
      .desc("nRCDR(cycle)")
      .precision(6)
      ;
    spec->nRCDR = spec->speed_entry.nRCD;

    spec->nRCDW
      .name("nRCDW")
      .desc("nRCDW(cycle)")
      .precision(6)
      ;
    spec->nRCDW = spec->speed_entry.nRCD;

    spec->nRP
      .name("nRP")
      .desc("nRP(cycle)")
      .precision(6)
      ;
    spec->nRP = spec->speed_entry.nRPpb;

    spec->nBL
      .name("nBL")
      .desc("nBL(cycle)")
      .precision(6)
      ;
    spec->nBL = spec->speed_entry.nBL;

    spec->nCL
      .name("nCL")
      .desc("nCL(cycle)")
      .precision(6)
      ;
    spec->nCL = spec->speed_entry.nCL;

    spec->read_latency_stat
      .name("read_latency")
      .desc("read_latency (ns)")
      .precision(6)
      ;
    spec->read_latency_stat = spec->read_latency * spec->tCK.value();

    spec->frequency
      .name("frequency")
      .desc("memory spec->frequency (MHz)")
      .precision(6)
      ;
    spec->frequency = spec->speed_entry.freq;
}

} // namespace ramulator
