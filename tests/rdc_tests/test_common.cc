/*
Copyright (c) 2019 - Advanced Micro Devices, Inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <assert.h>
#include <stdint.h>
#include <getopt.h>

#include <iostream>
#include <string>
#include <map>

#include "rdc_tests/test_base.h"
#include "rdc_tests/test_common.h"
#include "rocm_smi/rocm_smi.h"

static const std::map<grpc_connectivity_state, const char *> kGRPCChanState = {
    {
      {GRPC_CHANNEL_IDLE, "GRPC_CHANNEL_IDLE: Channel is idle"},
      {GRPC_CHANNEL_CONNECTING,
                            "GRPC_CHANNEL_CONNECTING: Channel is connecting"},
      {GRPC_CHANNEL_READY, "GRPC_CHANNEL_READY: Channel is ready for work"},
      {GRPC_CHANNEL_TRANSIENT_FAILURE, "GRPC_CHANNEL_TRANSIENT_FAILURE: "
                         "Channel has seen a failure but expects to recover"},
      {GRPC_CHANNEL_SHUTDOWN, "GRPC_CHANNEL_SHUTDOWN: "
                    "Channel has seen a failure that it cannot recover from"},
    },
};

static const std::map<rsmi_gpu_block_t, const char *> kBlockNameMap = {
    {RSMI_GPU_BLOCK_UMC, "UMC"},
    {RSMI_GPU_BLOCK_SDMA, "SDMA"},
    {RSMI_GPU_BLOCK_GFX, "GFX"},
    {RSMI_GPU_BLOCK_MMHUB, "MMHUB"},
    {RSMI_GPU_BLOCK_ATHUB, "ATHUB"},
    {RSMI_GPU_BLOCK_PCIE_BIF, "PCIE_BIF"},
    {RSMI_GPU_BLOCK_HDP, "HDP"},
    {RSMI_GPU_BLOCK_XGMI_WAFL, "XGMI_WAFL"},
    {RSMI_GPU_BLOCK_DF, "DF"},
    {RSMI_GPU_BLOCK_SMN, "SMN"},
    {RSMI_GPU_BLOCK_SEM, "SEM"},
    {RSMI_GPU_BLOCK_MP0, "MP0"},
    {RSMI_GPU_BLOCK_MP1, "MP1"},
    {RSMI_GPU_BLOCK_FUSE, "FUSE"},
};
static_assert(RSMI_GPU_BLOCK_LAST == RSMI_GPU_BLOCK_FUSE,
                                         "kBlockNameMap needs to be updated");

static const char * kRasErrStateStrings[] = {
    "None",                          // RSMI_RAS_ERR_STATE_NONE
    "Disabled",                      // RSMI_RAS_ERR_STATE_DISABLED
    "Error Unknown",                 // RSMI_RAS_ERR_STATE_PARITY
    "Single, Correctable",           // RSMI_RAS_ERR_STATE_SING_C
    "Multiple, Uncorrectable",       // RSMI_RAS_ERR_STATE_MULT_UC
    "Poison"                         // RSMI_RAS_ERR_STATE_POISON
    "Off",                           // RSMI_RAS_ERR_STATE_DISABLED
    "On",                            // RSMI_RAS_ERR_STATE_ENABLED
};
static_assert(
  sizeof(kRasErrStateStrings)/sizeof(char *) == (RSMI_RAS_ERR_STATE_LAST + 1),
                                       "kErrStateNameMap needs to be updated");


static const std::map<rsmi_ras_err_state_t, const char *> kErrStateNameMap = {
    {RSMI_RAS_ERR_STATE_NONE,
                            kRasErrStateStrings[RSMI_RAS_ERR_STATE_NONE]},
    {RSMI_RAS_ERR_STATE_DISABLED,
                            kRasErrStateStrings[RSMI_RAS_ERR_STATE_DISABLED]},
    {RSMI_RAS_ERR_STATE_PARITY,
                            kRasErrStateStrings[RSMI_RAS_ERR_STATE_PARITY]},
    {RSMI_RAS_ERR_STATE_SING_C,
                            kRasErrStateStrings[RSMI_RAS_ERR_STATE_SING_C]},
    {RSMI_RAS_ERR_STATE_MULT_UC,
                            kRasErrStateStrings[RSMI_RAS_ERR_STATE_MULT_UC]},
    {RSMI_RAS_ERR_STATE_POISON,
                            kRasErrStateStrings[RSMI_RAS_ERR_STATE_POISON]},
    {RSMI_RAS_ERR_STATE_ENABLED,
                            kRasErrStateStrings[RSMI_RAS_ERR_STATE_ENABLED]},
};
static_assert(RSMI_RAS_ERR_STATE_LAST == RSMI_RAS_ERR_STATE_ENABLED,
                                      "kErrStateNameMap needs to be updated");

static const struct option long_options[] = {
  {"iterations", required_argument, nullptr, 'i'},
  {"verbose", required_argument, nullptr, 'v'},
  {"monitor_verbose", required_argument, nullptr, 'm'},
  {"remote_server_ip", required_argument, nullptr, 's'},
  {"remote_server_port", required_argument, nullptr, 'p'},
  {"start_rdcd", optional_argument, nullptr, 'd'},
  {"dont_fail", no_argument, nullptr, 'f'},
  {"rdctst_help", no_argument, nullptr, 'r'},

  {nullptr, 0, nullptr, 0}
};
static const char* short_options = "i:v:m:s:p:d:fr";

static void PrintHelp(void) {
  std::cout <<
     "Optional rdctst Arguments:\n"
     "--dont_fail, -f if set, don't fail test when individual test fails; "
         "default is to fail when an individual test fails\n"
     "--rdctst_help, -r print this help message\n"
     "--verbosity, -v <verbosity level>\n"
     "  Verbosity levels:\n"
     "   0    -- minimal; just summary information\n"
     "   1    -- intermediate; show intermediate values such as intermediate "
                  "perf. data\n"
     "   2    -- progress; show progress displays\n"
     "   >= 3 -- more debug output\n"
     "--start_rdcd <optional path>; start default version of rdcd, or "
                                                 "optionally specified rdcd\n"
     "--remote_server_ip <ip address>; connect to already running "
                                                      "rdcd on specified IP\n"
     "--remote_server_port <remote port>; connect to already running "
                                         "rdcd on specified IP at this port\n";
}

uint32_t ProcessCmdline(RDCTstGlobals* test, int arg_cnt, char** arg_list) {
  int a;
  int ind = -1;

  assert(test != nullptr);

  while (true) {
    a = getopt_long(arg_cnt, arg_list, short_options, long_options, &ind);

    if (a == -1) {
      break;
    }

    switch (a) {
      case 'i':
        test->num_iterations = std::stoi(optarg);
        break;

      case 'v':
        test->verbosity = std::stoi(optarg);
        break;

      case 'm':
        test->monitor_verbosity = std::stoi(optarg);
        break;

      case 'd':
        if (optarg) {
          test->rdcd_path = optarg;
        } else {
          test->rdcd_path = "/usr/sbin/rdcd";
        }
        break;

      case 's':
        test->monitor_server_ip = optarg;
        break;

      case 'p':
        test->monitor_server_port = optarg;
      break;

      case 'r':
        PrintHelp();
        return 1;

      case 'f':
        test->dont_fail = true;
        break;

      default:
        std::cout << "Unknown command line option: \"" << a <<
                                               "\". Ignoring..." << std::endl;
        PrintHelp();
        return 0;
    }
  }
  return 0;
}

const char *GetBlockNameStr(rsmi_gpu_block_t id) {
  return kBlockNameMap.at(id);
}
const char *GetErrStateNameStr(rsmi_ras_err_state_t st) {
  return kErrStateNameMap.at(st);
}
const char *GetGRPCChanStateStr(grpc_connectivity_state st) {
  return kGRPCChanState.at(st);
}

const char *FreqEnumToStr(rsmi_clk_type rsmi_clk) {
  static_assert(RSMI_CLK_TYPE_LAST == RSMI_CLK_TYPE_MEM,
                                       "FreqEnumToStr() needs to be updated");
  switch (rsmi_clk) {
    case RSMI_CLK_TYPE_SYS:  return "System clock";
    case RSMI_CLK_TYPE_DF:   return "Data Fabric clock";
    case RSMI_CLK_TYPE_DCEF: return "Display Controller Engine clock";
    case RSMI_CLK_TYPE_SOC:  return "SOC clock";
    case RSMI_CLK_TYPE_MEM:  return "Memory clock";
    default: return "Invalid Clock ID";
  }
}

#if ENABLE_SMI
void DumpMonitorInfo(const TestBase *test) {
  int ret = 0;
  uint32_t value;
  uint32_t value2;
  std::string val_str;
  std::vector<std::string> val_vec;

  assert(test != nullptr);
  assert(test->monitor_devices() != nullptr &&
                            "Make sure to call test->set_monitor_devices()");
  auto print_attr_label =
      [&](std::string attrib) -> bool {
          std::cout << "\t** " << attrib;
          if (ret == -1) {
            std::cout << "not available" << std::endl;
            return false;
          }
          return true;
  };

  auto delim = "\t***********************************";

  std::cout << "\t***** Hardware monitor values *****" << std::endl;
  std::cout << delim << std::endl;
  std::cout.setf(std::ios::dec, std::ios::basefield);
  for (auto dev : *test->monitor_devices()) {
    auto print_vector =
                     [&](amd::smi::DevInfoTypes type, std::string label) {
      ret = dev->readDevInfo(type, &val_vec);
      if (print_attr_label(label)) {
        for (auto vs : val_vec) {
          std::cout << "\t**  " << vs << std::endl;
        }
        val_vec.clear();
      }
    };
    auto print_val_str =
                     [&](amd::smi::DevInfoTypes type, std::string label) {
      ret = dev->readDevInfo(type, &val_str);

      std::cout << "\t** " << label;
      if (ret == -1) {
        std::cout << "not available";
      } else {
        std::cout << val_str;
      }
      std::cout << std:: endl;
    };

    print_val_str(amd::smi::kDevDevID, "Device ID: ");
    print_val_str(amd::smi::kDevPerfLevel, "Performance Level: ");
    print_val_str(amd::smi::kDevOverDriveLevel, "OverDrive Level: ");
    print_vector(amd::smi::kDevGPUMClk,
                                 "Supported GPU Memory clock frequencies:\n");
    print_vector(amd::smi::kDevGPUSClk,
                                    "Supported GPU clock frequencies:\n");

    if (dev->monitor() != nullptr) {
      ret = dev->monitor()->readMonitor(amd::smi::kMonName, &val_str);
      if (print_attr_label("Monitor name: ")) {
        std::cout << val_str << std::endl;
      }

      ret = dev->monitor()->readMonitor(amd::smi::kMonTemp, &value);
      if (print_attr_label("Temperature: ")) {
        std::cout << static_cast<float>(value)/1000.0 << "C" << std::endl;
      }

      std::cout.setf(std::ios::dec, std::ios::basefield);

      ret = dev->monitor()->readMonitor(amd::smi::kMonMaxFanSpeed, &value);
      if (ret == 0) {
        ret = dev->monitor()->readMonitor(amd::smi::kMonFanSpeed, &value2);
      }
      if (print_attr_label("Current Fan Speed: ")) {
        std::cout << value2/static_cast<float>(value) * 100 << "% (" <<
                                   value2 << "/" << value << ")" << std::endl;
      }
    }
    std::cout << "\t=======" << std::endl;
  }
  std::cout << delim << std::endl;
}
#endif