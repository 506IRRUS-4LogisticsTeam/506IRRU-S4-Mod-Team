AudioSignalResClass {
 Inputs {
  IOPItemInputClass {
   id 1
   name "TransmissionQuality"
   tl -352.778 176.889
   children {
    6 7 14
   }
   value 1
  }
  IOPInputValueClass {
   id 9
   name "Vol Min [dB]"
   tl -346.364 378.182
   children {
    10
   }
   value -60
  }
  IOPInputValueClass {
   id 16
   name "Vol Min [dB]"
   tl -351.795 543
   children {
    17
   }
   value -12
  }
  IOPInputVariableClass {
   id 24
   name "VON_LEFT"
   tl -51.71 676.789
   children {
    27
   }
   varName "VON_LEFT"
   varResource "{1C14FBA390717FC3}Sounds/VON/VON_DIRECTION.conf"
  }
  IOPInputVariableClass {
   id 25
   name "VON_RIGHT"
   tl -95.815 783.251
   children {
    26
   }
   varName "VON_RIGHT"
   varResource "{1C14FBA390717FC3}Sounds/VON/VON_DIRECTION.conf"
  }
 }
 Ops {
  IOPItemOpInterpolateClass {
   id 6
   name "Interpolate 1"
   tl 36 95
   children {
    5
   }
   inputs {
    ConnectionClass connection {
     id 1
     port 0
    }
   }
   "Y min" 0.85
   "Y max" 0.2
  }
  IOPItemOpInterpolateClass {
   id 7
   name "Interpolate 1"
   tl 39.111 282
   children {
    8
   }
   inputs {
    ConnectionClass connection {
     id 10
     port 4
    }
    ConnectionClass connection {
     id 1
     port 0
    }
   }
   "X max" 0.8
   "Y min" 1
   "Fade In Type" "Power of 1/3"
   "Fade Out Type" "Power of 1/3"
  }
  SignalOpDb2GainClass {
   id 10
   name "Db2Gain 10"
   tl -157.273 378.182
   children {
    7
   }
   inputs {
    ConnectionClass connection {
     id 9
     port 0
    }
   }
  }
  IOPItemOpInterpolateClass {
   id 14
   name "Interpolate 14"
   tl 44.57 465.472
   children {
    12 26 27
   }
   inputs {
    ConnectionClass connection {
     id 17
     port 3
    }
    ConnectionClass connection {
     id 1
     port 0
    }
   }
   "Y min" 1
  }
  SignalOpDb2GainClass {
   id 17
   name "Db2Gain 10"
   tl -156.593 547.75
   children {
    14
   }
   inputs {
    ConnectionClass connection {
     id 16
     port 0
    }
   }
  }
  IOPItemOpMulClass {
   id 26
   name "Mul 26"
   tl 249.424 772.605
   children {
    29
   }
   inputs {
    ConnectionClass connection {
     id 14
     port 0
    }
    ConnectionClass connection {
     id 25
     port 0
    }
   }
  }
  IOPItemOpMulClass {
   id 27
   name "Mul 27"
   tl 263.111 652.455
   children {
    28
   }
   inputs {
    ConnectionClass connection {
     id 14
     port 0
    }
    ConnectionClass connection {
     id 24
     port 0
    }
   }
  }
 }
 Outputs {
  IOPItemOutputClass {
   id 5
   name "Quality_W"
   tl 227 95
   input 6
  }
  IOPItemOutputClass {
   id 8
   name "Noise_V"
   tl 227 281
   input 7
  }
  IOPItemOutputClass {
   id 12
   name "Radio_V"
   tl 499 484.287
   input 14
  }
  IOPItemOutputClass {
   id 28
   name "VON_LEFT"
   tl 507.972 660.06
   input 27
  }
  IOPItemOutputClass {
   id 29
   name "VON_RIGHT"
   tl 500.368 771.084
   input 26
  }
 }
 compiled IOPCompiledClass {
  visited {
   517 389 261 519 133 263 5 391 779 390 651 518 262 135 134 7 6
  }
  ins {
   IOPCompiledIn {
    data {
     3 3 65539 196611
    }
   }
   IOPCompiledIn {
    data {
     1 131075
    }
   }
   IOPCompiledIn {
    data {
     1 262147
    }
   }
   IOPCompiledIn {
    data {
     1 393219
    }
   }
   IOPCompiledIn {
    data {
     1 327683
    }
   }
  }
  ops {
   IOPCompiledOp {
    data {
     1 2 2 0 0
    }
   }
   IOPCompiledOp {
    data {
     1 65538 4 131073 4 0 0
    }
   }
   IOPCompiledOp {
    data {
     1 65539 2 65536 0
    }
   }
   IOPCompiledOp {
    data {
     3 131074 327683 393219 4 262145 3 0 0
    }
   }
   IOPCompiledOp {
    data {
     1 196611 2 131072 0
    }
   }
   IOPCompiledOp {
    data {
     1 262146 4 196609 0 262144 0
    }
   }
   IOPCompiledOp {
    data {
     1 196610 4 196609 0 196608 0
    }
   }
  }
  outs {
   IOPCompiledOut {
    data {
     0
    }
   }
   IOPCompiledOut {
    data {
     0
    }
   }
   IOPCompiledOut {
    data {
     0
    }
   }
   IOPCompiledOut {
    data {
     0
    }
   }
   IOPCompiledOut {
    data {
     0
    }
   }
  }
  processed 17
  version 2
  ins_reeval_list {
   3 4
  }
 }
}