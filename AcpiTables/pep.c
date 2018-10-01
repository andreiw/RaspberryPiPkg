/*
 * PEP device tables.
 *
 * Copyright (c), 2018, Andrey Warkentin <andrey.warkentin@gmail.com>
 * Copyright (c), Microsoft Corporation. All rights reserved.
 *
 * This program and the accompanying materials
 * are licensed and made available under the terms and conditions of the BSD License
 * which accompanies this distribution.  The full text of the license may be found at
 * http://opensource.org/licenses/bsd-license.php
 *
 * THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 * WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 */
#include "pep.h"

PEP_PROCESSOR_TABLE_PLAT RPI2Processors = {
  1, //Version
  1, //NumberProcessors
  { //ProcessorInfo
    { //[0]
      L"\\_SB.CPU0", //DevicePath , wchar_t[16]
      0, //FeedbackCounterCount
      0x00000000, //Flags
      0, //NumberIdleStates
      0, //NumberPerfStates
      { //IdleInfo
      },
      { // perfinfo
      }
    }
  }
};

PEP_COORDINATED_STATE_TABLE_PLAT RPI2CoordinatedStates = {
  1, //Version
  1, //CoordinatedStateCount
  { //CordinatedStates[]
    { //[0]
      { // DependencyInfo
        { //[0]
          1, //ExpectedState
          0, //TargetProcessor
          0x0 | 0x2 | 0x4, //LooseDependency = FALSE, InitialState = TRUE, DependentState = TRUE
        }
      },
      SOC_STATE_TYPE, //StateType
      0x1, //Flags
      0, //Latency
      0, //BreakEvenDuration
      1, //DependencyCount
      1, //MaximumDependencySize
    }
  }
};

PEP_DEVICE_TABLE_PLAT RPI2Devices = {
  1, //Version
  1, //NumberDevices
  { //DeviceInfo
    { //[1]
      L"\\_SB.I2C0", //DevicePath , wchar_t[16]
      0x1 | (1 << 3), //DStateSupportMask (D0 and D3)
      1, //NumberCompoenents
      { //DStateRequirement
        { //[0]
          PowerDeviceD3 //DState
        }
      },
      { // FStateRequirement
        { //[0]
            { //FState
              { //[0]
                0
              }
            }
          }
      }
    }
  }
};
