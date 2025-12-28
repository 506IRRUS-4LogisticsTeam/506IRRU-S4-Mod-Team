AudioSignalResClass {
 Inputs {
  IOPItemInputClass {
   id 1
   name "EarRouting"
   tl -253.584 -46.442
   children {
    20 21
   }
  }
  IOPInputVariableClass {
   id 25
   name "Channel Volume"
   tl 84.429 275.99
   children {
    27 31
   }
   varName "ChannelVolume"
   varResource "{23599C437CC8463D}Sounds/VON/RadioEarRouting.conf"
  }
 }
 Ops {
  IOPItemOpConvertorClass {
   id 20
   name "LeftChannelVol"
   tl 71.065 -185.632
   children {
    27
   }
   inputs {
    ConnectionClass "1:0" {
     id 1
     port 0
    }
   }
   Default 2
   Intervals {
    IOPItemOpConvertorRange LeftChannel {
     min 1
     max 2
    }
   }
  }
  IOPItemOpConvertorClass {
   id 21
   name "RightChannelVol"
   tl 76.623 124.05
   children {
    31
   }
   inputs {
    ConnectionClass "1:0" {
     id 1
     port 0
    }
   }
   Default 2
   Intervals {
    IOPItemOpConvertorRange RightChannel {
     min 2
     max 3
    }
   }
  }
  IOPItemOpMulClass {
   id 27
   name "Mul 27"
   tl 309.345 -202.795
   children {
    28
   }
   inputs {
    ConnectionClass "25:0" {
     id 25
     port 0
    }
    ConnectionClass "20:0" {
     id 20
     port 0
    }
   }
  }
  IOPItemOpMulClass {
   id 28
   name "Mul 27"
   tl 480.233 -199.77
   children {
    29
   }
   inputs {
    ConnectionClass "27:0" {
     id 27
     port 0
    }
   }
  }
  IOPItemOpMulClass {
   id 29
   name "Mul 27"
   tl 648.097 -192.209
   children {
    33
   }
   inputs {
    ConnectionClass "28:0" {
     id 28
     port 0
    }
   }
  }
  IOPItemOpMulClass {
   id 30
   name "Mul 27"
   tl 485.778 128.9
   children {
    32
   }
   inputs {
    ConnectionClass "31:0" {
     id 31
     port 0
    }
   }
  }
  IOPItemOpMulClass {
   id 31
   name "Mul 27"
   tl 314.89 125.875
   children {
    30
   }
   inputs {
    ConnectionClass "25:0" {
     id 25
     port 0
    }
    ConnectionClass "21:0" {
     id 21
     port 0
    }
   }
  }
  IOPItemOpMulClass {
   id 32
   name "Mul 27"
   tl 653.642 137.218
   children {
    34
   }
   inputs {
    ConnectionClass "30:0" {
     id 30
     port 0
    }
   }
  }
  IOPItemOpMulClass {
   id 33
   name "Mul 27"
   tl 841.109 -193.058
   children {
    22
   }
   inputs {
    ConnectionClass "29:0" {
     id 29
     port 0
    }
   }
  }
  IOPItemOpMulClass {
   id 34
   name "Mul 27"
   tl 848.695 140.957
   children {
    2
   }
   inputs {
    ConnectionClass "32:0" {
     id 32
     port 0
    }
   }
  }
 }
 Outputs {
  IOPItemOutputClass {
   id 2
   name "RightVol"
   tl 1082.863 139.991
   input 34
  }
  IOPItemOutputClass {
   id 22
   name "LeftVol"
   tl 1099.2 -190.94
   input 33
  }
 }
 Input_Order {
  ItemDetailListItemClass EarRouting {
   Name "EarRouting"
   Id 1
  }
 }
 Output_Order {
  ItemDetailListItemClass RightVol {
   Name "RightVol"
   Id 2
  }
  ItemDetailListItemClass LeftVol {
   Name "LeftVol"
   Id 22
  }
 }
}